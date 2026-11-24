#include "E108.h"
#include <math.h>
#include <stdlib.h>

E108 GPS;

// Construtor para inicializar buffers
E108::E108() : nmeaBufferPos(0) {
    memset(nmeaLineBuffer, 0, NMEA_BUFFER_SIZE);
}

int E108::getUtcDay() const { return StructDateTimeUtc.Dia; }
int E108::getUtcMonth() const { return StructDateTimeUtc.Mes; }
int E108::getUtcYear() const { return StructDateTimeUtc.Ano; }
int E108::getUtcHour() const { return StructDateTimeUtc.Hora; }
int E108::getUtcMin() const { return StructDateTimeUtc.Min; }
int E108::getUtcSec() const { return StructDateTimeUtc.S; }
bool E108::getRmcValid() const { return dt_valid == "A"; }
bool E108::hasValidSpeed() const {
    return speedValid && getRmcValid();
}
float E108::getSpeedKmh() const {
    return hasValidSpeed() ? speedKmh : 0.0f;
}
int E108::getFixQuality() const { return c_val; }
float E108::getLat() const { return Lat; }
float E108::getLon() const { return Long; }
float E108::getHdop() const { return Hdop; }
int E108::getAlt() const { return Alt; }

void E108::IniciaSerialGps(int RX_GPS)
{
    SerialGPS.begin(9600, SERIAL_8N1, RX_GPS, -1, false);
}


String E108::converteUTCparaBRT(const String &utcTime)
{
    if (utcTime.length() < 6)
        return "00:00:00"; // inválido

    int hora = utcTime.substring(0, 2).toInt();
    int minuto = utcTime.substring(2, 4).toInt();
    int segundo = utcTime.substring(4, 6).toInt();

    // Ajusta para o fuso horário de Brasília (UTC-3)
    hora -= 3;

    // Corrige para valores negativos ou acima de 24h
    if (hora < 0)
        hora += 24;
    if (hora >= 24)
        hora -= 24;

    // Formata como HH:MM:SS
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hora, minuto, segundo);
    return String(buffer);
}

bool E108::datetimeUTC(const String &utcDate, const String &utcTime)
{
    if (utcDate.length() < 6 || utcTime.length() < 6)
        return false; // incompleto

    // ddmmyy
    int dd = utcDate.substring(0, 2).toInt();
    int MM = utcDate.substring(2, 4).toInt();
    int yy = utcDate.substring(4, 6).toInt();
    // Converte para 4 dígitos (convenção NMEA):
    int YYYY = (yy >= 80) ? (1900 + yy) : (2000 + yy);

    // hhmmss(.sss opcional) -> pegamos só os 6 primeiros
    int hh = utcTime.substring(0, 2).toInt();
    int mm = utcTime.substring(2, 4).toInt();
    int ss = utcTime.substring(4, 6).toInt();

    // validações simples
    if (dd < 1 || dd > 31)
        return false;
    if (MM < 1 || MM > 12)
        return false;
    if (hh < 0 || hh > 23)
        return false;
    if (mm < 0 || mm > 59)
        return false;
    if (ss < 0 || ss > 59)
        return false;

    StructDateTimeUtc.Dia = dd;
    StructDateTimeUtc.Mes = MM;
    StructDateTimeUtc.Ano = YYYY; // <-- agora 4 dígitos
    StructDateTimeUtc.Hora = hh;
    StructDateTimeUtc.Min = mm;
    StructDateTimeUtc.S = ss;

    return true;
}

void E108::GNGGA(const String &nmea)
{
    if (!nmea.startsWith("$GNGGA"))
        return;

    // evita parse parcial
    int commas = 0;
    for (char ch : nmea)
        if (ch == ',')
            commas++;
    if (commas < 9)
        return;

    // separa campos
    int idx = 0, last = 0;
    String campos[15];
    for (int i = 0; i < 15; i++)
    {
        idx = nmea.indexOf(',', last);
        if (idx == -1)
            break;
        campos[i] = nmea.substring(last, idx);
        last = idx + 1;
    }

    String latStr = campos[2];
    String latHem = campos[3];
    String lonStr = campos[4];
    String lonHem = campos[5];
    if (latStr.length() < 4 || lonStr.length() < 5)
    {
        this->c_val = 0;
        return;
    }

    float latitude = latStr.substring(0, 2).toFloat() +
                     latStr.substring(2).toFloat() / 60.0;
    if (latHem == "S")
        latitude = -latitude;

    float longitude = lonStr.substring(0, 3).toFloat() +
                      lonStr.substring(3).toFloat() / 60.0;
    if (lonHem == "W")
        longitude = -longitude;

    int c_val = campos[6].toInt();
    int numSats = campos[7].toInt();
    float hdop = campos[8].toFloat();
    float alt = campos[9].toFloat();

    // atualiza atributos
    this->c_val = c_val;
    this->Lat = latitude;
    this->Alt = alt;
    this->Long = longitude;
    this->Hdop = hdop;
    this->numSats = numSats;
    positionTimestampMs = millis();
    ++positionSequence;
    if (positionSequence == 0) ++positionSequence;

}

bool E108::GNRMC(const String &nmea)
{
    if (!nmea.startsWith("$GNRMC,") && !nmea.startsWith("$GPRMC,"))
        return false;

    // evita parse de linha parcial (mín. de vírgulas)
    int commas = 0;
    for (char ch : nmea)
        if (ch == ',')
            commas++;
    if (commas < 9)
        return false;

    // split
    int idx = 0, last = 0;
    String f[15];
    for (int i = 0; i < 15; i++)
    {
        idx = nmea.indexOf(',', last);
        if (idx == -1)
            break;
        f[i] = nmea.substring(last, idx);
        last = idx + 1;
    }

    // Campos principais (indices padrão do RMC):
    // f[1]=hora (hhmmss.sss), f[2]=status(A/V), f[3]=lat, f[4]=N/S,
    // f[5]=lon, f[6]=E/W, f[7]=vel(knots), f[8]=curso, f[9]=data (ddmmyy)
    dt_valid = f[2]; // "A"=válido, "V"=inválido

    // RMC fornece velocidade sobre o solo em nos; armazena em km/h.
    char *end = nullptr;
    const float vel_kn = strtof(f[7].c_str(), &end);
    const float vel_kmh = vel_kn * 1.852f;
    speedValid = getRmcValid() && f[7].length() > 0 &&
                 end != f[7].c_str() && *end == '\0' &&
                 isfinite(vel_kmh) && vel_kn >= 0.0f;
    speedKmh = speedValid ? vel_kmh : 0.0f;
    float curso = f[8].toFloat();

    datetimeUTC(f[9], f[1]);
    Hora = converteUTCparaBRT(f[1]);
    Data = f[9];


    return (dt_valid == "A");
}

void E108::ProcessaLinha(const char *nmea)
{
    if (nmea == nullptr)
        return;

    String linha(nmea);
    if (linha.startsWith("$GNGGA"))
        GNGGA(linha);
    else if (linha.startsWith("$GNRMC,") || linha.startsWith("$GPRMC,"))
        GNRMC(linha);
}

String E108::RespostaGPS(void (*aoReceberLinha)())
{
    static char ultimaLinha[128] = "";

    while (SerialGPS.available())
    {
        char c = SerialGPS.read();
        if (c == '\r')
            continue; // trata CRLF

        // Proteção contra overflow
        if (nmeaBufferPos < NMEA_BUFFER_SIZE - 1) {
            nmeaLineBuffer[nmeaBufferPos++] = c;
        }

        if (c == '\n') // sentença completa
        {
            // Sinaliza recepcao mesmo sem fix ou sentenca reconhecida pelo parser.
            if (nmeaBufferPos > 1 && aoReceberLinha != nullptr)
                aoReceberLinha();

            nmeaLineBuffer[nmeaBufferPos] = '\0'; // Termina string

            ProcessaLinha(nmeaLineBuffer);

            // Copia para última linha (buffer fixo)
            strncpy(ultimaLinha, nmeaLineBuffer, sizeof(ultimaLinha) - 1);
            ultimaLinha[sizeof(ultimaLinha) - 1] = '\0';

            nmeaBufferPos = 0; // LIMPA para a próxima
        }
    }
    return String(ultimaLinha); // "" se nenhuma linha fechou
}
