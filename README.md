# PixelForge

**Desktop Image Style & Filter Editor** — C++20 / Qt 6 / OpenCV

PixelForge is a cross-platform desktop application for transforming photos into art styles and cinematic filters — completely offline, fast, and reproducible.

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![C++](https://img.shields.io/badge/C%2B%2B-20-brightgreen)
![License](https://img.shields.io/badge/license-MIT-yellow)

---

## Features

### 🎨 WPAP (Wedha's Pop Art Portrait)
- Automatic WPAP generation from a single click
- Edge-aware Delaunay triangulation with face detection
- Configurable: color count (6–32), detail level, palette presets
- Export as raster (PNG/JPEG) or vector (SVG)

### 🎬 Cinematic Color Grading (15 Built-in Presets)

| Cinematic | Japan Style | Vintage / Other |
|-----------|-------------|-----------------|
| Teal & Orange | Mono no Aware (Muted Pastel) | Vintage Kodak |
| Bleach Bypass | Wong Kar-wai Neon Tokyo | Vintage Polaroid |
| Moody Blue | Showa Retro Film | B&W Fine Art |
| Film Noir | Anime Flat Look | HDR Dramatic |
| Golden Hour | | Pastel Soft |
| | | Urban Street Gritty |

### 🔧 Fine-Tuning Controls
- Intensity, Grain, Vignette, Temperature, Tint
- Contrast, Brightness, Saturation, Highlights, Shadows
- Import custom `.cube` LUT files (Adobe/DaVinci standard)
- Save custom presets

### 📂 Non-Destructive Editing
- Unlimited undo/redo history
- Adjustment layers with reordering
- Save/load `.pforge` project files

### ⚡ Batch Processing
- Process hundreds of images with a single preset
- Multi-threaded parallel processing
- Custom naming patterns: `{name}_{preset}`, `{index}`
- Progress monitoring with per-file status

### 🖥️ UI/UX
- Single-window dark-themed workspace (light theme toggle)
- Split-view before/after comparison
- Zoom/pan canvas with mouse wheel + drag
- Drag & drop file opening
- Keyboard shortcuts (Ctrl+Z, Ctrl+S, etc.)

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│           Presentation Layer (Qt 6)             │
│  MainWindow · ImageCanvas · WapPanel            │
│  FilterPanel · BatchDialog                      │
└────────────────────┬────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────┐
│           Application Layer                     │
│  ProjectManager · HistoryManager · BatchProcessor│
└────────────────────┬────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────┐
│           Core Engine Layer (C++20)             │
│  WapModule · ColorGradingModule · IoModule      │
└────────────────────┬────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────┐
│           Foundation Layer                      │
│  OpenCV · DelaunayTriangulation · ColorQuant    │
│  LutEngine · ToneCurve · NoiseGenerator         │
└─────────────────────────────────────────────────┘
```

---

## Build from Source

### Prerequisites

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | ≥ 3.20 | Build system |
| Qt 6 | ≥ 6.4 | GUI framework |
| OpenCV | ≥ 4.x | Image processing (core, imgproc, photo, objdetect) |
| OpenMP | (optional) | Parallel processing acceleration |
| GoogleTest | (optional) | Unit testing |

### Using vcpkg (Recommended)

```bash
# Install vcpkg if not already installed
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat  # Windows
# ./vcpkg/bootstrap-vcpkg.sh  # Linux/macOS

# Install dependencies
./vcpkg install qt6 opencv4 gtest

# Build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Using System Packages (Linux)

```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev libopencv-dev libgtest-dev cmake build-essential

# Build
cmake -B build
cmake --build build
```

### Run

```bash
# GUI mode
./build/PixelForge

# CLI mode (future)
./build/PixelForge --cli -p cinematic_teal_orange -o output.png input.jpg
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

---

## Project Structure

```
pixelforge/
├── CMakeLists.txt              # Root build configuration
├── README.md
├── prd.md                      # Product Requirements Document
├── .gitignore
├── resources/
│   └── resources.qrc           # Qt resources
├── src/
│   ├── main.cpp                # Application entry point
│   ├── core/                   # Core engine modules
│   │   ├── CoreTypes.h         # Shared types & enums
│   │   ├── Image.h/cpp         # Image wrapper (cv::Mat)
│   │   ├── WapModule.h/cpp     # WPAP generation pipeline
│   │   ├── ColorGradingModule.h/cpp  # Filter & LUT pipeline
│   │   └── IoModule.h/cpp      # Image I/O & validation
│   ├── utils/                  # Processing utilities
│   │   ├── DelaunayTriangulation.h/cpp
│   │   ├── ColorQuantization.h/cpp
│   │   ├── LutEngine.h/cpp
│   │   ├── ToneCurve.h/cpp
│   │   └── NoiseGenerator.h/cpp
│   ├── app/                    # Application logic
│   │   ├── ProjectManager.h/cpp
│   │   ├── HistoryManager.h/cpp
│   │   └── BatchProcessor.h/cpp
│   └── gui/                    # Qt 6 GUI
│       ├── MainWindow.h/cpp
│       ├── ImageCanvas.h/cpp
│       ├── WapPanel.h/cpp
│       ├── FilterPanel.h/cpp
│       └── BatchDialog.h/cpp
└── tests/                      # GoogleTest unit tests
    ├── test_delaunay.cpp
    ├── test_color_quantization.cpp
    ├── test_lut_engine.cpp
    ├── test_noise_generator.cpp
    ├── test_tone_curve.cpp
    ├── test_image.cpp
    ├── test_history_manager.cpp
    └── test_batch_processor.cpp
```

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++20 |
| GUI | Qt 6 (Widgets, OpenGL) |
| Image Processing | OpenCV 4 |
| Geometry | Custom Bowyer-Watson Delaunay |
| Color Science | K-means++ in LAB space, 3D LUT with trilinear interpolation |
| Noise | Simplex noise with fBm |
| Build | CMake + CPack |
| Testing | GoogleTest |
| Package Manager | vcpkg / system packages |

---

## Performance Targets

| Metric | Target |
|--------|--------|
| WPAP generation (12MP) | ≤ 3 seconds |
| Filter preview (2MP) | ≥ 24 fps |
| Batch 100× 12MP photos | ≤ 5 minutes |
| Memory idle | ≤ 150 MB |
| Crash rate | ≤ 0.5% per session |

---

## Roadmap

- [x] **v1.0** — WPAP generator, 15 presets, batch processing, non-destructive editing
- [ ] **v1.1** — Color picker for WPAP polygons, comparison grid, crop/rotate
- [ ] **v1.2** — Full CLI mode for automation/scripting
- [ ] **v1.3** — Plugin system (dynamic library filters)
- [ ] **v2.0** — AI Style mode (ONNX Runtime, CPU-friendly)

---

## License

This project is licensed under the MIT License.

---

## Acknowledgments

- **Wedha Abdul Rasyid** — Creator of the WPAP art style
- **OpenCV** — Computer vision library
- **Qt** — Cross-platform GUI framework
- Inspired by cinematic color grading techniques from Hollywood and Japanese aesthetic movements