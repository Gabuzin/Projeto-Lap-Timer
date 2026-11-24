
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Example CYD  ESP32-2432S028R [Loading screen + LED RGB control]         // 
// Design UI on Squareline Studio. LVGL V9.1                               //
// Youtube:https://www.youtube.com/@pangcrd                                //
// Github: https://github.com/pangcrd                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "lvgl.h"
#include "ui.h"
#include "E108.h"
#include "LapData.h"
#include "LapDisplayFormat.h"
#include "WEB/page.h"

// UART2 para o STM: TX do STM -> GPIO22; RX do STM -> GPIO27; GND comum.
static constexpr int RX_STM = 22;
static constexpr int TX_STM = 27;
static constexpr uint32_t BAUD_STM = 115200;
static HardwareSerial SerialSTM(2);

static float rollGraus = 0.0f;
static float pitchGraus = 0.0f;
static float yawGraus = 0.0f;

static void processaAtitudeStm(const char *linha)
{
    float roll, pitch, yaw;
    int fim = 0;
    // O STM transmite $ATT,roll,pitch,yaw em graus.
    if (sscanf(linha, "$ATT,%f,%f,%f%n", &roll, &pitch, &yaw, &fim) != 3 ||
        fim == 0 || linha[fim] != '\0' ||
        !isfinite(roll) || !isfinite(pitch) || !isfinite(yaw) ||
        roll < -180.0f || roll > 180.0f) {
        return;
    }
    rollGraus = roll;
    pitchGraus = pitch;
    yawGraus = yaw;
}

static void processaLinhaStm(const char *linha)
{
    if (strncmp(linha, "$ATT,", 5) == 0) {
        processaAtitudeStm(linha);
    } else {
        GPS.ProcessaLinha(linha);
    }
}

static void encaminhaSerialStm()
{
    static char linha[96];
    static size_t tamanho = 0;

    // Acumula mensagens; '$' tambem recupera sentencas coladas sem newline.
    while (SerialSTM.available()) {
        const int recebido = SerialSTM.read();
        if (recebido < 0) break;
        Serial.write(static_cast<uint8_t>(recebido));
        const char c = static_cast<char>(recebido);
        if (c == '$') {
            if (tamanho > 0) {
                linha[tamanho] = '\0';
                processaLinhaStm(linha);
            }
            tamanho = 0;
            linha[tamanho++] = c;
        } else if (c == '\n') {
            if (tamanho > 0) {
                linha[tamanho] = '\0';
                processaLinhaStm(linha);
            }
            tamanho = 0;
        } else if (c != '\r' && tamanho > 0) {
            if (tamanho < sizeof(linha) - 1) {
                linha[tamanho++] = c;
            } else {
                tamanho = 0; // Descarta mensagens maiores que o buffer.
            }
        }
    }
}

static void atualizaDisplayGps()
{
    lv_label_set_text_fmt(objects.n_sat_1, "%d", GPS.numSats);
    lv_label_set_text_fmt(objects.n_sat_3, "%d", GPS.numSats);
    lv_label_set_text_fmt(objects.n_sat_5, "%d", GPS.numSats);

    char textoHdop[16];
    const float hdop = GPS.getHdop();
    if (GPS.getFixQuality() > 0 && isfinite(hdop) && hdop >= 0.0f && hdop < 100.0f) {
        const unsigned int centesimos = static_cast<unsigned int>(hdop * 100.0f + 0.5f);
        snprintf(textoHdop, sizeof(textoHdop), "HDOP:%u.%02u",
                 centesimos / 100U, centesimos % 100U);
    } else {
        snprintf(textoHdop, sizeof(textoHdop), "HDOP:--");
    }

    lv_label_set_text(objects.gps, textoHdop);
    lv_label_set_text(objects.gps_4, textoHdop);
    lv_label_set_text(objects.gps_5, textoHdop);
}

static void atualizaDadosDaPista(uint32_t agora)
{
    LapTelemetrySample sample;
    sample.timestampMs = agora;
    sample.positionSequence = GPS.getPositionSequence();
    sample.positionTimestampMs = GPS.getPositionTimestampMs();
    sample.gpsFix = GPS.getFixQuality() > 0 && GPS.hasFreshPosition(agora);
    sample.fixQuality = GPS.getFixQuality();
    sample.satellites = GPS.numSats;
    sample.latitude = GPS.getLat();
    sample.longitude = GPS.getLon();
    sample.altitude = GPS.getAlt();
    sample.speedKmh = GPS.getSpeedKmh();
    sample.hdop = GPS.getHdop();
    sample.roll = rollGraus;
    sample.pitch = pitchGraus;
    sample.yaw = yawGraus;
    sample.utcYear = GPS.getUtcYear();
    sample.utcMonth = GPS.getUtcMonth();
    sample.utcDay = GPS.getUtcDay();
    sample.utcHour = GPS.getUtcHour();
    sample.utcMinute = GPS.getUtcMin();
    sample.utcSecond = GPS.getUtcSec();
    lapData.update(sample);
}

static void textoTempoNoDisplay(lv_obj_t *label, const char *text, int width)
{
    // Use the existing fonts; long times must fit inside their display card.
    lv_point_t size;
    lv_text_get_size(&size, text, &lv_font_montserrat_34, 0, 0,
                     LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_style_text_font(label, size.x <= width ? &lv_font_montserrat_34
                                                     : &lv_font_montserrat_16, 0);
    lv_obj_set_width(label, width);
    lv_label_set_text(label, text);
}

static void atualizaDisplayVoltas()
{
    const LapDisplayData data = lapData.displayData();
    char best[24], gap[24];
    formatBestLap(best, sizeof(best), data.bestLapMs);
    formatLapGap(gap, sizeof(gap), data.gapValid, data.gapMs);
    textoTempoNoDisplay(objects.melhor_volta, best, 137);
    textoTempoNoDisplay(objects.gap, gap, 115);
    const uint32_t color = !data.gapValid || data.gapMs == 0 ? 0xd5d5d5
                          : data.gapMs < 0 ? 0x00ff28 : 0xff5252;
    lv_obj_set_style_text_color(objects.gap, lv_color_hex(color), 0);
}

// Conector TX/RX da CYD ESP32-2432S028R (P1).
// TX do GPS -> RX da CYD (GPIO3); GND comum. RX do GPS desconectado.
static constexpr int RX_GPS = 3;
// LED RGB da CYD: ativo em LOW. Vermelho usado no teste de recepcao.
static constexpr int LED_GPS = 4;
static uint32_t piscadasPendentes = 0;

static void sinalizaMensagemGps()
{
    ++piscadasPendentes;
}

static void atualizaLedGps()
{
    static bool aceso = false;
    static uint32_t ultimaTroca = 0;
    const uint32_t agora = millis();
    // Pulsos de 25 ms com intervalo apagado, sem bloquear a leitura da UART.
    if (agora - ultimaTroca < 25) return;
    if (aceso) {
        digitalWrite(LED_GPS, HIGH);
        aceso = false;
        ultimaTroca = agora;
    } else if (piscadasPendentes > 0) {
        --piscadasPendentes;
        digitalWrite(LED_GPS, LOW);
        aceso = true;
        ultimaTroca = agora;
    }

}

static int value = 0;
static lv_timer_t * Slidetimer = NULL;

uint16_t R_value = 0;
uint16_t G_value = 0;
uint16_t B_value = 0;

/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/
/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf [SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft = TFT_eSPI( screenWidth, screenHeight ); /* TFT instance */
/*Touch screen config*/
#define XPT2046_IRQ 36 //GPIO driver cảm ứng 
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass tsSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
//Run calib_touch files to get value 
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800; //Chạy Calibration để lấy giá trị mỗi màn hình mỗi khác

/* Display flushing */
void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    if (LV_COLOR_16_SWAP) {
        size_t len = lv_area_get_size( area );
        lv_draw_sw_rgb565_swap( pixelmap, len );
    }

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*) pixelmap, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*========== Read the encoder rotary ==========*/
void my_touch_read (lv_indev_t *indev_drv, lv_indev_data_t * data)
{
    if(ts.touched())
    {
        TS_Point p = ts.getPoint();
        //Some very basic auto calibration so it doesn't go out of range
        if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
        //Map this to the pixel position
        data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,screenWidth); /* Touchscreen X calibration */
        data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,screenHeight); /* Touchscreen Y calibration */
        data->state = LV_INDEV_STATE_PR;

    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

/*Set tick routine needed for LVGL internal timings*/
static uint32_t my_tick_get_cb (void) { return millis(); }

// // Hàm callback cho timer
// static void set_value_task(lv_timer_t * t) {

//     lv_bar_set_range(ui_Bar1, 0, 100);
//     value++;
//     lv_bar_set_value(ui_Bar1, value, LV_ANIM_ON); 

//     // Xóa timer khi đạt 100 và chuyển màn hình
//     if(value >= 100) {
//         lv_timer_del(Slidetimer);
//         Slidetimer = NULL;
//         _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_Screen3_screen_init);

//     }
// }


void setup()
{
    // Apaga os tres canais do LED RGB antes de iniciar o teste.
    for (int pino : {4, 16, 17}) {
        digitalWrite(pino, HIGH);
        pinMode(pino, OUTPUT);
    }
    // Diagnostico somente no TX0 (GPIO1), sem tomar o GPIO3 usado pelo GPS.
    Serial.begin(115200, SERIAL_8N1, -1, 1);
   // SerialSTM.setRxBufferSize(2048);
    SerialSTM.begin(BAUD_STM, SERIAL_8N1, RX_STM, TX_STM);
    GPS.IniciaSerialGps(RX_GPS);
    if (!lapData.begin())
        Serial.println("[LapData] Inicializacao do armazenamento falhou.");
    webServerInit();
    lv_init();

    tsSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(tsSpi);
    ts.setRotation(1);

    tft.begin();
    tft.setRotation(1);

    static lv_disp_t* disp;
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_buffers(disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);

    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touch_read);

    lv_tick_set_cb(my_tick_get_cb);

    ui_init();
    lv_label_set_text(objects.gps_7, "Wi-Fi: desligado");
    lv_label_set_text(objects.velocidade, "0");
    atualizaDisplayGps();
    lv_label_set_text(objects.angulo, "0\xC2\xB0");
    lv_arc_set_value(objects.roll, 0);
    lv_label_set_text(objects.tempo, "00:00:000");
    atualizaDisplayVoltas();


}

void loop()
{
    encaminhaSerialStm();
    GPS.RespostaGPS(sinalizaMensagemGps);
    atualizaLedGps();

    static uint32_t ultimaAtualizacao = 0;
    static uint32_t ultimaAtualizacaoTempo = 0;
    const uint32_t agora = millis();
    atualizaDadosDaPista(agora);

    // Calcula o tempo real decorrido e atualiza o texto a cada 20 ms.
    if (agora - ultimaAtualizacaoTempo >= 20) {
        ultimaAtualizacaoTempo = agora;
        const uint32_t decorrido = lapData.currentLapTimeMs();
        const unsigned long minutos = decorrido / 60000;
        const unsigned long segundos = (decorrido / 1000) % 60;
        const unsigned long milissegundos = decorrido % 1000;
        lv_label_set_text_fmt(objects.tempo, "%02lu:%02lu:%03lu",
                              minutos, segundos, milissegundos);
    }

    // Mostra a velocidade recebida em km/h, ou zero quando invalida.
    if (agora - ultimaAtualizacao >= 100) {
        ultimaAtualizacao = agora;
        const unsigned int velocidade = static_cast<unsigned int>(GPS.getSpeedKmh() + 0.5f);
        lv_label_set_text_fmt(objects.velocidade, "%u", velocidade);
        atualizaDisplayGps();
        atualizaDisplayVoltas();

        // Texto inteiro com sinal; o arco fica limitado a sua escala visual.
        const int rollInteiro = static_cast<int>(rollGraus);
        lv_label_set_text_fmt(objects.angulo, "%d\xC2\xB0", rollInteiro);
        lv_arc_set_value(objects.roll, constrain(rollInteiro, -90, 90));
    }

    lv_timer_handler();
    delay(5);

}
