#include <EasyMQTT.h>

// Konfigurasi WiFi dan MQTT
const char *WIFI_SSID = "your-ssid";
const char *WIFI_PASSWORD = "your-password";

const char *MQTT_SERVER = "mqtt.easylife.biz.id";
const int MQTT_PORT = 8883;
const char *MQTT_USER = "username";
const char *MQTT_PASS = "password";

// Identitas device
const char *USER = "user";
const char *DEVICE = "esp32";

// Inisialisasi EasyMQTT
EasyMQTT mqtt(USER, DEVICE, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

void setup()
{
    Serial.begin(115200);

    // Mulai koneksi WiFi & MQTT
    mqtt.begin(WIFI_SSID, WIFI_PASSWORD);

    // Callback ketika berhasil terkoneksi ke MQTT
    mqtt.onConnected([]()
                     {
    Serial.println("Terhubung ke MQTT");

    // Kirim notifikasi saat terkoneksi
    mqtt.sendNotification("ESP32 Aktif", "Perangkat baru saja online"); });

    // Callback jika koneksi terputus
    mqtt.onDisconnected([]()
                        { Serial.println("Terputus dari MQTT"); });
}

void loop()
{
    mqtt.loop();

    // Contoh kirim notifikasi setiap 10 detik (hanya demo)
    static unsigned long lastNotify = 0;
    if (millis() - lastNotify > 10000)
    {
        lastNotify = millis();
        mqtt.sendNotification("Notifikasi Rutin", "Ini adalah pesan setiap 10 detik");
    }
}
