#ifndef E108_H
#define E108_H

#include <Arduino.h>

//#include "includes.h"

// 🔽  Protótipos de tasks


struct UtcStamp
{
    int Ano, Mes, Dia, Hora, Min, S;
};

class E108
{
private:
    // Buffers estáticos para evitar fragmentação de memória
    static constexpr size_t NMEA_BUFFER_SIZE = 256;
    char nmeaLineBuffer[NMEA_BUFFER_SIZE];
    size_t nmeaBufferPos;
    float speedKmh = 0.0f;
    bool speedValid = false;
    uint32_t positionSequence = 0;
    uint32_t positionTimestampMs = 0;

    String converteUTCparaBRT(const String &utcTime);
    bool datetimeUTC(const String &utcDate, const String &utcTime);
    bool GNRMC(const String &nmea);
    void GNGGA(const String &nmea);


public:
    E108();  // Construtor para inicializar buffers
    HardwareSerial SerialGPS = HardwareSerial(1); // Usando UART1
    int c_val = 0;
    int numSats = 0;
    int Alt = 0;
    float Lat = 0;
    float Long = 0;
    float Hdop = 10;
    String Data;
    String Hora;

    // novos:
    bool hasLastFix = false;
    double lastLat = 0, lastLon = 0;
    int lastAlt = 0;
    String dt_valid;
    UtcStamp StructDateTimeUtc{0, 0, 0, 0, 0, 0};
    bool getRmcValid() const;  // A = true, V = false
    bool hasValidSpeed() const;
    float getSpeedKmh() const; // Retorna zero quando a velocidade esta indisponivel.
    int getFixQuality() const; // c_val (0=sem fix, 1=GPS, 2=DGPS, ...)
    uint32_t getPositionSequence() const { return positionSequence; }
    uint32_t getPositionTimestampMs() const { return positionTimestampMs; }
    bool hasFreshPosition(uint32_t now) const {
        return positionSequence != 0 && now - positionTimestampMs <= 2500;
    }
     int getAlt() const;
    float getLat() const;
    float getLon() const;
    float getHdop() const;
    int getUtcDay() const;
    int getUtcMonth() const;
    int getUtcYear() const; // 4 dígitos
    int getUtcHour() const;
    int getUtcMin() const;
    int getUtcSec() const;
    void IniciaSerialGps(int RX_GPS); // Apenas RX; TX desabilitado.
    void ProcessaLinha(const char *nmea);
    String RespostaGPS(void (*aoReceberLinha)() = nullptr);
};

extern E108 GPS;

#endif
