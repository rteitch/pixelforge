# Product Requirements Document (PRD)
## PixelForge — Desktop Image Style & Filter Editor

| | |
|---|---|
| **Versi Dokumen** | 1.0 |
| **Tanggal** | 21 Juni 2026 |
| **Status** | Draft untuk Review |
| **Pemilik Dokumen** | Product Manager |
| **Bahasa Implementasi** | C++17/20 |

---

## 1. Ringkasan Eksekutif

PixelForge adalah aplikasi desktop *cross-platform* (Windows/Linux/macOS) berbasis C++ untuk mengubah foto menjadi gaya seni dan filter sinematik secara lokal (offline), cepat, dan dapat di-reproduksi. Aplikasi menyasar tiga kategori transformasi utama:

1. **WPAP (Wedha's Pop Art Portrait)** — portrait diubah menjadi seni *geometric low-poly* dengan bidang-bidang warna kontras khas WPAP.
2. **Filter sinematik (Cinematic Color Grading)** — look ala film (teal-orange, Kodak/Fuji emulation, moody, bleach bypass, dll).
3. **Filter gaya populer lain** — termasuk *Japan Style* (Mono no Aware / Yohji-muted, Wong Kar-wai neon, Anime-flat, Vintage Film Japan), serta gaya umum lain (Vintage/Retro, Black & White Fine Art, HDR Dramatic, Pastel/K-style).

Aplikasi dirancang sebagai **engine pemrosesan gambar performa tinggi** dengan GUI desktop native, mendukung *batch processing*, *non-destructive editing*, dan ekstensi filter via plugin/preset di masa depan.

---

## 2. Latar Belakang & Masalah

### 2.1 Konteks
WPAP adalah gaya seni asal Indonesia (diciptakan oleh Wedha Abdul Rasyid) yang sangat populer untuk portrait, namun pembuatannya manual di Adobe Illustrator memakan waktu berjam-jam per gambar. Di sisi lain, filter sinematik dan gaya "Japan aesthetic" sangat populer di media sosial, namun:

- Aplikasi mobile (VSCO, Snapseed, Lightroom Mobile) **bergantung pada koneksi internet** untuk filter premium dan AI.
- Tool berbasis Python/AI (neural style transfer) **lambat di perangkat tanpa GPU** dan sulit didistribusikan sebagai aplikasi ringan.
- Tidak ada tool desktop *open native performance* yang menggabungkan WPAP generator otomatis dengan color grading sinematik dalam satu aplikasi ringan, cepat, dan berjalan offline penuh.

### 2.2 Peluang
Dengan C++ dan pustaka pengolahan gambar (OpenCV) plus algoritma geometri komputasi (Delaunay triangulation, k-means/superpixel segmentation), proses WPAP dan color grading dapat dilakukan **secara deterministik, cepat (<2 detik per gambar di CPU modern)**, dan tanpa dependensi cloud/AI berat.

### 2.3 Target Pengguna
- **Desainer grafis & content creator** yang butuh hasil WPAP/cinematic cepat tanpa Illustrator manual.
- **Fotografer hobi/semi-pro** yang ingin look sinematik/Japan-style konsisten untuk portofolio.
- **Pemilik bisnis kecil** (studio foto, percetakan merchandise WPAP) yang butuh produksi massal/batch.
- **Developer/power user** yang ingin mengintegrasikan engine via CLI/SDK ke pipeline mereka sendiri.

---

## 3. Tujuan Produk (Goals)

### 3.1 Tujuan Bisnis
- Menjadi alternatif desktop *offline-first* untuk filter sinematik & WPAP dengan kualitas setara tool premium.
- Membangun *engine* modular yang dapat dikembangkan menjadi SDK/plugin di masa depan (monetisasi preset pack).

### 3.2 Tujuan Produk
- Mengonversi foto menjadi WPAP secara **otomatis** dengan kontrol manual opsional (jumlah warna, ukuran poligon, palet).
- Menyediakan minimal **15 preset filter** lintas kategori (Cinematic, Japan Style, Vintage, B&W, dll) yang dapat disesuaikan.
- Mendukung **batch processing** ratusan gambar tanpa menurunkan stabilitas aplikasi.
- Memberikan pengalaman *non-destructive editing* (preview real-time, undo/redo, export tanpa merusak file asli).

### 3.3 Non-Goals (di luar lingkup v1)
- Tidak menyediakan AI generative image (text-to-image) atau neural style transfer berbasis deep learning di versi pertama.
- Tidak menyasar platform mobile (iOS/Android) — dibahas di roadmap jangka panjang.
- Tidak menyediakan fitur kolaborasi cloud/real-time multi-user.
- Tidak menyediakan video editing (hanya still image).

---

## 4. Rekomendasi Pendekatan Teknis

Karena pengguna meminta rekomendasi, berikut pendekatan yang direkomendasikan beserta alasannya.

### 4.1 WPAP Generator — Rekomendasi: **Geometric/Computational, bukan Deep Learning**

| Pendekatan | Kecepatan | Kontrol Artistik | Kompleksitas Implementasi | Rekomendasi |
|---|---|---|---|---|
| Manual tracing (seperti software desain) | Lambat (manual) | Sangat tinggi | Rendah (tidak otomatis) | Tidak dipakai |
| **Delaunay Triangulation + Edge/Feature Detection + K-Means Color Quantization** | Cepat (<2s) | Tinggi (parametrik) | Sedang | ✅ **Dipilih** |
| Neural Style Transfer (CNN) | Lambat tanpa GPU (5-30s) | Rendah (gaya general, bukan WPAP spesifik) | Tinggi | Tidak dipakai (v1) |
| GAN khusus WPAP (perlu dataset training) | Cepat saat inferensi, tapi butuh training mahal | Tinggi tapi butuh dataset besar | Sangat tinggi | Dipertimbangkan untuk v3+ |

**Alasan:** WPAP punya struktur visual yang sangat geometris dan dapat didekati secara matematis murni (bukan butuh "kreativitas" generatif), sehingga pipeline klasik computer vision lebih cepat, ringan, deterministik, dan jauh lebih mudah dikontrol pengguna (jumlah facet, ukuran poligon, jumlah warna) dibanding model AI generatif yang "black box".

**Pipeline yang direkomendasikan:**
1. **Face/Edge Detection** — deteksi wajah & fitur tepi (Haar Cascade/DNN face detector ringan dari OpenCV) untuk memberi bobot lebih pada poligon di area wajah.
2. **Color Quantization** — kuantisasi warna ke palet kontras-tinggi (k-means clustering pada ruang warna LAB, default 8-24 warna sesuai gaya WPAP).
3. **Edge-Aware Point Sampling** — sampling titik kontrol lebih padat di area edge/kontras tinggi (menggunakan gradient magnitude/Canny edge sebagai *density map*).
4. **Delaunay Triangulation** — buat mesh segitiga dari titik-titik sampel (library: CGAL atau implementasi ringan sendiri).
5. **Facet Color Fill** — tiap segitiga diwarnai dengan warna dominan/rata-rata area tersebut dari hasil kuantisasi.
6. **Optional Vector Export (SVG)** — agar hasil bisa di-scale tanpa pecah, cocok untuk cetak merchandise.

### 4.2 Filter Cinematic & Japan Style — Rekomendasi: **LUT + Parametric Color Grading (bukan Deep Learning)**

| Pendekatan | Kecepatan | Konsistensi | Kemudahan Bikin Preset Baru | Rekomendasi |
|---|---|---|---|---|
| **3D LUT (.cube) + Curve/Tone Mapping manual** | Sangat cepat (real-time, GPU-friendly) | Sangat konsisten | Mudah (tinggal load LUT baru) | ✅ **Dipilih (mayoritas)** |
| Parametric grading manual (HSL split-tone, curves, custom code per filter) | Cepat | Konsisten | Sedang (perlu coding manual tiap gaya unik) | ✅ **Dipilih (untuk filter kompleks: grain, halation, vignette)** |
| Neural style transfer per filter | Lambat | Tidak konsisten (variatif tiap gambar) | Sulit (perlu training/fine-tune) | Tidak dipakai (v1), opsional di roadmap v3 sebagai filter eksperimental "AI Style" |

**Alasan:** Look sinematik (teal-orange, bleach bypass) dan estetika Japan-style sebenarnya adalah kombinasi **transformasi warna matematis yang sudah well-documented di industri film** (color grading dengan LUT adalah standar industri Hollywood/Netflix). Pendekatan LUT + parametric grading:
- Berjalan **real-time** tanpa GPU mahal (cocok untuk preview interaktif di C++ native).
- Hasilnya **dapat diprediksi & direplikasi** — penting untuk preset yang konsisten antar foto.
- Memungkinkan pengguna **membuat/impor LUT custom** (.cube format, kompatibel dengan software industri seperti DaVinci Resolve).

**Pipeline yang direkomendasikan per filter:**
1. **Base Tone Mapping** — adjust exposure, contrast, highlight/shadow rolloff (curve-based, mirip film response curve).
2. **3D LUT Application** — trilinear interpolation pada LUT 17³/33³ untuk transformasi warna utama (look dasar: teal-orange, Kodachrome, dll).
3. **Split Toning** — warna berbeda untuk shadow vs highlight (ciri khas cinematic & Japan moody style).
4. **Texture Layer** — film grain (procedural noise dengan size/intensity parametrik), halation (bloom pada highlight), light leak (opsional, untuk vintage).
5. **Vignette & Chromatic Aberration** — efek lensa untuk look sinematik.
6. **Sharpening/Softening akhir** — unsharp mask atau gaussian blur halus sesuai gaya (Japan-style sering pakai soft-focus halus).

### 4.3 Kategori "Japan Style" — Detail Sub-Gaya yang Direkomendasikan
Karena "Japan Style" adalah istilah luas, berikut pemecahan sub-gaya konkret yang akan diimplementasikan sebagai preset terpisah:
- **Mono no Aware (Muted Pastel)** — desaturasi lembut, highlight kebiruan, shadow kehijauan, kontras rendah (terinspirasi sinematografi Ozu/Hirokazu Kore-eda).
- **Wong Kar-wai Neon Tokyo** — saturasi tinggi pada warna neon (merah/hijau), shadow gelap pekat, grain kasar.
- **Showa Retro Film** — emulasi film Fujifilm era 80-90an, warna hangat, vignette kuat, light leak halus.
- **Anime Flat Look** — kontras tinggi, saturasi cerah, *posterize* halus pada gradasi kulit (bukan WPAP, lebih halus).

### 4.4 Stack Teknis yang Direkomendasikan

| Komponen | Pilihan | Alasan |
|---|---|---|
| Bahasa inti | **C++17/20** | Sesuai permintaan, performa native |
| Image processing core | **OpenCV (core, imgproc)** | Standar industri, dukungan SIMD, lisensi Apache 2.0 |
| Geometri WPAP (Delaunay) | **CGAL** atau implementasi Bowyer-Watson sendiri | CGAL robust tapi berat (lisensi GPL untuk fitur tertentu—perlu cek); alternatif ringan: implementasi sendiri agar lisensi bebas |
| GUI Framework | **Qt 6 (LGPL)** | Cross-platform native look, dukungan OpenGL untuk preview real-time, ekosistem matang |
| Akselerasi preview | **OpenGL/Compute Shader (opsional)** atau multithreading CPU (std::thread/OpenMP) | Real-time preview tanpa wajib GPU diskrit |
| Build system | **CMake** | Standar cross-platform C++ |
| Package/Installer | **CPack (Windows: NSIS/MSI, macOS: DMG, Linux: AppImage/DEB)** | Distribusi multi-platform |
| Format LUT | **.cube (Adobe/DaVinci standard)** | Kompatibel dengan tool industri, mudah dibuat di tool eksternal lalu diimpor |
| Testing | **GoogleTest** | Standar C++ |

---

## 5. Fitur & Requirement

### 5.1 Fitur Inti (Must-Have, v1.0)

#### F1 — Import & Export Gambar
- Import format: JPEG, PNG, BMP, TIFF, WebP.
- Export format: JPEG (quality slider), PNG (lossless), TIFF (untuk cetak).
- Mempertahankan metadata EXIF dasar (opsional toggle "strip metadata" untuk privasi).
- Resolusi maksimum yang didukung: minimal 8000×8000 px tanpa crash (dengan tiling/chunked processing jika perlu).

#### F2 — Modul WPAP Generator
- Generate WPAP otomatis dari satu klik dengan parameter default.
- Kontrol manual:
  - Jumlah warna/palet (slider 6–32 warna).
  - Tingkat detail/jumlah poligon (low/medium/high/custom angka titik).
  - Pilihan palet warna (preset: Vibrant, Pastel, Monochrome-accent, Custom palette picker).
  - Deteksi wajah otomatis (toggle on/off) untuk fokus detail di area wajah.
- Preview real-time saat slider digeser (debounced agar tidak lag).
- Export hasil sebagai **raster (PNG/JPEG)** dan **vector (SVG)**.

#### F3 — Modul Filter Style (Cinematic, Japan Style, Lainnya)
- Minimal **15 preset filter** built-in saat rilis v1, terbagi:
  - Cinematic: Teal & Orange, Bleach Bypass, Moody Blue, Film Noir, Golden Hour.
  - Japan Style: Mono no Aware, Wong Kar-wai Neon Tokyo, Showa Retro Film, Anime Flat Look.
  - Lainnya: Vintage Kodak, Vintage Polaroid, Black & White Fine Art, HDR Dramatic, Pastel Soft, Urban Street Gritty.
- Setiap preset dapat di-*fine-tune* dengan slider: Intensity (0-100%), Grain Amount, Vignette Strength, Temperature/Tint tambahan.
- Kemampuan **impor LUT custom** (.cube file) sebagai preset baru.
- Kemampuan **simpan preset hasil tuning sebagai preset baru** (custom user preset).
- Preview real-time split-view (before/after) dan side-by-side.

#### F4 — Non-Destructive Editing & History
- Setiap efek diterapkan sebagai *layer/adjustment* yang dapat diubah urutannya atau dihapus, bukan permanen menimpa pixel asli sampai export.
- Undo/redo unlimited dalam satu sesi.
- Simpan "project file" (format custom `.pforge` berisi referensi gambar asli + stack adjustment dalam JSON) agar bisa dibuka & diedit ulang kapan saja.

#### F5 — Batch Processing
- Pilih folder/banyak file sekaligus, terapkan satu preset (WPAP atau filter) ke semua.
- Progress bar per-file dan keseluruhan, dengan opsi cancel.
- Output otomatis ke folder terpisah dengan penamaan file yang dapat dikustomisasi (`{nama_asli}_{preset}.jpg`).
- Pemrosesan paralel multi-thread agar batch besar tidak memblokir UI.

#### F6 — UI/UX Dasar
- Single-window workspace: panel kiri (tools/preset), kanvas tengah (preview), panel kanan (parameter/properties).
- Drag-and-drop file ke kanvas untuk membuka gambar.
- Tema gelap (default, cocok untuk kerja warna) dan tema terang.
- Dukungan keyboard shortcut standar (Ctrl+Z, Ctrl+S, dll).

### 5.2 Fitur Tambahan (Should-Have, v1.x)
- Color picker manual untuk override warna spesifik hasil WPAP (klik poligon → ganti warna).
- Preset "favorit"/bookmark untuk akses cepat.
- Crop & rotate dasar sebelum apply filter.
- Comparison grid (lihat satu gambar dengan beberapa preset sekaligus dalam grid thumbnail).

### 5.3 Fitur Masa Depan (Nice-to-Have, v2+)
- Plugin system (load filter/preset dari pihak ketiga via dynamic library).
- CLI mode penuh untuk integrasi automation/scripting (`pixelforge --input photo.jpg --preset cinematic_teal --output out.jpg`).
- Mode "AI Style" eksperimental (opsional, neural style transfer ringan untuk gaya yang sulit didekati matematis, dijalankan via ONNX Runtime agar tetap CPU-friendly).
- Dukungan video singkat (terapkan filter ke tiap frame video pendek, untuk konten reels/shorts).
- Sinkronisasi preset via akun cloud opsional (tetap offline-first, cloud hanya untuk backup).

---

## 6. Spesifikasi Teknis & Arsitektur

### 6.1 Arsitektur Tingkat Tinggi

```
┌─────────────────────────────────────────────────────────────┐
│                      Presentation Layer                      │
│              (Qt 6 GUI — Widgets/QML, OpenGL Preview)        │
└───────────────────────────┬───────────────────────────────────┘
                            │  (Signal/Slot, Commands)
┌───────────────────────────▼───────────────────────────────────┐
│                     Application Layer                        │
│   Project Manager | History/Undo Manager | Batch Orchestrator│
└───────────────────────────┬───────────────────────────────────┘
                            │
┌───────────────────────────▼───────────────────────────────────┐
│                      Core Engine Layer (C++)                 │
│  ┌───────────────┐  ┌───────────────────┐  ┌────────────────┐│
│  │ WPAP Module    │  │ Color Grading      │  │ I/O Module     ││
│  │ - FaceDetect   │  │ Module             │  │ - Decoder      ││
│  │ - EdgeSample   │  │ - LUT Engine       │  │ - Encoder      ││
│  │ - Delaunay     │  │ - Curve/ToneMap    │  │ - Metadata     ││
│  │ - ColorQuant   │  │ - Grain/Vignette   │  │                ││
│  │ - SVGExport    │  │ - SplitTone        │  │                ││
│  └───────────────┘  └───────────────────┘  └────────────────┘│
└───────────────────────────┬───────────────────────────────────┘
                            │
┌───────────────────────────▼───────────────────────────────────┐
│                  Foundation Layer                             │
│     OpenCV (core, imgproc, photo) | Threading (std::thread/   │
│     OpenMP) | SIMD (intrinsics) | CGAL/custom geometry         │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Modul Detail

**6.2.1 WPAP Module**
- Input: `cv::Mat` gambar asli + parameter (jumlah warna, density, mode wajah).
- Output: `cv::Mat` raster WPAP + struktur data poligon (untuk export SVG).
- Kompleksitas target: O(n log n) untuk triangulasi dengan n titik sampel (~500–5000 titik tergantung detail level).

**6.2.2 Color Grading Module**
- LUT diterapkan via **trilinear interpolation** pada grid 3D (ukuran umum 17×17×17 atau 33×33×33).
- Tone curve direpresentasikan sebagai kurva Bezier/spline yang dapat dikonfigurasi per channel (R, G, B, Luma).
- Grain menggunakan Perlin/simplex noise yang diberi bobot berdasarkan luminance area (grain lebih terlihat di shadow, sesuai karakter film asli).

**6.2.3 I/O Module**
- Decoder/encoder gambar: gunakan `libjpeg-turbo`, `libpng`, `libtiff` (biasanya sudah terbundel via OpenCV, namun perlu pin versi untuk keamanan).
- Validasi file (cek magic bytes, bukan hanya ekstensi) untuk mencegah file corrupt/malicious menyebabkan crash.

### 6.3 Performance Requirement
- Preview real-time filter (bukan WPAP) pada gambar resolusi preview (≤2MP) harus berjalan **≥24 fps** di CPU quad-core kelas menengah (mis. Intel i5 generasi 8+).
- WPAP generation pada gambar 12MP harus selesai **≤3 detik** di hardware yang sama.
- Batch processing 100 foto resolusi 12MP dengan filter cinematic harus selesai **≤5 menit** (multi-threaded).
- Memory footprint aplikasi idle **≤150MB**; saat memproses gambar besar, gunakan *tiled processing* agar tidak melebihi 2GB RAM untuk gambar hingga 50MP.

### 6.4 Kompatibilitas Platform
| Platform | Versi Minimum | Catatan |
|---|---|---|
| Windows | Windows 10 64-bit | Instalasi via MSI/NSIS |
| macOS | macOS 12 (Monterey), Apple Silicon & Intel (Universal Binary) | Distribusi via DMG, code-signed & notarized |
| Linux | Ubuntu 22.04 LTS / distro setara (glibc 2.35+) | AppImage utama, paket .deb sekunder |

---

## 7. Pengalaman Pengguna (UX) — Alur Utama

### 7.1 User Flow: Membuat WPAP
1. Pengguna membuka aplikasi → drag foto ke kanvas.
2. Pilih tab "WPAP" di panel kiri.
3. Aplikasi otomatis menampilkan preview WPAP dengan setting default (deteksi wajah aktif jika ada wajah).
4. Pengguna menyesuaikan slider jumlah warna & detail; preview update real-time.
5. Pengguna klik "Export" → pilih format (PNG/SVG) → simpan.

### 7.2 User Flow: Apply Filter Cinematic
1. Pengguna membuka foto.
2. Pilih tab "Filters" → browse galeri preset (thumbnail preview tiap preset langsung di foto pengguna).
3. Klik preset "Teal & Orange" → preview ter-apply instan.
4. Sesuaikan intensity & grain via slider.
5. Klik "Save As New Preset" (opsional) atau langsung "Export".

### 7.3 User Flow: Batch Processing
1. Pengguna pilih menu "Batch" → pilih folder sumber.
2. Pilih satu preset (WPAP atau filter) yang akan diterapkan ke semua file.
3. Atur folder output & format penamaan.
4. Klik "Start" → progress bar muncul → notifikasi selesai.

---

## 8. Metrik Keberhasilan (Success Metrics)

| Kategori | Metrik | Target v1.0 |
|---|---|---|
| **Adopsi** | Jumlah unduhan aplikasi dalam 3 bulan pertama | 10.000 unduhan |
| **Engagement** | Rata-rata jumlah gambar diproses per pengguna aktif/bulan | ≥15 gambar |
| **Retensi** | Retensi pengguna aktif bulanan (MAU) bulan ke-2 | ≥35% |
| **Performa** | Waktu rata-rata generate WPAP (gambar 12MP) | ≤3 detik |
| **Performa** | FPS preview filter real-time | ≥24 fps |
| **Kualitas** | Crash rate per sesi | ≤0.5% |
| **Kepuasan** | Rating aplikasi (jika didistribusikan via store/marketplace) | ≥4.3/5.0 |
| **Kepuasan** | Net Promoter Score (NPS) dari survei in-app | ≥40 |
| **Fungsional** | Tingkat keberhasilan batch processing (selesai tanpa error) | ≥99% |

---

## 9. Roadmap Produk

### Fase 0 — Riset & Fondasi (Bulan 1–2)
- Setup arsitektur project (CMake, struktur modul, CI/CD).
- Riset & validasi algoritma WPAP (proof-of-concept Delaunay + color quant).
- Riset & kalibrasi LUT untuk 5 preset cinematic awal.
- Setup GUI skeleton (Qt) dengan kanvas dasar.

### Fase 1 — MVP Internal (Bulan 3–4)
- Implementasi WPAP module fungsional (tanpa face detection dulu).
- Implementasi color grading engine dasar (LUT + curve).
- Import/export gambar dasar (JPEG/PNG).
- Internal alpha testing oleh tim.

### Fase 2 — Beta Privat (Bulan 5–6)
- Tambah face detection untuk WPAP.
- Lengkapi 15 preset filter (Cinematic, Japan Style, lainnya).
- Non-destructive editing & history/undo.
- Batch processing.
- Closed beta dengan 50–100 pengguna terpilih (desainer, fotografer).
- Iterasi berdasarkan feedback (UX, bug, performa).

### Fase 3 — Rilis v1.0 Publik (Bulan 7)
- Polish UI/UX final, instalasi multi-platform (Windows/macOS/Linux).
- Dokumentasi pengguna & video tutorial singkat.
- Rilis publik (situs resmi, mungkin juga listing di platform seperti itch.io/Gumroad untuk distribusi awal).

### Fase 4 — Pasca-Rilis & Ekspansi (Bulan 8–12)
- v1.1: Color picker manual untuk WPAP, comparison grid, custom palette save.
- v1.2: CLI mode untuk automation.
- v1.3: Plugin system dasar (load preset/filter eksternal).
- Evaluasi kebutuhan fitur AI Style eksperimental (v2 candidate) berdasarkan permintaan pengguna.

### Fase 5 — Visi Jangka Panjang (Tahun 2+)
- v2.0: Mode AI Style opsional (ONNX-based, tetap offline).
- Eksplorasi dukungan video singkat.
- Eksplorasi versi mobile (kemungkinan menggunakan core engine C++ yang sama via JNI/Swift bridge).

---

## 10. Struktur Tim yang Direkomendasikan

| Peran | Jumlah | Tanggung Jawab Utama |
|---|---|---|
| Product Manager | 1 | Definisi requirement, prioritas roadmap, koordinasi stakeholder |
| C++ Engineer (Core/Image Processing) | 2 | Implementasi WPAP module, color grading engine, optimasi performa |
| C++/Qt Engineer (GUI) | 1–2 | Implementasi UI/UX, integrasi engine dengan frontend Qt |
| QA Engineer | 1 | Test plan, automated testing (GoogleTest), manual testing lintas platform |
| UI/UX Designer | 1 (paruh waktu/kontrak) | Desain wireframe, asset ikon, flow pengalaman pengguna |
| DevOps/Build Engineer | 1 (paruh waktu) | CI/CD, packaging multi-platform (CPack), code signing |
| QA/Beta Community Manager | 1 (paruh waktu) | Mengelola closed beta, mengumpulkan & triase feedback |

**Total estimasi tim inti: 6–8 orang** untuk timeline 7 bulan menuju v1.0.

---

## 11. Risiko & Mitigasi

| Risiko | Dampak | Kemungkinan | Mitigasi |
|---|---|---|---|
| Hasil WPAP otomatis tidak "terasa seni" seperti hasil manual desainer | Tinggi | Sedang | Sediakan kontrol manual granular (override warna per poligon, adjust titik) agar tetap ada sentuhan manual; lakukan user testing dengan desainer WPAP asli |
| Performa lambat pada gambar resolusi sangat besar | Sedang | Sedang | Implementasi tiled/chunked processing & multi-threading sejak awal, tetapkan limit resolusi yang wajar dengan opsi downsample otomatis |
| Lisensi library (CGAL GPL, dsb.) membatasi distribusi komersial | Tinggi | Sedang | Evaluasi lisensi sejak Fase 0; siapkan implementasi Delaunay sendiri (BSD/MIT) sebagai fallback agar bebas lisensi |
| Cross-platform build/packaging lebih kompleks dari estimasi | Sedang | Tinggi | Setup CI/CD multi-platform sejak Fase 0 (bukan di akhir), gunakan CMake + vcpkg/conan untuk dependency management konsisten |
| Kompetisi dari aplikasi mobile gratis (Snapseed, VSCO) | Sedang | Tinggi | Diferensiasi jelas: fokus WPAP otomatis (jarang ada kompetitor) + offline-first + kontrol granular untuk power user |
| Scope creep menambah fitur AI di v1 | Sedang | Sedang | Tegaskan non-goals di dokumen ini, AI Style eksplisit didorong ke v2+ |

---

## 12. Pertanyaan Terbuka (Open Questions)

1. Apakah aplikasi akan didistribusikan gratis, freemium (preset dasar gratis, preset premium berbayar), atau berbayar penuh (one-time purchase)?
2. Apakah perlu dukungan format RAW (CR2, NEF, ARW) untuk fotografer yang mengedit langsung dari kamera, atau cukup JPEG/PNG/TIFF di v1?
3. Apakah hasil WPAP perlu opsi export langsung ke template cetak (misal ukuran kanvas untuk merchandise mug/kaos)?
4. Berapa anggaran yang tersedia untuk lisensi font/asset UI premium (jika ada)?

---

## 13. Lampiran: Daftar Preset Filter v1.0 (Rincian)

| Nama Preset | Kategori | Karakteristik Utama |
|---|---|---|
| Teal & Orange | Cinematic | Shadow biru-teal, skin tone oranye hangat, kontras tinggi |
| Bleach Bypass | Cinematic | Desaturasi parsial, kontras ekstrem, highlight terang |
| Moody Blue | Cinematic | Dominan biru gelap, shadow pekat, mood dramatis |
| Film Noir | Cinematic | Hitam putih kontras tinggi, vignette kuat |
| Golden Hour | Cinematic | Warna hangat keemasan, highlight lembut, halation ringan |
| Mono no Aware | Japan Style | Pastel muted, kontras rendah, highlight kebiruan |
| Wong Kar-wai Neon Tokyo | Japan Style | Neon merah/hijau saturasi tinggi, shadow gelap, grain kasar |
| Showa Retro Film | Japan Style | Warna hangat film 80-90an, vignette, light leak halus |
| Anime Flat Look | Japan Style | Kontras tinggi, saturasi cerah, gradasi halus diposterize |
| Vintage Kodak | Vintage/Retro | Warm tone, grain halus, kontras sedang |
| Vintage Polaroid | Vintage/Retro | Putih dorong, warna pudar, vignette bulat |
| Black & White Fine Art | Monokrom | Tonal range lebar, sharpening halus, grain minim |
| HDR Dramatic | Lainnya | Dynamic range diperluas, detail shadow/highlight maksimal |
| Pastel Soft | Lainnya | Saturasi rendah, highlight terangkat, soft glow |
| Urban Street Gritty | Lainnya | Kontras tinggi, desaturasi sebagian, grain kasar, clarity tinggi |

---

*Dokumen ini adalah draft v1.0 dan akan diperbarui seiring berjalannya riset teknis lebih lanjut dan feedback dari stakeholder.*