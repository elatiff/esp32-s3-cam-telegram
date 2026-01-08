# 🕵️ Workshop IoT Day 1: Membuat "CCTV Pintar" Telegram

**Selamat Datang di Dunia IoT!**
Hari ini kita akan belajar bagaimana cara membuat alat canggih yang bisa mendeteksi gerakan dan mengirim fotonya langsung ke HP kamu lewat Telegram. Keren kan? 😎

---

## 🌍 Apa itu IoT?

**IoT (Internet of Things)** itu gampangnya adalah:
**"Benda-benda di sekitar kita yang dikasih Internet biar jadi Pintar"**

Contohnya:
- 💡 Lampu yang bisa nyala sendiri pas kita pulang.
- 📺 Kulkas yang bisa pesen susu kalau habis.
- 📷 **Kamera yang lapor ke Telegram kalau ada maling!** (Ini yang akan kita buat hari ini!)

---

## � Kamus Kecil (Istilah Penting)

Supaya gak bingung, yuk kenalan sama istilah-istilah di dunia elektro:

### 1. Software (Tempat Nulis Kode)
| Istilah | Penjelasan Simpel |
| :--- | :--- |
| **Arduino IDE** | Aplikasi coding sejuta umat. Gampang banget, cocok buat belajar dasar. Ibarat "buku tulis biasa". |
| **ESP-IDF** | Aplikasi coding **Profesional** dari pembuat chipnya. Lebih canggih dan bisa akses fitur dalam (seperti AI). Kita pakai ini biar hasil maksimal! 🚀 |

### 2. Kelistrikan (Makanan Robot)
| Istilah | Penjelasan Simpel |
| :--- | :--- |
| **5V** (Volt) | Tegangan listrik dari USB Laptop. Anggap aja ini "Energi Besar" (Main Power). |
| **3V3** (Volt) | Tegangan listrik khusus untuk Chip/Otak. Anggap aja ini "Energi Sedang". **⚠️ PENTING:** Jangan colok 5V ke pin 3V3, nanti chipnya hangus! 🔥 |
| **GND** (Ground) | **Kutub Negatif (-)**. Ini seperti "Tanah/Lantai". Semua arus listrik wajib balik ke sini. Kalau GND gak nyambung, alat GAK AKAN NYALA. |

### 3. Komunikasi (Cara Ngobrol)
| Istilah | Penjelasan Simpel |
| :--- | :--- |
| **TX** (Transmit) | **MULUT** 🗣️ (Buat ngomong/kirim data). |
| **RX** (Receive) | **TELINGA** 👂 (Buat denger/terima data). |
| **Aturan Emas** | Kabel **TX** harus colok ke **RX** (Mulut ketemu Telinga). Jangan Mulut ketemu Mulut, nanti gak nyambung! 🤣 |

### 4. Lain-lain
- **GPIO** _(General Purpose Input Output)_: Kaki-kaki logam di board ESP32 yang bisa diprogram sesuka hati. Bisa kita suruh jadi tombol, lampu, sensor, dll. Ibarat "Tangan & Kaki" robot.

---

## �🛠️ Alat Perang Kita (Hardware)

Kita pakai alat-alat canggih ini (beberapa akan dipakai besok):

1.  **ESP32-S3-CAM** 🧠📷
    -   **Otak** dari proyek kita. Punya **Kamera** dan **WiFi**.
2.  **USB to TTL PL2303** 🔌
    -   **Penerjemah** bahasa komputer ke ESP32. Alat wajib buat masukin program (coding).
3.  **Microphone (INMP441)** 🎤
    -   **Telinga** robot. Bikin alat kita bisa mendengar suara kamu. (Dipakai hari ke-2)
4.  **Speaker + Amplifier (MAX98357)** �
    -   **Mulut** robot. Bikin alat kita bisa bicara balik ke kamu. (Dipakai hari ke-2)
5.  **Layar OLED (SSD1306)** 📺
    -   **Wajah/Layar** kecil. Buat nampilin tulisan atau mata robot. (Dipakai hari ke-2)
6.  **Kabel Jumper** 🌈
    -   **Urat Nadi**. Kabel warna-warni buat nyambungin semua alat di atas.

---

## 🚀 Misi Hari Ini

1.  **Rakitan**: Menyambungkan kabel-kabel.
2.  **Bikin Bot Telegram**: Membuat "robot" chat kamu sendiri.
3.  **Coding (Dikit aja)**: Masukin "jiwa" ke ESP32.
4.  **Aksi**: Tes kamera deteksi gerakan!

---

## ⚡ Langkah 1: Merakit (Wiring)

Kita perlu menghubungkan ESP32 ke Laptop supaya bisa diisi program.
Hati-hati ya, jangan sampai salah warna kabel!

**Sambungkan USB Adapter ke ESP32:**

| USB Adapter (PL2303) | Kabel | ESP32-S3-CAM |
| :--- | :---: | :--- |
| **5V** (Merah) | 🟥 | **5V** |
| **GND** (Hitam) | ⬛ | **GND** |
| **TX** (Hijau/Putih) | 🟩 | **RX** (Ingat: Menyilang!) |
| **RX** (Putih/Hijau) | ⬜ | **TX** (Ingat: Menyilang!) |

> **Tips:**
> - **TX** ketemu **RX**
> - **RX** ketemu **TX**
> - Kalau kebalik, nanti error "Failed to connect".

---

## 🤖 Langkah 2: Membuat Bot Telegram

Biar kamera bisa kirim foto, kita butuh "alamat" tujuan di Telegram.

1.  Buka aplikasi **Telegram** di HP/Laptop.
2.  Cari akun bernama: **@BotFather** (ini bapaknya semua bot!).
3.  Chat: `/newbot`
4.  Kasih nama bot kamu. Contoh: `SatpamPintar_NamaKamu`.
5.  Kasih username (harus akhirannya 'bot'). Contoh: `SatpamPintar123_bot`.
6.  **SIMPAN KODE API TOKEN!**
    -   Kodenya panjang & aneh, contoh: `110201543:AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw`
    -   Ini kuncinya, jangan kasih tau orang lain!

**Satu lagi: Cari Chat ID kamu**
1.  Cari bot bernama: **@userinfobot**
2.  Klik tombol **Start** atau chat `/start`.
3.  Dia akan balas dengan angka (contoh: `123456789`). Itu ID kamu. Catat ya!

---

## 💻 Langkah 3: Setup Program (Software)

Kita akan menggunakan kode program yang sudah disiapkan.

### 1. Buka Terminal
Buka aplikasi terminal di laptop kamu (Tanya kakak mentor kalau bingung!).

### 2. Masuk ke Folder Proyek
Ketik perintah ini (atau copy-paste):

```bash
cd /media/elatif/Data/Work/Coolpineapple/IOT/esp32-s3-cam-tele
```

### 3. Setting WiFi & Telegram
Kita perlu kasih tau ESP32 nama WiFi dan Token Telegram tadi.

Ketik:
```bash
idf.py menuconfig
```

Nanti muncul layar biru/abu-abu seperti BIOS komputer.
1.  Pilih menu **"ESP32-S3-CAM Telegram Configuration"**.
2.  Masuk ke **WiFi Configuration**:
    -   Isi nama WiFi (SSID).
    -   Isi password WiFi.
3.  Masuk ke **Telegram Bot Configuration**:
    -   Isi **Bot Token** (yang panjang tadi).
    -   Isi **Chat ID** (angka ID kamu).
4.  Tekan tombol **S** (Save) -> Enter.
5.  Tekan tombol **Esc** (Exit) sampai keluar.

---

## 🔥 Langkah 4: Upload & Jalankan!

Sekarang saatnya memasukkan program ke otak ESP32.

### 1. Masuk Mode Download (BOOT Mode)
Supaya ESP32 mau diisi program, kita harus "pancing" dulu:
1.  Cabut kabel USB dari laptop.
2.  Ambil satu kabel jumper, hubungkan **IO0** (Angka 0) ke **GND**.
3.  Colok kabel USB ke laptop.
4.  Tunggu 2 detik.
5.  **Lepas kabel jumper IO0 tadi**.

*(Sekarang ESP32 sudah siap menerima data)*

### 2. Perintah Upload
Ketik perintah "sakti" ini di terminal:

```bash
idf.py build flash monitor
```

-   **build**: Masak kodenya.
-   **flash**: Kirim ke ESP32.
-   **monitor**: Lihat apa yang terjadi.

Tunggu sebentar... banyak tulisan berjalan... ⏳
Kalau berhasil, tulisan akhirnya akan berhenti dan muncul log sistem.

---

## 🎉 Langkah 5: Uji Coba (Demo Time!)

Lihat di layar terminal (monitor), kalau ada tulisan:
`WiFi Connected` ... `Telegram Bot Started` ...
Berarti SKSES! 🥳

**Cara Tes:**
1.  Buka Telegram kamu.
2.  Cari nama bot kamu tadi.
3.  Klik **Start** atau ketik `/start`.
4.  Sekarang, gerakkan tangan kamu di depan kamera ESP32. 👋
5.  Tunggu beberapa detik...
6.  **TING! 🔔** Foto akan masuk ke Telegram kamu!

**Selamat! Kamu baru saja membuat CCTV Pintar!** 🏆

---

## 🔍 Kalau Error Gimana? (Troubleshooting)

-   **Gak bisa upload?**
    -   Cek kabel TX/RX (sering kebalik!).
    -   Ulangi langkah "Masuk Mode Download".
-   **WiFi gak connect?**
    -   Apa password WiFi salah? Cek lagi di `idf.py menuconfig`.
-   **Gak kirim foto?**
    -   Kameranya kebalik? Atau gelap?
    -   Pastikan kamu sudah chat `/start` ke bot kamu.

---

## 🔮 Bocoran Rahasia Day 2: Robot yang Bisa BICARA!

Hari ini robot kita sudah **Punya Mata** (Kamera) dan bisa lapor ke Telegram.
 **TAPI...** dia masih "BISU" dan "TULI". Kasihan kan? 😢

Besok, kita akan operasi plastik robot kita dan memasang alat-alat canggih ini:

### 1. Microphone **INMP441** (Telinga Ajaib) 🎤
Besok robotmu bakal bisa **MENDENGAR**.
Kamu bisa perintah dia: *"Halo robot, nyalakan lampu!"* atau *"Siapa nama presiden Indonesia?"*

### 2. Speaker Amplifier **MAX98357** (Mulut Robot) 🔊
Gak cuma diam, besok robotmu bakal **MENJAWAB** dengan suara manusia!
Dia bisa curhat, cerita lucu, atau menjawab pertanyaan PR kamu.
*"Halo bos! Jawaban matematika tadi adalah 50!"*

### 3. Layar OLED **SSD1306** (Wajah & Ekspresi) 📺
Layar kecil ini bakal jadi **WAJAH**-nya.
Dia bisa senyum 😊 kalau kamu puji, bisa bingung 🤨 kalau pertanyaanmu susah, atau menampilkan status Wifi.

---

### **Misi Besok: Membuat JARVIS Sendiri!** 🦾�
Bayangkan punya asisten pribadi kayak di film Iron Man, tapi buatan tanganmu sendiri.
Pastikan besok datang tepat waktu ya!

Sampai jumpa di Day 2! 👋
