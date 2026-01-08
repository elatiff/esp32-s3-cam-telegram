# ESP32-S3-CAM Telegram Bot

Proyek ESP-IDF untuk ESP32-S3-CAM dengan fitur **Face Detection** dan **Motion Detection** yang mengirim gambar ke **Telegram**.

## 📦 Fitur

- ✅ **Motion Detection** - Mendeteksi gerakan menggunakan perbandingan frame
- ✅ **Face Detection** - Mendeteksi wajah menggunakan analisis skin-tone (simplified)
- ✅ **Telegram Integration** - Mengirim foto dan notifikasi ke Telegram Bot
- ✅ **LED Indication** - Indikasi status via LED
- ✅ **Multi-board Support** - Mendukung berbagai modul ESP32-S3-CAM

## 🛠️ Hardware yang Didukung

- ESP32-S3-CAM (FREENOVE/Generic)
- ESP32-S3-EYE (Espressif)
- Seeed XIAO ESP32S3 Sense

## 📋 Prasyarat

1. **ESP-IDF** v5.0 atau lebih baru
2. **Python 3.8+**
3. **Telegram Bot Token** dari [@BotFather](https://t.me/BotFather)
4. **Chat ID** Telegram Anda

## 🔧 Instalasi

### 1. Clone Repository

```bash
cd /path/to/project
```

### 2. Setup Environment

```bash
. $HOME/esp/esp-idf/export.sh
```

### 3. Konfigurasi

```bash
idf.py menuconfig
```

Navigasi ke **ESP32-S3-CAM Telegram Configuration** dan isi:

- **WiFi Configuration**
  - WiFi SSID
  - WiFi Password
  
- **Telegram Bot Configuration**
  - Bot Token (dari BotFather)
  - Chat ID (gunakan [@userinfobot](https://t.me/userinfobot) untuk mendapatkannya)
  
- **Detection Configuration**
  - Enable/Disable Face Detection
  - Enable/Disable Motion Detection
  - Motion Threshold (sensitivitas)
  - Detection Interval
  - Telegram Cooldown (untuk menghindari spam)
  
- **Camera Configuration**
  - Pilih modul kamera yang digunakan

### 4. Build & Flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📱 Setup Telegram Bot

### Langkah 1: Buat Bot

1. Buka Telegram dan cari **@BotFather**
2. Kirim `/newbot`
3. Ikuti instruksi untuk memberi nama bot
4. Salin **Bot Token** yang diberikan

### Langkah 2: Dapatkan Chat ID

1. Buka Telegram dan cari **@userinfobot**
2. Kirim `/start`
3. Salin **Chat ID** Anda

### Langkah 3: Mulai Bot

1. Cari bot Anda di Telegram
2. Kirim `/start` ke bot
3. Sekarang bot dapat mengirim pesan kepada Anda

## 📁 Struktur Project

```
esp32-s3-cam-tele/
├── CMakeLists.txt           # Project CMake
├── sdkconfig.defaults       # Default SDK config
├── partitions.csv           # Custom partition table
├── README.md                # Dokumentasi ini
└── main/
    ├── CMakeLists.txt       # Component CMake
    ├── idf_component.yml    # Component dependencies
    ├── Kconfig.projbuild    # Menuconfig options
    ├── main.c               # Aplikasi utama
    ├── wifi_manager.c       # WiFi handler
    ├── camera_manager.c     # Camera handler
    ├── motion_detector.c    # Motion detection
    ├── face_detector.c      # Face detection
    ├── telegram_bot.c       # Telegram API client
    ├── led_control.c        # LED control
    ├── telegram_root_cert.pem  # SSL certificate
    └── include/
        ├── wifi_manager.h
        ├── camera_manager.h
        ├── motion_detector.h
        ├── face_detector.h
        ├── telegram_bot.h
        └── led_control.h
```

## ⚙️ Konfigurasi Default

| Parameter | Default | Keterangan |
|-----------|---------|------------|
| Motion Threshold | 15 | Perbedaan pixel minimum |
| Pixel Threshold | 5% | Persentase pixel berubah |
| Detection Interval | 500ms | Interval antar deteksi |
| Telegram Cooldown | 10s | Waktu tunggu antar notifikasi |

## 🔍 Troubleshooting

### Camera tidak terdeteksi

- Pastikan pin kamera sesuai dengan board yang dipilih
- Cek power supply (ESP32-S3-CAM butuh minimal 5V/1A)
- Pastikan PSRAM terdeteksi

### WiFi tidak connect

- Verifikasi SSID dan password
- Pastikan router dalam jangkauan
- Coba restart ESP32

### Telegram tidak mengirim

- Verifikasi Bot Token benar
- Pastikan sudah mengirim `/start` ke bot
- Cek Chat ID benar
- Verifikasi koneksi internet

### Brownout detected

- Gunakan power supply yang lebih kuat
- Kurangi kecerahan flash LED
- Tambahkan kapasitor 100µF pada VCC

## 📝 Catatan

- Face detection menggunakan metode simplified (skin-tone analysis)
- Untuk face detection yang lebih akurat, integrasikan dengan ESP-DL human_face_detect
- Motion detection bekerja optimal dengan pencahayaan stabil
- Cooldown mencegah spam notifikasi

## 📄 License

MIT License

## 🙏 Credits

- Espressif ESP-IDF
- Espressif ESP32-Camera
- Telegram Bot API
