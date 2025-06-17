#include <EasyMQTT.h>

// Ganti dengan kredensial WiFi kamu
const char *ssid = "NAMA_WIFI";
const char *password = "PASSWORD_WIFI";

// Info MQTT dan perangkat
const char *user = "user123";
const char *device = "esp32dev";
const char *mqttServer = "mqtt.easylife.biz.id";
const int mqttPort = 8883;
const char *mqttUser = "mqtt_user";
const char *mqttPassword = "mqtt_password";

// Inisialisasi EasyMQTT
EasyMQTT mqtt(user, device, mqttServer, mqttPort, mqttUser, mqttPassword);

void setup()
{
    Serial.begin(115200);

    mqtt.onConnected([]()
                     {
    Serial.println("[MQTT] Terhubung!");
    mqtt.subscribe("v99", [](String url) {
      Serial.println("[OTA] URL diterima: " + url);
      mqtt.updateFirmware(url, true);  // true = allow insecure SSL
    }); });

    mqtt.onDisconnected([]()
                        { Serial.println("[MQTT] Terputus."); });

    mqtt.begin(ssid, password);
}

void loop()
{
    mqtt.loop();
}
