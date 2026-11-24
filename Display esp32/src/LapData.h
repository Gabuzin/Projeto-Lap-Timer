#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "LapGap.h"

struct LapTelemetrySample
{
    uint32_t timestampMs = 0;
    uint32_t positionSequence = 0;
    uint32_t positionTimestampMs = 0;
    bool gpsFix = false;
    int fixQuality = 0;
    int satellites = 0;
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    float speedKmh = 0.0f;
    float hdop = 0.0f;
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    int utcYear = 0;
    int utcMonth = 0;
    int utcDay = 0;
    int utcHour = 0;
    int utcMinute = 0;
    int utcSecond = 0;
};

struct LapDisplayData
{
    uint32_t bestLapMs = 0;
    int32_t gapMs = 0;
    bool gapValid = false;
};

class LapData
{
public:
    bool begin();
    bool isReady() const { return _ready; }
    uint32_t currentLapTimeMs() const;
    LapDisplayData displayData() const;
    void update(const LapTelemetrySample &sample);

    String statusJson() const;
    String tracksJson() const;
    String settingsJson() const;
    String sessionsJson() const;
    String devicePassword() const;
    bool saveTrack(const String &id, const String &name,
                   double latA, double lonA, double latB, double lonB,
                   String &savedId, String &error);
    bool deleteTrack(const String &id, String &error);
    bool saveSettings(const String &deviceName, bool hasPassword,
                      const String &devicePassword, String &error);
    bool beginRecording(const String &activeTrack, int minLapSeconds, String &error);
    bool getSessionPath(const String &id, String &path) const;
    bool getSessionFilePath(const String &id, const String &filename,
                            String &path) const;
    // Stops recording until beginRecording() is explicitly requested again.
    bool endSession();

private:
    bool loadOrCreate(JsonDocument &document, const char *path,
                      bool tracksFile);
    bool saveDocument(const JsonDocument &document, const char *path);
    bool saveTracks();
    bool saveSettingsFile();
    bool trackExists(const String &id) const;
    String trackName(const String &id) const;
    bool resetLapState(bool clearSummary = true);
    void updateLap(const LapTelemetrySample &sample);
    bool crossedFinishLine(double previousLat, double previousLon,
                           double currentLat, double currentLon,
                           const JsonObjectConst &track, int &direction, double &fraction) const;
    bool startSession(const LapTelemetrySample &sample);
    bool appendSample(const LapTelemetrySample &sample);
    String formatUtc(const LapTelemetrySample &sample) const;
    String newSessionId() const;
    String sessionFilePath(const char *filename) const;
    bool saveSessionMetadata(const char *state);
    bool appendLap(const LapTelemetrySample &sample);
    void recoverSessions();

    JsonDocument _tracks;
    JsonDocument _settings;
    SemaphoreHandle_t _mutex = nullptr;
    bool _ready = false;
    bool _recordingRequested = false;

    LapTelemetrySample _live;
    bool _hasPreviousPosition = false;
    uint32_t _previousPositionMs = 0;
    uint32_t _lastPositionSequence = 0;
    LapGap _gap;
    double _previousLatitude = 0.0;
    double _previousLongitude = 0.0;
    int _lap = 0;
    int _crossingDirection = 0;
    uint32_t _lapStartMs = 0;
    uint32_t _lastLapCrossingMs = 0;
    uint32_t _lastSampleMs = 0;
    uint32_t _sessionStartMs = 0;
    uint32_t _sampleIndex = 0;
    float _lastLapSeconds = 0.0f;
    float _bestLapSeconds = 0.0f;
    String _sessionId;
    JsonDocument _sessionMetadata;
    uint32_t _completedLaps = 0;
    uint32_t _lastMetadataMs = 0;
    bool _sessionWriteFailed = false;
};

extern LapData lapData;
