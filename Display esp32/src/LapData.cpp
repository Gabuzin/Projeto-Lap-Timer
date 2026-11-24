#include "LapData.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <utility>

LapData lapData;

namespace
{
constexpr char TRACKS_PATH[] = "/config/pistas.json";
constexpr char SETTINGS_PATH[] = "/config/settings.json";
constexpr char LOGS_PATH[] = "/logs";
constexpr char TEST_TRACK_ID[] = "linha_teste";
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;
constexpr uint32_t METADATA_INTERVAL_MS = 30000;
constexpr char CSV_HEADER[] = "indice_amostra,timestamp_ms,timestamp_utc_iso,segundos_desde_inicio,volta_associada,tempo_volta_s,ultima_volta_s,latitude_decimal,longitude_decimal,altitude_m,velocidade_kmh,roll_deg,pitch_deg,yaw_deg,hdop,satelites,fix_quality,track_id\n";

class MutexGuard
{
public:
    explicit MutexGuard(SemaphoreHandle_t mutex) : _mutex(mutex)
    {
        if (_mutex != nullptr)
            xSemaphoreTake(_mutex, portMAX_DELAY);
    }

    ~MutexGuard()
    {
        if (_mutex != nullptr)
            xSemaphoreGive(_mutex);
    }

private:
    SemaphoreHandle_t _mutex;
};

double orientation(double ax, double ay, double bx, double by,
                   double cx, double cy)
{
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

bool withinSegment(double ax, double ay, double bx, double by,
                   double px, double py)
{
    constexpr double tolerance = 0.000001;
    return px >= min(ax, bx) - tolerance &&
           px <= max(ax, bx) + tolerance &&
           py >= min(ay, by) - tolerance &&
           py <= max(ay, by) + tolerance;
}

bool segmentsIntersect(double ax, double ay, double bx, double by,
                       double cx, double cy, double dx, double dy)
{
    const double o1 = orientation(ax, ay, bx, by, cx, cy);
    const double o2 = orientation(ax, ay, bx, by, dx, dy);
    const double o3 = orientation(cx, cy, dx, dy, ax, ay);
    const double o4 = orientation(cx, cy, dx, dy, bx, by);
    constexpr double tolerance = 0.000001;

    if (((o1 > tolerance && o2 < -tolerance) ||
         (o1 < -tolerance && o2 > tolerance)) &&
        ((o3 > tolerance && o4 < -tolerance) ||
         (o3 < -tolerance && o4 > tolerance)))
        return true;

    return (fabs(o1) <= tolerance && withinSegment(ax, ay, bx, by, cx, cy)) ||
           (fabs(o2) <= tolerance && withinSegment(ax, ay, bx, by, dx, dy)) ||
           (fabs(o3) <= tolerance && withinSegment(cx, cy, dx, dy, ax, ay)) ||
           (fabs(o4) <= tolerance && withinSegment(cx, cy, dx, dy, bx, by));
}

bool validCoordinate(double latitude, double longitude)
{
    return isfinite(latitude) && isfinite(longitude) &&
           latitude >= -90.0 && latitude <= 90.0 &&
           longitude >= -180.0 && longitude <= 180.0;
}

bool validId(const String &id)
{
    if (id.isEmpty() || id.length() > 40)
        return false;
    for (size_t i = 0; i < id.length(); ++i)
    {
        const char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}
}

bool LapData::begin()
{
    if (_mutex == nullptr)
        _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr)
    {
        Serial.println("[LapData] Falha ao criar mutex.");
        return false;
    }

    MutexGuard guard(_mutex);
    if (!LittleFS.begin(true))
    {
        Serial.println("[LapData] Falha ao montar LittleFS.");
        return false;
    }

    if (!LittleFS.exists("/config") && !LittleFS.mkdir("/config"))
    {
        Serial.println("[LapData] Falha ao criar /config.");
        return false;
    }
    if (!LittleFS.exists(LOGS_PATH) && !LittleFS.mkdir(LOGS_PATH))
    {
        Serial.println("[LapData] Falha ao criar /logs.");
        return false;
    }

    const bool firstTrackSetup = !LittleFS.exists(TRACKS_PATH);
    if (!loadOrCreate(_tracks, TRACKS_PATH, true) ||
        !loadOrCreate(_settings, SETTINGS_PATH, false))
        return false;

    const String activeTrack = _settings["activeTrack"] | "";
    if (firstTrackSetup)
    {
        if (!trackExists(TEST_TRACK_ID))
        {
            JsonObject track = _tracks["tracks"].as<JsonArray>().add<JsonObject>();
            track["id"] = TEST_TRACK_ID;
            track["name"] = "Linha de teste";
            track["latA"] = -21.146423;
            track["lonA"] = -48.946993;
            track["latB"] = -21.146390;
            track["lonB"] = -48.947040;
            if (!saveTracks())
            {
                Serial.println("[LapData] Falha ao criar a linha de teste.");
                return false;
            }
        }

        _settings["activeTrack"] = TEST_TRACK_ID;
        if (!saveSettingsFile())
        {
            Serial.println("[LapData] Falha ao ativar a linha de teste.");
            return false;
        }
    }

    else if (!activeTrack.isEmpty() && !trackExists(activeTrack))
    {
        _settings["activeTrack"] = "";
        if (!saveSettingsFile())
            return false;
    }

    recoverSessions();
    _ready = true;
    return true;
}

uint32_t LapData::currentLapTimeMs() const
{
    MutexGuard guard(_mutex);
    return _lap > 0 ? millis() - _lapStartMs : 0;
}

LapDisplayData LapData::displayData() const
{
    MutexGuard guard(_mutex);
    LapDisplayData result;
    result.bestLapMs = static_cast<uint32_t>(_bestLapSeconds * 1000.0f + 0.5f);
    result.gapValid = _recordingRequested && !_sessionWriteFailed && _live.gpsFix &&
        millis() - _live.positionTimestampMs <= 2500 && _gap.gapValid();
    result.gapMs = _gap.gapMs();
    return result;
}

bool LapData::loadOrCreate(JsonDocument &document, const char *path,
                           bool tracksFile)
{
    if (!LittleFS.exists(path))
    {
        document.clear();
        if (tracksFile)
            document["tracks"].to<JsonArray>();
        else
        {
            document["activeTrack"] = "";
            document["minLapSeconds"] = 15;
            document["deviceName"] = "RaceMetrics ESP32";
            document["devicePassword"] = "";
        }
        return saveDocument(document, path);
    }

    File file = LittleFS.open(path, FILE_READ);
    if (!file)
    {
        Serial.printf("[LapData] Nao foi possivel abrir %s.\n", path);
        return false;
    }

    const DeserializationError error = deserializeJson(document, file);
    file.close();
    if (error)
    {
        Serial.printf("[LapData] JSON invalido em %s: %s\n",
                      path, error.c_str());
        return false;
    }

    if (tracksFile && !document["tracks"].is<JsonArray>())
    {
        Serial.printf("[LapData] Campo tracks invalido em %s.\n", path);
        return false;
    }

    return true;
}

bool LapData::saveDocument(const JsonDocument &document, const char *path)
{
    if (document.overflowed())
        return false;
    const String tempPath = String(path) + ".tmp";
    File file = LittleFS.open(tempPath, FILE_WRITE);
    if (!file)
    {
        Serial.printf("[LapData] Nao foi possivel gravar %s.\n", tempPath.c_str());
        return false;
    }

    const size_t written = serializeJson(document, file);
    file.flush();
    file.close();
    if (written != measureJson(document))
    {
        LittleFS.remove(tempPath);
        Serial.printf("[LapData] Falha ao serializar %s.\n", path);
        return false;
    }

    // LittleFS rename replaces the destination atomically. Never unlink the
    // committed file first: a reset must leave either the old or the new JSON.
    if (!LittleFS.rename(tempPath, path))
    {
        LittleFS.remove(tempPath);
        Serial.printf("[LapData] Nao foi possivel renomear %s.\n", path);
        return false;
    }
    return true;
}

bool LapData::saveTracks()
{
    return saveDocument(_tracks, TRACKS_PATH);
}

bool LapData::saveSettingsFile()
{
    return saveDocument(_settings, SETTINGS_PATH);
}

bool LapData::trackExists(const String &id) const
{
    const JsonArrayConst tracks = _tracks["tracks"].as<JsonArrayConst>();
    for (const JsonObjectConst track : tracks)
        if (track["id"].as<String>() == id)
            return true;
    return false;
}

String LapData::trackName(const String &id) const
{
    const JsonArrayConst tracks = _tracks["tracks"].as<JsonArrayConst>();
    for (const JsonObjectConst track : tracks)
        if (track["id"].as<String>() == id)
            return track["name"].as<String>();
    return "Pista desconhecida";
}

String LapData::tracksJson() const
{
    MutexGuard guard(_mutex);
    String output;
    serializeJson(_tracks["tracks"], output);
    return output;
}

String LapData::settingsJson() const
{
    MutexGuard guard(_mutex);
    JsonDocument response;
    response["activeTrack"] = _settings["activeTrack"] | "";
    response["minLapSeconds"] = _settings["minLapSeconds"] | 15;
    response["deviceName"] = _settings["deviceName"] | "RaceMetrics ESP32";
    String output;
    serializeJson(response, output);
    return output;
}

String LapData::statusJson() const
{
    MutexGuard guard(_mutex);
    JsonDocument response;
    response["device"] = _settings["deviceName"] | "RaceMetrics ESP32";
    response["storageReady"] = _ready;
    response["sessionId"] = _sessionId;
    response["sessionWriteFailed"] = _sessionWriteFailed;
    response["recordingRequested"] = _recordingRequested;
    response["sessionState"] = !_ready ? "unavailable"
        : !_recordingRequested ? (_sessionId.isEmpty() ? "idle" : "stop_error")
        : _sessionWriteFailed ? "error"
        : _sessionId.isEmpty() ? "waiting_gps"
        : !_live.gpsFix ? "gps_lost" : "recording";
    response["gpsFix"] = _live.gpsFix;
    response["fixQuality"] = _live.fixQuality;
    response["speedKmh"] = _live.speedKmh;
    response["satellites"] = _live.satellites;
    response["hdop"] = _live.hdop;
    response["latitude"] = _live.latitude;
    response["longitude"] = _live.longitude;
    response["altitude"] = _live.altitude;
    response["roll"] = _live.roll;
    response["pitch"] = _live.pitch;
    response["yaw"] = _live.yaw;
    response["lap"] = _lap;
    response["lapTimeSeconds"] =
        _lap > 0 ? (millis() - _lapStartMs) / 1000.0f : 0.0f;
    response["lastLapSeconds"] = _lastLapSeconds;
    response["bestLapSeconds"] = _bestLapSeconds;
    if (_recordingRequested && !_sessionWriteFailed && _live.gpsFix &&
        millis() - _live.positionTimestampMs <= 2500 && _gap.gapValid())
        response["gapSeconds"] = _gap.gapMs() / 1000.0f;
    else
        response["gapSeconds"] = nullptr;
    const String activeTrack = _settings["activeTrack"] | "";
    response["track"] = activeTrack.isEmpty() ? "Sem pista" : trackName(activeTrack);
    String output;
    serializeJson(response, output);
    return output;
}

bool LapData::saveTrack(const String &id, const String &name,
                        double latA, double lonA, double latB, double lonB,
                        String &savedId, String &error)
{
    MutexGuard guard(_mutex);
    if (name.isEmpty() || name.length() > 70 ||
        !validCoordinate(latA, lonA) || !validCoordinate(latB, lonB) ||
        (latA == latB && lonA == lonB))
    {
        error = "Nome ou coordenadas da pista invalidos.";
        return false;
    }

    const bool update = !id.isEmpty();
    if (update && (!validId(id) || !trackExists(id)))
    {
        error = "Pista nao encontrada.";
        return false;
    }

    if (update && _settings["activeTrack"].as<String>() == id &&
        !resetLapState())
    {
        error = "Falha ao encerrar a sessao atual.";
        return false;
    }

    if (update)
    {
        JsonArray tracks = _tracks["tracks"].as<JsonArray>();
        for (JsonObject track : tracks)
        {
            if (track["id"].as<String>() == id)
            {
                track["name"] = name;
                track["latA"] = latA;
                track["lonA"] = lonA;
                track["latB"] = latB;
                track["lonB"] = lonB;
                break;
            }
        }
        savedId = id;
    }
    else
    {
        uint32_t seed = millis();
        do
        {
            savedId = "track_" + String(seed++, HEX);
        } while (trackExists(savedId));

        JsonObject track = _tracks["tracks"].as<JsonArray>().add<JsonObject>();
        track["id"] = savedId;
        track["name"] = name;
        track["latA"] = latA;
        track["lonA"] = lonA;
        track["latB"] = latB;
        track["lonB"] = lonB;
    }

    if (!saveTracks())
    {
        error = "Falha ao salvar config/pistas.json.";
        return false;
    }
    return true;
}

bool LapData::deleteTrack(const String &id, String &error)
{
    MutexGuard guard(_mutex);
    if (!validId(id) || !trackExists(id))
    {
        error = "Pista nao encontrada.";
        return false;
    }

    if (_settings["activeTrack"].as<String>() == id && !resetLapState())
    {
        error = "Falha ao encerrar a sessao atual.";
        return false;
    }

    JsonArray tracks = _tracks["tracks"].as<JsonArray>();
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        if (tracks[i]["id"].as<String>() == id)
        {
            tracks.remove(i);
            break;
        }
    }

    if (!saveTracks())
    {
        error = "Falha ao salvar config/pistas.json.";
        return false;
    }

    if (_settings["activeTrack"].as<String>() == id)
    {
        _settings["activeTrack"] = "";
        if (!saveSettingsFile())
        {
            error = "Pista removida, mas falhou ao limpar a configuracao ativa.";
            return false;
        }
    }
    return true;
}

bool LapData::saveSettings(const String &deviceName, bool hasPassword,
                           const String &devicePassword, String &error)
{
    MutexGuard guard(_mutex);
    if (!_ready)
    {
        error = "Armazenamento indisponivel.";
        return false;
    }
    if (deviceName.isEmpty() || deviceName.length() > 40)
    {
        error = "Configuracao invalida.";
        return false;
    }
    if (hasPassword && !devicePassword.isEmpty() &&
        (devicePassword.length() < 8 || devicePassword.length() > 63))
    {
        error = "A senha do Wi-Fi deve ter de 8 a 63 caracteres.";
        return false;
    }

    JsonDocument nextSettings(_settings);
    nextSettings["deviceName"] = deviceName;
    if (hasPassword)
        nextSettings["devicePassword"] = devicePassword;

    if (!saveDocument(nextSettings, SETTINGS_PATH))
    {
        error = "Falha ao salvar config/settings.json.";
        return false;
    }
    _settings = std::move(nextSettings);
    return true;
}

bool LapData::beginRecording(const String &activeTrack, int minLapSeconds, String &error)
{
    MutexGuard guard(_mutex);
    if (!_ready)
    {
        error = "Armazenamento indisponivel.";
        return false;
    }
    if (_recordingRequested || !_sessionId.isEmpty())
    {
        error = "Pare a sessao atual antes de iniciar outra.";
        return false;
    }
    if ((!activeTrack.isEmpty() && !trackExists(activeTrack)) ||
        minLapSeconds < 1 || minLapSeconds > 3600)
    {
        error = "Pista ou intervalo entre voltas invalido.";
        return false;
    }
    JsonDocument nextSettings(_settings);
    nextSettings["activeTrack"] = activeTrack;
    nextSettings["minLapSeconds"] = minLapSeconds;
    if (!saveDocument(nextSettings, SETTINGS_PATH))
    {
        error = "Falha ao salvar a configuracao da sessao.";
        return false;
    }
    _settings = std::move(nextSettings);
    resetLapState(); // No open session: resets counters before arming.
    _recordingRequested = true;
    return true;
}

String LapData::devicePassword() const
{
    MutexGuard guard(_mutex);
    return _settings["devicePassword"] | "";
}

bool LapData::endSession()
{
    MutexGuard guard(_mutex);
    return resetLapState(false);
}

bool LapData::resetLapState(bool clearSummary)
{
    // Stop accepting samples even if final metadata must be retried.
    _recordingRequested = false;
    if (!_sessionId.isEmpty() &&
        !saveSessionMetadata(_sessionWriteFailed ? "error" : "closed"))
        return false;
    _hasPreviousPosition = false;
    _lastPositionSequence = 0;
    _gap.reset();
    _lap = 0;
    _crossingDirection = 0;
    _lapStartMs = 0;
    _lastLapCrossingMs = 0;
    if (clearSummary)
    {
        _lastLapSeconds = 0.0f;
        _bestLapSeconds = 0.0f;
    }
    _sessionId = "";
    _sampleIndex = 0;
    _completedLaps = 0;
    _sessionMetadata.clear();
    _sessionWriteFailed = false;
    return true;
}

bool LapData::crossedFinishLine(double previousLat, double previousLon,
                                double currentLat, double currentLon,
                                const JsonObjectConst &track, int &direction, double &fraction) const
{
    const double latA = track["latA"].as<double>();
    const double lonA = track["lonA"].as<double>();
    const double latB = track["latB"].as<double>();
    const double lonB = track["lonB"].as<double>();
    const double refLatRad = (latA + latB) * 0.5 * 0.017453292519943295;
    constexpr double metersPerDegree = 111320.0;

    const double bx = (lonB - lonA) * metersPerDegree * cos(refLatRad);
    const double by = (latB - latA) * metersPerDegree;
    const double px1 = (previousLon - lonA) * metersPerDegree * cos(refLatRad);
    const double py1 = (previousLat - latA) * metersPerDegree;
    const double px2 = (currentLon - lonA) * metersPerDegree * cos(refLatRad);
    const double py2 = (currentLat - latA) * metersPerDegree;

    const double side1 = orientation(0.0, 0.0, bx, by, px1, py1);
    const double side2 = orientation(0.0, 0.0, bx, by, px2, py2);
    if ((side1 > 0.0 && side2 <= 0.0) ||
        (side1 < 0.0 && side2 >= 0.0))
    {
        const bool intersects = segmentsIntersect(
            px1, py1, px2, py2, 0.0, 0.0, bx, by);
        if (intersects)
        {
            direction = side2 > side1 ? 1 : -1;
            fraction = side1 / (side1 - side2);
        }
        return intersects;
    }
    return false;
}

void LapData::updateLap(const LapTelemetrySample &sample)
{
    // A GPS position is consumed once, not once per 5 ms UI loop.
    if (sample.positionSequence == _lastPositionSequence) return;
    _lastPositionSequence = sample.positionSequence;
    const String activeTrack = _settings["activeTrack"] | "";
    JsonObjectConst active;
    const JsonArrayConst tracks = _tracks["tracks"].as<JsonArrayConst>();
    for (const JsonObjectConst track : tracks)
        if (track["id"].as<String>() == activeTrack)
        {
            active = track;
            break;
        }

    if (active.isNull())
    {
        _hasPreviousPosition = false;
        return;
    }

    constexpr double metersPerDegree = 111320.0;
    const double originLat = active["latA"].as<double>();
    const double originLon = active["lonA"].as<double>();
    const double longitudeScale = metersPerDegree * cos(originLat * 0.017453292519943295);
    const float x = (sample.longitude - originLon) * longitudeScale;
    const float y = (sample.latitude - originLat) * metersPerDegree;
    const uint32_t positionMs = sample.positionTimestampMs;

    if (_hasPreviousPosition)
    {
        int direction = 0;
        double fraction = 0;
        if (crossedFinishLine(_previousLatitude, _previousLongitude,
                              sample.latitude, sample.longitude,
                              active, direction, fraction))
        {
            const uint32_t crossingMs = _previousPositionMs +
                static_cast<uint32_t>((positionMs - _previousPositionMs) * fraction);
            const float crossingX = (_previousLongitude +
                (sample.longitude - _previousLongitude) * fraction - originLon) * longitudeScale;
            const float crossingY = (_previousLatitude +
                (sample.latitude - _previousLatitude) * fraction - originLat) * metersPerDegree;
            const uint32_t minimumLapMs =
                static_cast<uint32_t>(_settings["minLapSeconds"] | 15) * 1000U;
            if (_lap == 0)
            {
                _lap = 1;
                _crossingDirection = direction;
                _lapStartMs = crossingMs;
                _lastLapCrossingMs = crossingMs;
                _gap.startLap(crossingX, crossingY);
            }
            else if (direction == _crossingDirection &&
                     crossingMs - _lastLapCrossingMs >= minimumLapMs)
            {
                const uint32_t durationMs = crossingMs - _lapStartMs;
                _gap.sample(crossingX, crossingY, durationMs, true);
                const bool newBest = _gap.lapValid() &&
                    (_bestLapSeconds == 0.0f || durationMs / 1000.0f < _bestLapSeconds);
                LapTelemetrySample crossing = sample;
                crossing.timestampMs = crossingMs;
                if (!_sessionId.isEmpty() && !_sessionWriteFailed &&
                    !appendLap(crossing))
                {
                    _sessionWriteFailed = true;
                    Serial.println("[LapData] Falha ao salvar laps.json.");
                }
                _lastLapSeconds = durationMs / 1000.0f;
                if (newBest)
                    _bestLapSeconds = _lastLapSeconds;
                _gap.finishLap(crossingX, crossingY, durationMs, newBest);
                ++_lap;
                _lapStartMs = crossingMs;
                _lastLapCrossingMs = crossingMs;
                _gap.startLap(crossingX, crossingY);
            }
        }
    }

    if (_lap > 0)
        _gap.sample(x, y, positionMs - _lapStartMs);

    _previousLatitude = sample.latitude;
    _previousLongitude = sample.longitude;
    _previousPositionMs = positionMs;
    _hasPreviousPosition = true;
}

String LapData::formatUtc(const LapTelemetrySample &sample) const
{
    if (sample.utcYear < 2000 || sample.utcMonth < 1 ||
        sample.utcMonth > 12 || sample.utcDay < 1 ||
        sample.utcDay > 31 || sample.utcHour < 0 ||
        sample.utcHour > 23 || sample.utcMinute < 0 ||
        sample.utcMinute > 59 || sample.utcSecond < 0 ||
        sample.utcSecond > 59)
        return "";

    char timestamp[25];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             sample.utcYear, sample.utcMonth, sample.utcDay,
             sample.utcHour, sample.utcMinute, sample.utcSecond);
    return timestamp;
}

String LapData::newSessionId() const
{
    // Reserve both layouts, including incomplete sessions, across reboots.
    for (uint32_t number = 1; number != 0; ++number)
    {
        char id[32];
        snprintf(id, sizeof(id), "session_%03lu", static_cast<unsigned long>(number));
        const String base = String(LOGS_PATH) + "/" + id;
        if (!LittleFS.exists(base) && !LittleFS.exists(base + ".csv") &&
            !LittleFS.exists(base + ".json"))
            return id;
    }
    return "";
}

String LapData::sessionFilePath(const char *filename) const
{
    return String(LOGS_PATH) + "/" + _sessionId + "/" + filename;
}

bool LapData::startSession(const LapTelemetrySample &sample)
{
    _sessionId = newSessionId();
    if (_sessionId.isEmpty())
        return false;
    const String directory = String(LOGS_PATH) + "/" + _sessionId;
    if (!LittleFS.mkdir(directory))
    {
        _sessionId = "";
        return false;
    }
    _sessionStartMs = sample.timestampMs;
    _sampleIndex = 0;
    _completedLaps = 0;
    _sessionWriteFailed = false;
    const String csvPath = sessionFilePath("telemetry.csv");
    File csv = LittleFS.open(csvPath, FILE_WRITE);
    const bool headerWritten = csv && csv.print(CSV_HEADER) == strlen(CSV_HEADER);
    if (csv)
    {
        csv.flush();
        csv.close();
    }

    _sessionMetadata.clear();
    JsonDocument &metadata = _sessionMetadata;
    metadata["schemaVersion"] = 1;
    metadata["telemetrySchemaVersion"] = 1;
    metadata["id"] = _sessionId;
    metadata["name"] = String(_settings["deviceName"] | "RaceMetrics ESP32") +
                       " " + _sessionId;
    const String activeTrack = _settings["activeTrack"] | "";
    metadata["trackId"] = activeTrack;
    metadata["track"] = activeTrack.isEmpty() ? "Sem pista" : trackName(activeTrack);
    // Snapshot the track so historical sessions survive edits/deletion.
    for (JsonObjectConst track : _tracks["tracks"].as<JsonArrayConst>())
        if (track["id"].as<String>() == activeTrack)
            metadata["trackSnapshot"].set(track);
    metadata["minLapSeconds"] = _settings["minLapSeconds"] | 15;
    metadata["date"] = formatUtc(sample);
    metadata["startedUtc"] = formatUtc(sample);
    metadata["endedUtc"] = nullptr;
    metadata["startedTimestampMs"] = sample.timestampMs;
    metadata["loggingIntervalMs"] = SAMPLE_INTERVAL_MS;
    metadata["telemetryFile"] = "telemetry.csv";
    metadata["lapsFile"] = "laps.json";

    JsonDocument laps;
    laps.to<JsonArray>();
    if (!headerWritten ||
        !saveDocument(laps, sessionFilePath("laps.json").c_str()) ||
        !saveSessionMetadata("recording"))
    {
        // Only these newly-created files belong to this failed attempt.
        LittleFS.remove(csvPath);
        LittleFS.remove(sessionFilePath("laps.json"));
        LittleFS.remove(sessionFilePath("metadata.json"));
        LittleFS.rmdir(directory);
        _sessionId = "";
        return false;
    }

    _lastMetadataMs = sample.timestampMs;
    _lastSampleMs = sample.timestampMs - SAMPLE_INTERVAL_MS;
    return true;
}

bool LapData::saveSessionMetadata(const char *state)
{
    _sessionMetadata["state"] = state;
    _sessionMetadata["sampleCount"] = _sampleIndex;
    _sessionMetadata["lapCount"] = _completedLaps;
    if (_bestLapSeconds > 0.0f)
        _sessionMetadata["bestLapTimeMs"] = static_cast<uint32_t>(_bestLapSeconds * 1000.0f + 0.5f);
    else
        _sessionMetadata["bestLapTimeMs"] = nullptr;
    _sessionMetadata["sampleCountExact"] = strcmp(state, "closed") == 0 && !_sessionWriteFailed;
    _sessionMetadata["checkpointTimestampMs"] = _live.timestampMs;
    if (strcmp(state, "closed") == 0 || strcmp(state, "error") == 0)
    {
        // The last observed UTC is not an invented wall-clock end time.
        _sessionMetadata["endedUtc"] = formatUtc(_live);
        _sessionMetadata["endedTimestampMs"] = _live.timestampMs;
    }
    const bool saved = saveDocument(_sessionMetadata, sessionFilePath("metadata.json").c_str());
    if (saved)
        _lastMetadataMs = _live.timestampMs;
    return saved;
}

bool LapData::appendLap(const LapTelemetrySample &sample)
{
    const String path = sessionFilePath("laps.json");
    const String tempPath = path + ".tmp";
    File previous = LittleFS.open(path, FILE_READ);
    if (!previous || previous.size() < 2 ||
        !previous.seek(previous.size() - 1) || previous.read() != ']' ||
        !previous.seek(0))
        return false;
    File next = LittleFS.open(tempPath, FILE_WRITE);
    if (!next)
        return false;

    // Copy the committed array without its closing bracket in bounded blocks.
    // Telemetry and even the lap history never need to fit in RAM.
    size_t remaining = previous.size() - 1;
    uint8_t buffer[512];
    bool ok = true;
    while (ok && remaining > 0)
    {
        const size_t count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const size_t read = previous.read(buffer, count);
        ok = read == count && next.write(buffer, count) == count;
        remaining -= read;
    }
    previous.close();
    if (ok && _completedLaps > 0)
        ok = next.print(',') == 1;

    JsonDocument lap;
    lap["number"] = _lap;
    lap["startTimestampMs"] = _lapStartMs;
    lap["endTimestampMs"] = sample.timestampMs;
    lap["durationMs"] = sample.timestampMs - _lapStartMs;
    lap["valid"] = _gap.lapValid();
    if (_bestLapSeconds > 0.0f)
        lap["gapToPreviousBestMs"] = static_cast<int64_t>(sample.timestampMs - _lapStartMs) -
                                    static_cast<int64_t>(_bestLapSeconds * 1000.0f + 0.5f);
    else
        lap["gapToPreviousBestMs"] = nullptr;
    ok = ok && !lap.overflowed() && serializeJson(lap, next) == measureJson(lap);
    ok = ok && next.print(']') == 1;
    next.flush();
    next.close();
    if (!ok || !LittleFS.rename(tempPath, path))
    {
        LittleFS.remove(tempPath);
        return false;
    }
    ++_completedLaps;
    return true;
}

void LapData::recoverSessions()
{
    File directory = LittleFS.open(LOGS_PATH);
    if (!directory || !directory.isDirectory())
        return;
    for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile())
    {
        if (!entry.isDirectory())
            continue;
        const String path = String(entry.path()) + "/metadata.json";
        entry.close();
        File file = LittleFS.open(path, FILE_READ);
        JsonDocument metadata;
        if (!file || deserializeJson(metadata, file))
            continue;
        file.close();
        if (metadata["state"] == "recording")
        {
            metadata["state"] = "interrupted";
            metadata["sampleCountExact"] = false;
            // Preserve the last checkpoint; the CSV may contain newer rows.
            if (!saveDocument(metadata, path.c_str()))
                Serial.printf("[LapData] Falha ao recuperar %s.\n", path.c_str());
        }
    }
}

bool LapData::appendSample(const LapTelemetrySample &sample)
{
    const String csvPath = sessionFilePath("telemetry.csv");
    File csv = LittleFS.open(csvPath, FILE_APPEND);
    if (!csv)
    {
        Serial.printf("[LapData] Falha ao abrir %s para append.\n", csvPath.c_str());
        return false;
    }

    const String utc = formatUtc(sample);
    const String activeTrack = _sessionMetadata["trackId"] | "";
    const float lapTime = _lap > 0
                              ? (sample.timestampMs - _lapStartMs) / 1000.0f
                              : 0.0f;
    String row;
    row.reserve(240);
    row += String(_sampleIndex);
    row += ','; row += String(sample.timestampMs);
    row += ','; row += utc;
    row += ','; row += String((sample.timestampMs - _sessionStartMs) / 1000.0f, 3);
    row += ','; row += String(_lap);
    row += ','; row += String(lapTime, 3);
    row += ','; row += String(_lastLapSeconds, 3);
    row += ','; row += String(sample.latitude, 7);
    row += ','; row += String(sample.longitude, 7);
    row += ','; row += String(sample.altitude, 2);
    row += ','; row += String(sample.speedKmh, 2);
    row += ','; row += String(sample.roll, 3);
    row += ','; row += String(sample.pitch, 3);
    row += ','; row += String(sample.yaw, 3);
    row += ','; row += String(sample.hdop, 2);
    row += ','; row += String(sample.satellites);
    row += ','; row += String(sample.fixQuality);
    row += ','; row += activeTrack;
    row += '\n';

    const size_t written = csv.print(row);
    csv.flush();
    csv.close();
    if (written != row.length())
    {
        Serial.printf("[LapData] Escrita incompleta em %s.\n", csvPath.c_str());
        return false;
    }
    ++_sampleIndex;
    _sessionMetadata["lastSampleTimestampMs"] = sample.timestampMs;
    return true;
}

void LapData::update(const LapTelemetrySample &sample)
{
    if (!_ready)
        return;
    MutexGuard guard(_mutex);
    _live = sample;

    if (!_recordingRequested || _sessionWriteFailed)
        return;

    if (!sample.gpsFix ||
        !validCoordinate(sample.latitude, sample.longitude))
    {
        _hasPreviousPosition = false;
        _gap.invalidateLap();
        return;
    }

    if (_sessionId.isEmpty() && !startSession(sample))
    {
        _sessionWriteFailed = true;
        Serial.println("[LapData] Falha ao iniciar a sessao. Pare e tente novamente.");
        return;
    }
    const uint32_t previousLapCount = _completedLaps;
    updateLap(sample);
    if (_sessionWriteFailed)
    {
        saveSessionMetadata("error");
        return;
    }

    if (sample.timestampMs - _lastSampleMs >= SAMPLE_INTERVAL_MS)
    {
        _lastSampleMs = sample.timestampMs;
        if (!appendSample(sample))
        {
            // Never append after a partial CSV row. Keep the file for recovery.
            _sessionWriteFailed = true;
            saveSessionMetadata("error");
            Serial.println("[LapData] A amostra de telemetria nao foi salva.");
            return;
        }
    }
    if (previousLapCount != _completedLaps ||
        sample.timestampMs - _lastMetadataMs >= METADATA_INTERVAL_MS)
    {
        if (!saveSessionMetadata("recording"))
        {
            _sessionWriteFailed = true;
            Serial.println("[LapData] Falha ao atualizar metadata.json.");
        }
    }
}

String LapData::sessionsJson() const
{
    MutexGuard guard(_mutex);
    JsonDocument response;
    JsonArray sessions = response.to<JsonArray>();
    File directory = LittleFS.open(LOGS_PATH);
    if (!directory || !directory.isDirectory())
    {
        Serial.println("[LapData] Nao foi possivel listar /logs.");
        return "[]";
    }

    File file = directory.openNextFile();
    while (file)
    {
        const String filename = file.name();
        const String entryPath = file.path();
        const bool isSessionDirectory = file.isDirectory();
        file.close();
        if (!isSessionDirectory && !filename.endsWith(".json"))
        {
            file = directory.openNextFile();
            continue;
        }

        const String metadataPath = isSessionDirectory
                                        ? entryPath + "/metadata.json"
                                        : entryPath;
        File metadataFile = LittleFS.open(metadataPath, FILE_READ);
        JsonDocument metadata;
        bool metadataValid = false;
        if (metadataFile)
        {
            metadataValid = !deserializeJson(metadata, metadataFile);
            metadataFile.close();
        }
        if (metadataValid)
        {
            const String id = metadata["id"] | "";
            const String base = String(LOGS_PATH) + "/" + id;
            // Only list a record when its ID matches its own filesystem path.
            if (validId(id) && entryPath == (isSessionDirectory ? base : base + ".json"))
            {
                const String csvPath = isSessionDirectory
                                           ? base + "/telemetry.csv" : base + ".csv";
                File csv = LittleFS.open(csvPath, FILE_READ);
                if (csv && !csv.isDirectory())
                {
                    JsonObject session = sessions.add<JsonObject>();
                    session["id"] = id;
                    session["name"] = metadata["name"] | id;
                    session["track"] = metadata["track"] | "Pista desconhecida";
                    session["date"] = metadata["date"] | "";
                    session["size"] = csv.size();
                    session["legacy"] = !isSessionDirectory;
                    session["hasLaps"] = isSessionDirectory && LittleFS.exists(base + "/laps.json");
                    session["state"] = id == _sessionId
                                           ? (_sessionWriteFailed ? "error" : "recording")
                                           : (metadata["state"] | "legacy");
                    session["sampleCount"] = id == _sessionId
                                                 ? _sampleIndex : (metadata["sampleCount"] | 0U);
                    session["lapCount"] = id == _sessionId
                                              ? _completedLaps : (metadata["lapCount"] | 0U);
                    csv.close();
                }
            }
        }
        else
            Serial.printf("[LapData] Metadata invalida: %s\n", filename.c_str());

        file = directory.openNextFile();
    }
    directory.close();

    String output;
    serializeJson(response, output);
    return output;
}

bool LapData::getSessionPath(const String &id, String &path) const
{
    return getSessionFilePath(id, "telemetry.csv", path);
}

bool LapData::getSessionFilePath(const String &id, const String &filename,
                                String &path) const
{
    MutexGuard guard(_mutex);
    path = "";
    if (!validId(id) || (filename != "telemetry.csv" &&
                        filename != "metadata.json" && filename != "laps.json"))
        return false;
    const String base = String(LOGS_PATH) + "/" + id;
    String candidate = base + "/" + filename;
    // Existing flat sessions stay readable without a destructive migration.
    if (!LittleFS.exists(base))
    {
        if (filename == "laps.json")
            return false;
        candidate = base + (filename == "telemetry.csv" ? ".csv" : ".json");
    }
    File file = LittleFS.open(candidate, FILE_READ);
    if (!file || file.isDirectory())
        return false;
    path = candidate;
    return true;
}
