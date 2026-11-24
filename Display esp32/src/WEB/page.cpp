#include "page.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "LapData.h"
#include "web_assets.h"

namespace
{
constexpr char AP_SSID[] = "RaceMetrics-LapTimer";
AsyncWebServer server(80);
bool routesRegistered = false;
bool webEnabled = false;

void sendJson(AsyncWebServerRequest *request, const String &json)
{
    request->send(200, "application/json; charset=utf-8", json);
}

void sendError(AsyncWebServerRequest *request, int status, const String &message)
{
    JsonDocument document;
    document["error"] = message;
    String body;
    serializeJson(document, body);
    request->send(status, "application/json; charset=utf-8", body);
}

bool formValue(AsyncWebServerRequest *request, const char *name, String &value)
{
    if (!request->hasParam(name, true))
        return false;
    value = request->getParam(name, true)->value();
    return true;
}

bool queryValue(AsyncWebServerRequest *request, const char *name, String &value)
{
    if (!request->hasParam(name))
        return false;
    value = request->getParam(name)->value();
    return true;
}

bool parseDouble(const String &value, double &result)
{
    if (value.isEmpty())
        return false;
    char *end = nullptr;
    result = strtod(value.c_str(), &end);
    return end != value.c_str() && *end == '\0' && isfinite(result);
}

bool parseInt(const String &value, int &result)
{
    if (value.isEmpty())
        return false;
    char *end = nullptr;
    const long parsed = strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX)
        return false;
    result = static_cast<int>(parsed);
    return true;
}

void handleTrackCreate(AsyncWebServerRequest *request)
{
    String name, latAString, lonAString, latBString, lonBString, id;
    double latA, lonA, latB, lonB;
    if (!formValue(request, "name", name) ||
        !formValue(request, "latA", latAString) ||
        !formValue(request, "lonA", lonAString) ||
        !formValue(request, "latB", latBString) ||
        !formValue(request, "lonB", lonBString) ||
        !parseDouble(latAString, latA) || !parseDouble(lonAString, lonA) ||
        !parseDouble(latBString, latB) || !parseDouble(lonBString, lonB))
    {
        sendError(request, 400, "Campos de pista ausentes ou invalidos.");
        return;
    }

    String savedId, error;
    if (!lapData.saveTrack(id, name, latA, lonA, latB, lonB, savedId, error))
    {
        sendError(request, 400, error);
        return;
    }

    JsonDocument response;
    response["id"] = savedId;
    response["name"] = name;
    response["latA"] = latA;
    response["lonA"] = lonA;
    response["latB"] = latB;
    response["lonB"] = lonB;
    String body;
    serializeJson(response, body);
    request->send(201, "application/json; charset=utf-8", body);
}

void handleTrackUpdate(AsyncWebServerRequest *request)
{
    String id, name, latAString, lonAString, latBString, lonBString;
    double latA, lonA, latB, lonB;
    if (!formValue(request, "id", id) ||
        !formValue(request, "name", name) ||
        !formValue(request, "latA", latAString) ||
        !formValue(request, "lonA", lonAString) ||
        !formValue(request, "latB", latBString) ||
        !formValue(request, "lonB", lonBString) ||
        !parseDouble(latAString, latA) || !parseDouble(lonAString, lonA) ||
        !parseDouble(latBString, latB) || !parseDouble(lonBString, lonB))
    {
        sendError(request, 400, "Campos de pista ausentes ou invalidos.");
        return;
    }

    String savedId, error;
    if (!lapData.saveTrack(id, name, latA, lonA, latB, lonB, savedId, error))
    {
        sendError(request, 400, error);
        return;
    }
    sendJson(request, "{\"ok\":true}");
}

void handleTrackDelete(AsyncWebServerRequest *request)
{
    String id, error;
    if (!formValue(request, "id", id))
    {
        sendError(request, 400, "ID da pista ausente.");
        return;
    }
    if (!lapData.deleteTrack(id, error))
    {
        sendError(request, 404, error);
        return;
    }
    sendJson(request, "{\"ok\":true}");
}

void handleSettingsUpdate(AsyncWebServerRequest *request)
{
    String deviceName, devicePassword;
    const bool hasPassword = formValue(request, "devicePassword", devicePassword);
    if (!formValue(request, "deviceName", deviceName))
    {
        sendError(request, 400, "Campos de configuracao ausentes ou invalidos.");
        return;
    }

    String error;
    if (!lapData.saveSettings(deviceName, hasPassword, devicePassword, error))
    {
        sendError(request, 400, error);
        return;
    }
    sendJson(request, "{\"ok\":true}");
}

void handleSessionStart(AsyncWebServerRequest *request)
{
    String activeTrack, minLapString, error;
    int minLapSeconds;
    if (!formValue(request, "activeTrack", activeTrack) ||
        !formValue(request, "minLapSeconds", minLapString) ||
        !parseInt(minLapString, minLapSeconds))
    {
        sendError(request, 400, "Pista ou intervalo entre voltas invalido.");
        return;
    }
    if (!lapData.beginRecording(activeTrack, minLapSeconds, error))
    {
        sendError(request, 409, error);
        return;
    }
    sendJson(request, lapData.statusJson());
}

void handleSessionStop(AsyncWebServerRequest *request)
{
    if (!lapData.endSession())
    {
        sendError(request, 500, "Gravacao parada, mas falhou ao finalizar a sessao. Tente Parar novamente.");
        return;
    }
    sendJson(request, lapData.statusJson());
}

void registerRoutes()
{
    if (routesRegistered)
        return;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html; charset=utf-8", WEB_INDEX_HTML);
    });
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html; charset=utf-8", WEB_INDEX_HTML);
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/javascript; charset=utf-8", WEB_APP_JS);
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/css; charset=utf-8", WEB_STYLE_CSS);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJson(request, lapData.statusJson());
    });
    server.on("/api/tracks", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJson(request, lapData.tracksJson());
    });
    // String routes also match child paths in ESPAsyncWebServer. Register
    // specific actions first so /update and /delete never create a track.
    server.on("/api/tracks/update", HTTP_POST, handleTrackUpdate);
    server.on("/api/tracks/delete", HTTP_POST, handleTrackDelete);
    server.on("/api/tracks", HTTP_POST, handleTrackCreate);
    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJson(request, lapData.settingsJson());
    });
    server.on("/api/settings", HTTP_POST, handleSettingsUpdate);
    server.on("/api/recording/start", HTTP_POST, handleSessionStart);
    server.on("/api/recording/stop", HTTP_POST, handleSessionStop);
    server.on("/api/sessions", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJson(request, lapData.sessionsJson());
    });
    server.on("/api/session", HTTP_GET, [](AsyncWebServerRequest *request) {
        String id, path, filename = "telemetry.csv";
        queryValue(request, "file", filename);
        if (filename != "telemetry.csv" && filename != "metadata.json" &&
            filename != "laps.json")
        {
            sendError(request, 400, "Arquivo de sessao invalido.");
            return;
        }
        if (!queryValue(request, "id", id) ||
            !lapData.getSessionFilePath(id, filename, path))
        {
            sendError(request, 404, "Sessao nao encontrada.");
            return;
        }
        request->send(LittleFS, path,
                      filename == "telemetry.csv" ? "text/csv; charset=utf-8"
                                                  : "application/json; charset=utf-8", true);
    });

    routesRegistered = true;
}
}

void webServerInit()
{
    registerRoutes();
    WiFi.mode(WIFI_OFF);
}

extern "C" int setLapTimerWebEnabled(int enabled)
{
    if (enabled)
    {
        if (webEnabled)
            return 1;
        if (!lapData.isReady())
            Serial.println("[Web] Armazenamento indisponivel; iniciando somente a interface.");

        if (!WiFi.mode(WIFI_AP))
        {
            Serial.println("[Web] Falha ao ativar o modo ponto de acesso.");
            WiFi.mode(WIFI_OFF);
            return 0;
        }
        const String password = lapData.devicePassword();
        const bool started = password.isEmpty()
                                 ? WiFi.softAP(AP_SSID)
                                 : WiFi.softAP(AP_SSID, password.c_str());
        if (!started)
        {
            Serial.println("[Web] Falha ao iniciar o ponto de acesso.");
            WiFi.mode(WIFI_OFF);
            return 0;
        }

        server.begin();
        webEnabled = true;
        Serial.printf("[Web] Acesse http://%s/\n",
                      WiFi.softAPIP().toString().c_str());
        return 1;
    }

    if (webEnabled)
        server.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    webEnabled = false;
    Serial.println("[Web] Ponto de acesso desligado.");
    return 1;
}
