# EasyMQTT

**EasyMQTT** adalah library MQTT sederhana untuk ESP32 yang menggunakan koneksi SSL/TLS dan gaya penggunaan mirip Blynk.

Library ini mempermudah koneksi MQTT dengan fitur:

- SSL/TLS connection (port 8883)
- Auto reconnect WiFi dan MQTT
- Callback seperti `BLYNK_WRITE(v1)` untuk topik
- OTA update via HTTPS (opsional)

## 🔧 Instalasi

Tambahkan ke folder `lib/EasyMQTT/` dalam project PlatformIO kamu, atau gunakan `lib_extra_dirs` jika library disimpan di luar project.

```ini
lib_extra_dirs = ../EasyMQTT
