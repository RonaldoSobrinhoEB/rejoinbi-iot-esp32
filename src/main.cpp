#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <vector>

#include "icon_data.h"
#include "portal_page.h"
#include "root_ca.h"

namespace {

constexpr uint16_t DNS_PORT = 53;
constexpr uint8_t BOOT_BUTTON_PIN = 0;
constexpr char FIRMWARE_VERSION[] = "1.0";
constexpr uint32_t WIFI_RETRY_MS = 15000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t WIFI_FAILURE_GRACE_MS = 3000;
constexpr uint32_t FAST_BLINK_MS = 140;
constexpr uint32_t SLOW_BLINK_MS = 650;
constexpr uint32_t HTTP_TIMEOUT_MS = 15000;
constexpr uint32_t PORTAL_SHUTDOWN_DELAY_MS = 7000;
constexpr uint32_t RESTART_DELAY_MS = 1200;

struct DeviceConfig {
  String wifiSsid;
  String wifiPassword;
  String platformUrl;
  String sessionCookie;
  String userEmail;
  uint16_t intervalMinutes = 5;
  String ledMode = "single";
  int singlePin = 2;
  int redPin = 2;
  int greenPin = 4;
  int rgbPin = 48;
  bool activeLow = false;
  bool allowInsecureTls = false;
};

enum class MonitorState {
  Booting,
  ConfigMode,
  Connecting,
  WifiReady,
  Healthy,
  AuthFailed,
  Offline,
  PlatformError,
};

struct AuthResult {
  bool ok = false;
  bool requirePin = false;
  int httpCode = 0;
  String message;
};

Preferences preferences;
WebServer webServer(80);
DNSServer dnsServer;
Adafruit_NeoPixel rgbPixel(1, 48, NEO_GRB + NEO_KHZ800);
DeviceConfig config;

MonitorState monitorState = MonitorState::Booting;
String stateMessage = "Inicializando o dispositivo.";
String pendingEmail;
String pendingPassword;
bool pendingPin = false;
bool configMode = false;
bool routesConfigured = false;
bool clockReady = false;
bool rgbStarted = false;
bool wifiConnectPending = false;
uint32_t lastCheckMillis = 0;
uint32_t lastWifiAttemptMillis = 0;
uint32_t wifiConnectStartedAt = 0;
uint32_t lastButtonDownMillis = 0;
uint32_t portalShutdownAt = 0;
uint32_t restartAt = 0;
int lastHttpCode = 0;
uint32_t previousRgbColor = UINT32_MAX;

String stateKey() {
  switch (monitorState) {
    case MonitorState::ConfigMode: return "config";
    case MonitorState::Connecting: return "connecting";
    case MonitorState::WifiReady: return "wifi_ready";
    case MonitorState::Healthy: return "healthy";
    case MonitorState::AuthFailed: return "auth_failed";
    case MonitorState::Offline: return "offline";
    case MonitorState::PlatformError: return "platform_error";
    default: return "booting";
  }
}

String stateLabel() {
  switch (monitorState) {
    case MonitorState::ConfigMode: return "Aguardando configuração";
    case MonitorState::Connecting: return "Conectando";
    case MonitorState::WifiReady: return "Wi-Fi conectado";
    case MonitorState::Healthy: return "Plataforma disponível";
    case MonitorState::AuthFailed: return "Sessão inválida";
    case MonitorState::Offline: return "Sem conexão";
    case MonitorState::PlatformError: return "Plataforma indisponível";
    default: return "Inicializando";
  }
}

void setState(MonitorState nextState, const String &message) {
  monitorState = nextState;
  stateMessage = message;
  Serial.printf("[ESTADO] %s: %s\n", stateKey().c_str(), message.c_str());
}

void clearSensitiveString(String &value) {
  for (size_t index = 0; index < value.length(); ++index) {
    value.setCharAt(index, '\0');
  }
  value.remove(0);
}

bool elapsed(uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(millis() - since) >= interval;
}

uint16_t clampedInterval(int value) {
  return static_cast<uint16_t>(constrain(value, 1, 1440));
}

int clampedPin(int value, int fallback) {
  return value >= 0 && value <= 48 ? value : fallback;
}

String normalizedPlatformUrl(String value) {
  value.trim();
  while (value.endsWith("/")) value.remove(value.length() - 1);
  return value;
}

bool validPlatformUrl(const String &value) {
  if (!value.startsWith("https://") || value.length() < 12) return false;
  const int hostStart = 8;
  const int slash = value.indexOf('/', hostStart);
  return slash < 0;
}

void loadConfig() {
  preferences.begin("rejoin_iot", false);
  config.wifiSsid = preferences.isKey("ssid") ? preferences.getString("ssid", "") : "";
  config.wifiPassword = preferences.isKey("wifi_pass") ? preferences.getString("wifi_pass", "") : "";
  config.platformUrl = preferences.isKey("base_url") ? preferences.getString("base_url", "") : "";
  config.sessionCookie = preferences.isKey("session") ? preferences.getString("session", "") : "";
  config.userEmail = preferences.isKey("user_email") ? preferences.getString("user_email", "") : "";
  config.intervalMinutes = clampedInterval(preferences.getUShort("interval", 5));
  config.ledMode = preferences.isKey("led_mode") ? preferences.getString("led_mode", "single") : "single";
  config.singlePin = clampedPin(preferences.getInt("single_pin", 2), 2);
  config.redPin = clampedPin(preferences.getInt("red_pin", 2), 2);
  config.greenPin = clampedPin(preferences.getInt("green_pin", 4), 4);
  config.rgbPin = clampedPin(preferences.getInt("rgb_pin", 48), 48);
  config.activeLow = preferences.getBool("active_low", false);
  config.allowInsecureTls = preferences.getBool("tls_unsafe", false);
  if (config.ledMode != "single" && config.ledMode != "dual" && config.ledMode != "rgb") {
    config.ledMode = "single";
  }
}

void saveConfig() {
  preferences.putString("ssid", config.wifiSsid);
  preferences.putString("wifi_pass", config.wifiPassword);
  preferences.putString("base_url", config.platformUrl);
  preferences.putString("session", config.sessionCookie);
  preferences.putString("user_email", config.userEmail);
  preferences.putUShort("interval", config.intervalMinutes);
  preferences.putString("led_mode", config.ledMode);
  preferences.putInt("single_pin", config.singlePin);
  preferences.putInt("red_pin", config.redPin);
  preferences.putInt("green_pin", config.greenPin);
  preferences.putInt("rgb_pin", config.rgbPin);
  preferences.putBool("active_low", config.activeLow);
  preferences.putBool("tls_unsafe", config.allowInsecureTls);
}

void persistSession(const String &cookie, const String &email) {
  config.sessionCookie = cookie;
  config.userEmail = email;
  preferences.putString("session", config.sessionCookie);
  preferences.putString("user_email", config.userEmail);
}

void clearSession() {
  config.sessionCookie = "";
  config.userEmail = "";
  preferences.remove("session");
  preferences.remove("user_email");
}

void writeDigitalLed(int pin, bool on) {
  if (pin < 0 || pin > 48) return;
  digitalWrite(pin, config.activeLow ? !on : on);
}

void configureLeds() {
  rgbStarted = false;
  previousRgbColor = UINT32_MAX;
  if (config.ledMode == "rgb") {
    rgbPixel.setPin(config.rgbPin);
    rgbPixel.updateLength(1);
    rgbPixel.begin();
    rgbPixel.clear();
    rgbPixel.show();
    rgbStarted = true;
    return;
  }

  if (config.ledMode == "dual") {
    pinMode(config.redPin, OUTPUT);
    pinMode(config.greenPin, OUTPUT);
    writeDigitalLed(config.redPin, false);
    writeDigitalLed(config.greenPin, false);
    return;
  }

  pinMode(config.singlePin, OUTPUT);
  writeDigitalLed(config.singlePin, false);
}

void setRgb(uint8_t red, uint8_t green, uint8_t blue) {
  if (!rgbStarted) return;
  const uint32_t color = rgbPixel.Color(red, green, blue);
  if (color == previousRgbColor) return;
  previousRgbColor = color;
  rgbPixel.setPixelColor(0, color);
  rgbPixel.show();
}

bool isFailureState() {
  return monitorState == MonitorState::AuthFailed ||
         monitorState == MonitorState::Offline ||
         monitorState == MonitorState::PlatformError;
}

void updateLeds() {
  const bool fastPulse = (millis() / FAST_BLINK_MS) % 2 == 0;
  const bool slowPulse = (millis() / SLOW_BLINK_MS) % 2 == 0;
  const bool healthy = monitorState == MonitorState::Healthy;
  const bool failure = isFailureState();

  if (config.ledMode == "rgb") {
    if (healthy) setRgb(0, 72, 16);
    else if (failure) setRgb(fastPulse ? 110 : 0, 0, 0);
    else if (monitorState == MonitorState::ConfigMode) setRgb(0, 28, slowPulse ? 100 : 10);
    else setRgb(slowPulse ? 90 : 8, slowPulse ? 52 : 4, 0);
    return;
  }

  if (config.ledMode == "dual") {
    writeDigitalLed(config.greenPin, healthy || (!failure && slowPulse));
    writeDigitalLed(config.redPin, failure ? fastPulse : (monitorState == MonitorState::Connecting && slowPulse));
    return;
  }

  writeDigitalLed(config.singlePin, healthy ? true : (failure ? fastPulse : slowPulse));
}

String chipSuffix() {
  const uint64_t chip = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06llX", static_cast<unsigned long long>(chip & 0xFFFFFFULL));
  return String(suffix);
}

String accessPointName() {
  return "RejoinBI-IOT-" + chipSuffix();
}

String accessPointPassword() {
  const uint64_t chip = ESP.getEfuseMac();
  char value[13];
  snprintf(value, sizeof(value), "RJ%08llX", static_cast<unsigned long long>(chip & 0xFFFFFFFFULL));
  return String(value);
}

void addSecurityHeaders() {
  webServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  webServer.sendHeader("Pragma", "no-cache");
  webServer.sendHeader("X-Content-Type-Options", "nosniff");
  webServer.sendHeader("X-Frame-Options", "DENY");
  webServer.sendHeader(
    "Content-Security-Policy",
    "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; connect-src 'self'; img-src 'self'; frame-ancestors 'none'"
  );
}

void sendJson(JsonDocument &document, int status = 200) {
  String payload;
  serializeJson(document, payload);
  addSecurityHeaders();
  webServer.send(status, "application/json; charset=utf-8", payload);
}

void sendResult(bool ok, const String &message, int status = 200) {
  JsonDocument document;
  document["ok"] = ok;
  document["message"] = message;
  sendJson(document, status);
}

String lastCheckDescription() {
  if (lastCheckMillis == 0) return "ainda não realizada";
  const uint32_t seconds = static_cast<uint32_t>(millis() - lastCheckMillis) / 1000;
  if (seconds < 60) return "há " + String(seconds) + " segundo(s)";
  return "há " + String(seconds / 60) + " minuto(s)";
}

String wifiStatusDescription() {
  switch (WiFi.status()) {
    case WL_CONNECTED: return "conectado";
    case WL_NO_SSID_AVAIL: return "rede não encontrada";
    case WL_CONNECT_FAILED: return "falha de autenticação no Wi-Fi";
    case WL_CONNECTION_LOST: return "conexão perdida";
    case WL_DISCONNECTED: return "desconectado";
    case WL_IDLE_STATUS: return "aguardando conexão";
    default: return "estado desconhecido";
  }
}

void handleStatus() {
  JsonDocument document;
  document["ok"] = true;
  document["firmwareVersion"] = FIRMWARE_VERSION;
  document["state"] = stateKey();
  document["stateLabel"] = stateLabel();
  document["message"] = stateMessage;
  document["configMode"] = configMode;
  document["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  document["wifiConnectPending"] = wifiConnectPending;
  document["wifiStatus"] = wifiStatusDescription();
  document["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  document["wifiSsid"] = config.wifiSsid;
  document["platformUrl"] = config.platformUrl;
  document["intervalMinutes"] = config.intervalMinutes;
  document["ledMode"] = config.ledMode;
  document["singlePin"] = config.singlePin;
  document["redPin"] = config.redPin;
  document["greenPin"] = config.greenPin;
  document["rgbPin"] = config.rgbPin;
  document["activeLow"] = config.activeLow;
  document["allowInsecureTls"] = config.allowInsecureTls;
  document["userEmail"] = config.userEmail;
  document["authenticated"] = !config.sessionCookie.isEmpty();
  document["pendingPin"] = pendingPin;
  document["lastHttpCode"] = lastHttpCode;
  document["lastCheck"] = lastCheckDescription();
  sendJson(document);
}

void handleWifiScan() {
  Serial.println("[WIFI] Pesquisando redes de 2,4 GHz...");
  const int networkCount = WiFi.scanNetworks(false, true);
  if (networkCount < 0) {
    WiFi.scanDelete();
    sendResult(false, "Não foi possível pesquisar as redes. Tente novamente em alguns segundos.", 503);
    return;
  }

  JsonDocument document;
  document["ok"] = true;
  JsonArray networks = document["networks"].to<JsonArray>();
  std::vector<String> seenSsids;
  seenSsids.reserve(networkCount);

  for (int index = 0; index < networkCount && networks.size() < 24; ++index) {
    const String ssid = WiFi.SSID(index);
    if (ssid.isEmpty()) continue;

    bool duplicated = false;
    for (const String &seen : seenSsids) {
      if (seen == ssid) {
        duplicated = true;
        break;
      }
    }
    if (duplicated) continue;
    seenSsids.push_back(ssid);

    JsonObject network = networks.add<JsonObject>();
    const int rssi = WiFi.RSSI(index);
    network["ssid"] = ssid;
    network["rssi"] = rssi;
    network["quality"] = constrain(2 * (rssi + 100), 0, 100);
    network["secure"] = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    network["channel"] = WiFi.channel(index);
  }

  document["message"] = String(networks.size()) + " rede(s) compatível(is) encontrada(s).";
  WiFi.scanDelete();
  Serial.printf("[WIFI] %u rede(s) compatível(is) encontrada(s).\n", static_cast<unsigned int>(networks.size()));
  sendJson(document);
}

bool ensureClock() {
  if (config.allowInsecureTls) return true;
  if (clockReady && time(nullptr) > 1700000000) return true;

  setState(MonitorState::Connecting, "Sincronizando o relógio para validar o HTTPS.");
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  const uint32_t startedAt = millis();
  while (!elapsed(startedAt, 12000)) {
    updateLeds();
    if (time(nullptr) > 1700000000) {
      clockReady = true;
      return true;
    }
    delay(100);
  }
  return false;
}

bool configureTls(WiFiClientSecure &client) {
  client.setTimeout(HTTP_TIMEOUT_MS);
  if (config.allowInsecureTls) {
    client.setInsecure();
    return true;
  }
  if (!ensureClock()) return false;
  client.setCACert(REJOINBI_ROOT_CA);
  return true;
}

String extractSessionCookie(const String &setCookieHeader) {
  if (setCookieHeader.isEmpty()) return "";
  int start = setCookieHeader.indexOf("plataforma_session=");
  if (start < 0) return "";
  int end = setCookieHeader.indexOf(';', start);
  if (end < 0) end = setCookieHeader.length();
  return setCookieHeader.substring(start, end);
}

String apiMessage(JsonDocument &document, const String &fallback) {
  if (document["message"].is<const char *>()) return String(document["message"].as<const char *>());
  if (document["error"].is<const char *>()) return String(document["error"].as<const char *>());
  return fallback;
}

AuthResult authenticatePlatform(const String &email, const String &password, const String &pin) {
  AuthResult result;
  if (WiFi.status() != WL_CONNECTED) {
    result.message = "O ESP32 ainda não está conectado ao Wi-Fi.";
    return result;
  }
  if (!validPlatformUrl(config.platformUrl)) {
    result.message = "Configure um endereço HTTPS válido da plataforma.";
    return result;
  }

  setState(MonitorState::Connecting, "Autenticando na plataforma.");
  updateLeds();

  WiFiClientSecure client;
  if (!configureTls(client)) {
    result.message = "Não foi possível sincronizar o relógio para validar o certificado HTTPS.";
    setState(MonitorState::PlatformError, result.message);
    return result;
  }

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  const String url = config.platformUrl + "/plataforma/api/login";
  if (!http.begin(client, url)) {
    result.message = "Não foi possível iniciar a conexão HTTPS.";
    setState(MonitorState::PlatformError, result.message);
    return result;
  }

  const char *headerKeys[] = {"Set-Cookie"};
  http.collectHeaders(headerKeys, 1);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "RejoinBI-ESP32-Monitor/1.0");

  JsonDocument requestDocument;
  requestDocument["email"] = email;
  requestDocument["password"] = password;
  requestDocument["lang"] = "pt-BR";
  if (!pin.isEmpty()) requestDocument["pin"] = pin;
  String body;
  serializeJson(requestDocument, body);

  result.httpCode = http.POST(body);
  lastHttpCode = result.httpCode;
  const String responseBody = http.getString();
  const String newCookie = extractSessionCookie(http.header("Set-Cookie"));
  http.end();
  clearSensitiveString(body);

  if (result.httpCode <= 0) {
    result.message = "Falha de rede ao acessar a plataforma.";
    setState(MonitorState::PlatformError, result.message);
    return result;
  }

  JsonDocument responseDocument;
  const DeserializationError jsonError = deserializeJson(responseDocument, responseBody);
  if (jsonError) {
    result.message = "A plataforma respondeu em um formato inesperado.";
    setState(MonitorState::PlatformError, result.message);
    return result;
  }

  result.requirePin = responseDocument["require_pin"] | false;
  const bool apiSuccess = responseDocument["success"] | false;
  result.message = apiMessage(responseDocument, apiSuccess ? "Autenticação concluída." : "Não foi possível autenticar.");

  if (result.httpCode >= 200 && result.httpCode < 300 && apiSuccess && result.requirePin) {
    pendingPin = true;
    setState(MonitorState::ConfigMode, result.message);
    return result;
  }

  if (result.httpCode >= 200 && result.httpCode < 300 && apiSuccess && !newCookie.isEmpty()) {
    persistSession(newCookie, email);
    pendingPin = false;
    result.ok = true;
    lastCheckMillis = millis();
    setState(MonitorState::Healthy, "Login confirmado. Monitoramento ativo.");
    return result;
  }

  if (result.httpCode == 401 || result.httpCode == 403 || result.httpCode == 429) {
    setState(MonitorState::AuthFailed, result.message);
  } else {
    setState(MonitorState::PlatformError, result.message);
  }
  return result;
}

bool checkPlatformSession(String &message) {
  if (WiFi.status() != WL_CONNECTED) {
    message = "Wi-Fi desconectado.";
    setState(MonitorState::Offline, message);
    return false;
  }
  if (config.sessionCookie.isEmpty()) {
    message = "Faça login para criar uma sessão de monitoramento.";
    setState(MonitorState::AuthFailed, message);
    return false;
  }

  setState(MonitorState::Connecting, "Verificando sessão e disponibilidade.");
  updateLeds();
  WiFiClientSecure client;
  if (!configureTls(client)) {
    message = "Falha ao validar o certificado HTTPS.";
    setState(MonitorState::PlatformError, message);
    return false;
  }

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(HTTP_TIMEOUT_MS);
  const String url = config.platformUrl + "/plataforma/api/check-session";
  if (!http.begin(client, url)) {
    message = "Não foi possível iniciar a verificação HTTPS.";
    setState(MonitorState::PlatformError, message);
    return false;
  }

  const char *headerKeys[] = {"Set-Cookie"};
  http.collectHeaders(headerKeys, 1);
  http.addHeader("Accept", "application/json");
  http.addHeader("Cookie", config.sessionCookie);
  http.addHeader("User-Agent", "RejoinBI-ESP32-Monitor/1.0");
  lastHttpCode = http.GET();
  const String responseBody = http.getString();
  const String refreshedCookie = extractSessionCookie(http.header("Set-Cookie"));
  http.end();
  lastCheckMillis = millis();

  if (!refreshedCookie.isEmpty() && refreshedCookie != config.sessionCookie) {
    persistSession(refreshedCookie, config.userEmail);
  }

  if (lastHttpCode <= 0) {
    message = "A plataforma não respondeu à conexão.";
    setState(MonitorState::PlatformError, message);
    return false;
  }

  JsonDocument responseDocument;
  const DeserializationError jsonError = deserializeJson(responseDocument, responseBody);
  const bool sessionValid = !jsonError && lastHttpCode == 200 && (responseDocument["success"] | false);
  if (sessionValid) {
    message = "Sessão válida e plataforma respondendo normalmente.";
    setState(MonitorState::Healthy, message);
    return true;
  }

  if (lastHttpCode == 401 || lastHttpCode == 302 || lastHttpCode == 403) {
    message = apiMessage(responseDocument, "A sessão expirou. Faça login novamente.");
    clearSession();
    setState(MonitorState::AuthFailed, message);
  } else {
    message = apiMessage(responseDocument, "A plataforma respondeu com erro HTTP " + String(lastHttpCode) + ".");
    setState(MonitorState::PlatformError, message);
  }
  return false;
}

void connectToWifi() {
  if (config.wifiSsid.isEmpty()) return;
  setState(MonitorState::Connecting, "Conectando à rede Wi-Fi.");
  wifiConnectPending = true;
  wifiConnectStartedAt = millis();
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  lastWifiAttemptMillis = millis();
}

void finishWifiAttemptWithError(const String &message) {
  wifiConnectPending = false;
  wifiConnectStartedAt = 0;
  // Mantém o erro visível antes de uma nova tentativa automática.
  lastWifiAttemptMillis = millis();
  setState(MonitorState::Offline, message);
}

void updateWifiConnectionState() {
  if (!wifiConnectPending) return;

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiConnectPending = false;
    wifiConnectStartedAt = 0;
    const String ip = WiFi.localIP().toString();
    setState(
      MonitorState::WifiReady,
      "Wi-Fi conectado com sucesso. IP: " + ip + ". Agora você já pode entrar na plataforma."
    );
    return;
  }

  if (!elapsed(wifiConnectStartedAt, WIFI_FAILURE_GRACE_MS)) return;

  if (status == WL_NO_SSID_AVAIL) {
    finishWifiAttemptWithError("Rede Wi-Fi não encontrada. Confirme o nome e use uma rede de 2,4 GHz.");
    return;
  }
  if (status == WL_CONNECT_FAILED) {
    finishWifiAttemptWithError("Não foi possível autenticar no Wi-Fi. Confira a senha da rede e tente novamente.");
    return;
  }
  if (elapsed(wifiConnectStartedAt, WIFI_CONNECT_TIMEOUT_MS)) {
    finishWifiAttemptWithError("O tempo para conectar ao Wi-Fi acabou. Confira a senha, aproxime o ESP32 do roteador e tente novamente.");
  }
}

void handleConfigSave() {
  String ssid = webServer.arg("ssid");
  String platformUrl = normalizedPlatformUrl(webServer.arg("platformUrl"));
  ssid.trim();
  if (ssid.isEmpty()) {
    sendResult(false, "Informe o nome da rede Wi-Fi.", 400);
    return;
  }
  if (!validPlatformUrl(platformUrl)) {
    sendResult(false, "Use somente o endereço raiz HTTPS, por exemplo https://empresa.rejoinbi.com.br.", 400);
    return;
  }

  const String newLedMode = webServer.arg("ledMode");
  if (newLedMode != "single" && newLedMode != "dual" && newLedMode != "rgb") {
    sendResult(false, "Selecione um tipo de LED válido.", 400);
    return;
  }

  const int redPin = clampedPin(webServer.arg("redPin").toInt(), config.redPin);
  const int greenPin = clampedPin(webServer.arg("greenPin").toInt(), config.greenPin);
  if (newLedMode == "dual" && redPin == greenPin) {
    sendResult(false, "Os LEDs vermelho e verde precisam usar GPIOs diferentes.", 400);
    return;
  }

  const bool platformChanged = platformUrl != config.platformUrl;
  config.wifiSsid = ssid;
  const String submittedWifiPassword = webServer.arg("wifiPassword");
  if (!submittedWifiPassword.isEmpty()) config.wifiPassword = submittedWifiPassword;
  config.platformUrl = platformUrl;
  config.intervalMinutes = clampedInterval(webServer.arg("intervalMinutes").toInt());
  config.ledMode = newLedMode;
  config.singlePin = clampedPin(webServer.arg("singlePin").toInt(), config.singlePin);
  config.redPin = redPin;
  config.greenPin = greenPin;
  config.rgbPin = clampedPin(webServer.arg("rgbPin").toInt(), config.rgbPin);
  config.activeLow = webServer.hasArg("activeLow");
  config.allowInsecureTls = webServer.hasArg("allowInsecureTls");
  if (platformChanged) clearSession();
  saveConfig();
  configureLeds();

  WiFi.disconnect(false, false);
  delay(100);
  connectToWifi();
  sendResult(true, "Configuração salva. Testando a conexão com o Wi-Fi...");
}

void sendAuthResult(const AuthResult &result) {
  JsonDocument document;
  document["ok"] = result.ok || result.requirePin;
  document["requirePin"] = result.requirePin;
  document["message"] = result.message;
  document["httpCode"] = result.httpCode;
  int status = 200;
  if (!result.ok && !result.requirePin) {
    status = result.httpCode >= 400 && result.httpCode < 600 ? result.httpCode : 503;
  }
  sendJson(document, status);
}

void handleLogin() {
  String email = webServer.arg("email");
  String password = webServer.arg("password");
  email.trim();
  email.toLowerCase();
  if (email.isEmpty() || password.isEmpty()) {
    clearSensitiveString(password);
    sendResult(false, "Informe e-mail e senha.", 400);
    return;
  }

  clearSensitiveString(pendingPassword);
  pendingEmail = email;
  pendingPassword = password;
  clearSensitiveString(password);
  clearSession();
  const AuthResult result = authenticatePlatform(pendingEmail, pendingPassword, "");
  sendAuthResult(result);
  if (result.ok) {
    clearSensitiveString(pendingPassword);
    pendingEmail = "";
    portalShutdownAt = millis() + PORTAL_SHUTDOWN_DELAY_MS;
  }
}

void handlePin() {
  String pin = webServer.arg("pin");
  pin.trim();
  if (!pendingPin || pendingEmail.isEmpty() || pendingPassword.isEmpty()) {
    clearSensitiveString(pin);
    sendResult(false, "O desafio de PIN não está ativo. Refaça o login.", 409);
    return;
  }
  if (pin.length() != 6) {
    clearSensitiveString(pin);
    sendResult(false, "O PIN precisa ter seis números.", 400);
    return;
  }

  const AuthResult result = authenticatePlatform(pendingEmail, pendingPassword, pin);
  clearSensitiveString(pin);
  sendAuthResult(result);
  if (result.ok) {
    clearSensitiveString(pendingPassword);
    pendingEmail = "";
    pendingPin = false;
    portalShutdownAt = millis() + PORTAL_SHUTDOWN_DELAY_MS;
  }
}

void handleManualCheck() {
  String message;
  const bool ok = checkPlatformSession(message);
  sendResult(ok, message, ok ? 200 : (lastHttpCode == 401 ? 401 : 503));
}

void handleReset() {
  if (webServer.arg("confirm") != "APAGAR") {
    sendResult(false, "Confirmação inválida.", 400);
    return;
  }
  clearSensitiveString(pendingPassword);
  preferences.clear();
  sendResult(true, "Configuração removida. Reiniciando.");
  restartAt = millis() + RESTART_DELAY_MS;
}

void configureRoutes() {
  if (routesConfigured) return;
  routesConfigured = true;

  webServer.on("/", HTTP_GET, []() {
    addSecurityHeaders();
    webServer.send_P(200, "text/html; charset=utf-8", CONFIG_PORTAL_HTML);
  });
  webServer.on("/icon.png", HTTP_GET, []() {
    webServer.sendHeader("Cache-Control", "public, max-age=86400");
    webServer.sendHeader("X-Content-Type-Options", "nosniff");
    webServer.send_P(
      200,
      "image/png",
      reinterpret_cast<const char *>(REJOINBI_ICON_PNG),
      REJOINBI_ICON_PNG_LEN
    );
  });
  webServer.on("/api/status", HTTP_GET, handleStatus);
  webServer.on("/api/wifi-scan", HTTP_GET, handleWifiScan);
  webServer.on("/api/config", HTTP_POST, handleConfigSave);
  webServer.on("/api/login", HTTP_POST, handleLogin);
  webServer.on("/api/pin", HTTP_POST, handlePin);
  webServer.on("/api/check", HTTP_POST, handleManualCheck);
  webServer.on("/api/reset", HTTP_POST, handleReset);
  webServer.on("/api/restart", HTTP_POST, []() {
    sendResult(true, "Reiniciando o dispositivo.");
    restartAt = millis() + RESTART_DELAY_MS;
  });

  auto captiveRedirect = []() {
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
  };
  webServer.on("/generate_204", HTTP_ANY, captiveRedirect);
  webServer.on("/hotspot-detect.html", HTTP_ANY, captiveRedirect);
  webServer.on("/fwlink", HTTP_ANY, captiveRedirect);
  webServer.onNotFound([]() {
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
  });
}

void startConfigPortal(const String &reason) {
  if (configMode) return;
  configMode = true;
  portalShutdownAt = 0;
  WiFi.mode(WIFI_AP_STA);
  const String ssid = accessPointName();
  const String password = accessPointPassword();
  WiFi.softAP(ssid.c_str(), password.c_str());
  delay(100);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  configureRoutes();
  webServer.begin();
  if (isFailureState()) {
    setState(monitorState, stateMessage);
  } else if (WiFi.status() == WL_CONNECTED && config.sessionCookie.isEmpty()) {
    const String ip = WiFi.localIP().toString();
    setState(MonitorState::WifiReady, "Wi-Fi conectado com sucesso. IP: " + ip + ". " + reason);
  } else {
    setState(MonitorState::ConfigMode, reason);
  }

  Serial.println();
  Serial.println("========== PORTAL DE CONFIGURAÇÃO ==========");
  Serial.printf("Rede Wi-Fi : %s\n", ssid.c_str());
  Serial.printf("Senha      : %s\n", password.c_str());
  Serial.printf("Endereço   : http://%s\n", WiFi.softAPIP().toString().c_str());
  Serial.println("=============================================");
}

void stopConfigPortal() {
  if (!configMode) return;
  dnsServer.stop();
  webServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  configMode = false;
  portalShutdownAt = 0;
  Serial.println("[PORTAL] Configuração encerrada. Monitoramento protegido em execução.");
}

void monitorBootButton() {
  const bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;
  if (!pressed) {
    lastButtonDownMillis = 0;
    return;
  }
  if (lastButtonDownMillis == 0) lastButtonDownMillis = millis();
  if (!configMode && elapsed(lastButtonDownMillis, 3000)) {
    startConfigPortal("Modo de configuração ativado pelo botão BOOT.");
    lastButtonDownMillis = millis();
  }
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED || config.wifiSsid.isEmpty() || wifiConnectPending) return;
  if (lastWifiAttemptMillis == 0 || elapsed(lastWifiAttemptMillis, WIFI_RETRY_MS)) {
    setState(MonitorState::Offline, "Wi-Fi desconectado. Tentando reconectar.");
    connectToWifi();
  }
}

bool monitoringCheckDue() {
  if (WiFi.status() != WL_CONNECTED || config.sessionCookie.isEmpty()) return false;
  if (lastCheckMillis == 0) return true;
  const uint32_t intervalMs = static_cast<uint32_t>(config.intervalMinutes) * 60000UL;
  return elapsed(lastCheckMillis, intervalMs);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(350);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  loadConfig();
  configureLeds();
  updateLeds();

  Serial.println();
  Serial.printf("Rejoin BI ESP32 Monitor %s\n", FIRMWARE_VERSION);
  Serial.printf("Chip: %s | Revisão: %d | Flash: %u bytes\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getFlashChipSize());

  if (config.wifiSsid.isEmpty() || config.platformUrl.isEmpty()) {
    startConfigPortal("Primeira configuração necessária.");
    return;
  }

  WiFi.mode(WIFI_STA);
  connectToWifi();
  const uint32_t wifiStartedAt = millis();
  while (WiFi.status() != WL_CONNECTED && !elapsed(wifiStartedAt, 20000)) {
    updateLeds();
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    wifiConnectPending = false;
    wifiConnectStartedAt = 0;
    setState(MonitorState::Offline, "Não foi possível conectar ao Wi-Fi configurado.");
    startConfigPortal("Corrija a conexão Wi-Fi.");
    return;
  }

  wifiConnectPending = false;
  wifiConnectStartedAt = 0;
  Serial.printf("[WIFI] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
  if (config.sessionCookie.isEmpty()) {
    startConfigPortal("Faça login para iniciar o monitoramento.");
    return;
  }

  String message;
  if (!checkPlatformSession(message)) {
    startConfigPortal("A sessão precisa ser renovada. Faça login novamente.");
  }
}

void loop() {
  monitorBootButton();
  updateWifiConnectionState();
  maintainWifi();

  if (configMode) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  if (portalShutdownAt != 0 && static_cast<int32_t>(millis() - portalShutdownAt) >= 0) {
    stopConfigPortal();
  }
  if (restartAt != 0 && static_cast<int32_t>(millis() - restartAt) >= 0) {
    ESP.restart();
  }

  if (monitoringCheckDue()) {
    String message;
    checkPlatformSession(message);
    if (monitorState == MonitorState::AuthFailed && !configMode) {
      startConfigPortal("A sessão expirou. Faça login novamente.");
    }
  }

  updateLeds();
  delay(2);
}
