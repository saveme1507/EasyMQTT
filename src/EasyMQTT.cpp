#include "EasyMQTT.h"
#include <HTTPClient.h>
#include <Update.h>
#include "rootCA.h"
#include <ArduinoJson.h>

std::vector<EasyMQTT::PendingHandler> EasyMQTT::_pendingHandlers;

EasyMQTT *_instance = nullptr;

unsigned long lastVPinTime[101] = {0}; // Index 0-100 untuk V0–V100

EasyMQTT &EasyMQTT::getInstance()
{
  return *_instance;
}

EasyMQTT::EasyMQTT(const char *token)
    : _mqttToken(token)
{
  _instance = this;
}

void EasyMQTT::begin(const char *ssid, const char *password)
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  EASYMQTT_LOG(WiFi.SSID());
  EASYMQTT_LOG(WiFi.localIP());

  _secureClient.stop();
  _secureClient.setInsecure();

  if (!fetchDeviceConfig())
  {
    EASYMQTT_LOG("Gagal mengambil konfigurasi");
    return;
  }

  _secureClient.stop();
  _secureClient.setCACert(ROOT_CA);

  _client.setClient(_secureClient);
  _client.setServer(_mqttServer.c_str(), _mqttPort);
  _client.setCallback([this](char *topic, byte *payload, unsigned int length)
                      {
    String t = topic;
    String message;
    for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
    if (_callbacks.count(t)) _callbacks[t](message); });
}

void EasyMQTT::loop()
{
  if (!_client.connected())
    reconnect();
  _client.loop();
}

void EasyMQTT::reconnect()
{
  String willTopic = String(_user) + "/" + _device + "/status";
  const char *willMessage = "offline";
  const int qos = 1;
  const bool retain = true;
  const char *clientID = _device.c_str();
  const char *username = _mqttUser.c_str();
  const char *password = _mqttPassword.c_str();
  bool cleanSession = true;

  while (!_client.connected())
  {
    EASYMQTT_LOG("Attempting MQTT connection...");

    if (_client.connect(clientID, username, password, willTopic.c_str(), qos, retain, willMessage, cleanSession))
    {
      EASYMQTT_LOG("Connected to MQTT broker.");

      for (auto &kv : _callbacks)
      {
        _client.subscribe(kv.first.c_str());
        EASYMQTT_LOG("Subscribed to " + kv.first);
      }

      if (connectedCallback)
        connectedCallback();

      // Publish online status
      _client.publish(willTopic.c_str(), "online", retain);
      EASYMQTT_LOG("Published online status.");
      registerPendingHandlers();
    }
    else
    {
      EASYMQTT_LOG("Connection failed. Will retry in 2 seconds...");
      if (disconnectedCallback)
        disconnectedCallback();

      delay(2000);
    }
  }
}

void EasyMQTT::subscribe(const String &vpin, EasyMQTTCallback callback)
{
  String topic = getTopic(vpin);
  _callbacks[topic] = callback;
  _client.subscribe(topic.c_str());
}

void EasyMQTT::publish(const String &vpin, const String &payload)
{
  unsigned long now = millis();

  if ((long)(now - lastVPinTime[vpin]) < 500)
  {
    EASYMQTT_LOG(vpin + " publish terlalu cepat");
    return;
  }

  lastVPinTime[vpin] = now;

  EASYMQTT_LOG(getTopic(vpin).c_str());
  EASYMQTT_LOG(payload.c_str());
  _client.publish(getTopic(vpin).c_str(), payload.c_str());
}

String EasyMQTT::getTopic(const String &vpin)
{
  return String(_user) + "/" + _device + "/" + vpin;
}

void EasyMQTT::sendNotification(const String &title, const String &message)
{
  unsigned long now = millis();
  if ((long)(now - lastNotifTime) < 10000)
  {
    EASYMQTT_LOG("Notifikasi terlalu cepat");
    return;
  }

  lastNotifTime = now;

  String topic = String(_user) + "/" + _device + "/notification";
  String payload = "{\"title\":\"" + title + "\",\"message\":\"" + message + "\"}";
  _client.publish(topic.c_str(), payload.c_str(), true); // QoS 0, retain true
}

void EasyMQTT::updateFirmware(const String &url, bool allowInsecure)
{
  WiFiClientSecure client;
  if (allowInsecure)
    client.setInsecure(); // lewati validasi SSL

  HTTPClient https;
  Serial.println("Mulai update firmware dari: " + url);

  if (https.begin(client, url))
  {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK)
    {
      int contentLength = https.getSize();
      bool canBegin = Update.begin(contentLength);

      if (canBegin)
      {
        WiFiClient *updateClient = https.getStreamPtr();
        size_t written = Update.writeStream(*updateClient);

        if (written == contentLength)
        {
          if (Update.end())
          {
            if (Update.isFinished())
            {
              Serial.println("Update selesai. Restarting...");
              ESP.restart();
            }
            else
            {
              Serial.println("Update tidak selesai.");
            }
          }
          else
          {
            Serial.println("Gagal di Update.end(): " + String(Update.getError()));
          }
        }
        else
        {
          Serial.println("Jumlah byte tidak sesuai.");
        }
      }
      else
      {
        Serial.println("Gagal memulai update.");
      }
    }
    else
    {
      Serial.printf("HTTP Error: %d\n", httpCode);
    }
    https.end();
  }
  else
  {
    Serial.println("Gagal mengakses URL OTA");
  }
}

bool EasyMQTT::isConnected()
{
  return _client.connected();
}

bool EasyMQTT::fetchDeviceConfig()
{

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  while (time(nullptr) < 100000)
  {
    delay(100);
  }

  const char *url = "https://api.easylife.biz.id/auth/device/" + _token;

  HTTPClient https;
  https.begin(_secureClient, url);

  int httpCode = https.GET();
  if (httpCode == 200)
  {
    String payload = https.getString();
    EASYMQTT_LOG(payload);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    _mqttPort = doc["mqttPort"] | 8883;
    _mqttServer = doc["mqttServer"] | "";
    _user = doc["user"] | "";
    _device = doc["device"] | "";
    _mqttUser = doc["mqttUser"] | "";
    _mqttPassword = doc["mqttPassword"] | "";
    _token = doc["token"] | "";

    EASYMQTT_LOG("_mqttPort");
    EASYMQTT_LOG(_mqttPort);
    EASYMQTT_LOG("_mqttServer");
    EASYMQTT_LOG(_mqttServer);

    return true;
  }
  else
  {
    Serial.printf("HTTP Error: %d\n", httpCode);
    return false;
  }
}

void EasyMQTT::onConnected(EasyMQTTConnectedHandler handler)
{
  connectedCallback = handler;
}

void EasyMQTT::onDisconnected(EasyMQTTDisconnectedHandler handler)
{
  disconnectedCallback = handler;
}

void EasyMQTT::attachVirtualPin(int vpin, EasyMQTTCallback callback)
{
  if (vpin < 0 || vpin > 100)
  {
    Serial.println("[EasyMQTT] Error: vPin harus antara V0 sampai V100");
    return;
  }

  String topic = getTopic("V" + String(vpin));
  _callbacks[topic] = callback;

  if (_client.connected())
  {
    _client.subscribe(topic.c_str());
  }
}

void EasyMQTT::registerPendingHandlers()
{
  for (auto &p : _pendingHandlers)
  {
    attachVirtualPin(p.vpin, p.callback);
  }
  _pendingHandlers.clear();
}
