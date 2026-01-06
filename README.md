# Filter

![Platform](https://img.shields.io/badge/platform-Windows%20Only-0078D6) ![License](https://img.shields.io/badge/license-MIT-blue) ![Language](https://img.shields.io/badge/language-C++17-orange) ![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-ff69b4)

**Filter** is a high-performance image processing tool designed exclusively for Windows. It features a hardware-accelerated GUI built with **OpenGL**, **GLFW**, and **Dear ImGui** for real-time image manipulation.

The project is structured with a modular architecture, separating the core filtering engine from the UI, and includes an automated speed test suite for batch processing.

---

## 📸 Features

* **Interactive GUI:** Real-time feedback and parameter tuning using Dear ImGui.
* **Format Support:** Fast loading and saving of common image formats via `stb_image`.
* **Multithreading:** Core engine utilizes efficient threading for pixel operations.
* **Dual Build Targets:**
    * `FilterUI.exe`: Main application window (Console output suppressed).
    * `SpeedTest.exe`: Headless benchmark tool for performance analysis.
* **Current Filters:**
    * [x] Grayscale
    * [x] Sepia
    * [x] Invert Color

---

## 🛠️ Tech Stack

* **Platform:** Windows (Required)
* **Language:** C++17
* **Build System:** CMake (3.14+)
* **Dependencies (Included):**
    * GLFW (3.4)
    * Dear ImGui (1.92.5)
    * stb_image
* **Graphics:** OpenGL 3+

---

## 🚀 Building the Project

The project is self-contained. All dependencies are located in the `dependencies/` folder and are built automatically from source during compilation.

### Prerequisites

* **Windows OS**
* **C++ Compiler** (MSVC / Visual Studio 2019+ recommended)
* **CMake** (Version 3.14 or higher)

### Compilation Steps

1.  **Clone the repository:**
    ```powershell
    git clone [https://github.com/MBatuhanOzer/filter.git](https://github.com/MBatuhanOzer/filter.git)
    cd filter
    ```

2.  **Create a build directory:**
    ```powershell
    mkdir build
    cd build
    ```

3.  **Generate build files:**
    ```powershell
    cmake ..
    ```

4.  **Compile:**
    ```powershell
    cmake --build . --config Release
    ```

### Output Location
Upon successful compilation, the executable files are generated in the `out/` directory at the project root:
* `project_root/out/Release/FilterUI.exe`
* `project_root/out/Release/SpeedTest.exe`

*(Note: Depending on your specific generator, they might appear directly inside `out/` or inside a configuration folder like `out/Debug/`)*

---

## 💻 Usage

### 1. Graphical Interface (`FilterUI`)
Run the main application to interactively load images and apply filters using the windowed interface.

```powershell
.\out\Release\FilterUI.exe
```

### 2. Automated Speed Test & Batch Processing
The project includes a batch script to automate benchmarking and process large sets of images.

**The Workflow:**
1.  The script locates the compiled `SpeedTest.exe`.
2.  It relocates the executable to the test environment.
3.  It scans the `tests/images/input/` directory.
4.  It applies **every implemented filter** to **every input image**.
5.  Processed images are saved to `tests/images/output/`.

**How to run:**
Navigate to the `tests/` folder and execute the batch file:

```powershell
cd tests
.\runtests.bat
```
---

## 📂 Project Structure

```text
├── src/
│   ├── filter-engine/       # Core image processing library (Static Lib)
│   │   ├── filter.cpp
│   │   └── filter.h
│   └── UI/                  # GUI Application Source
│       └── main.cpp
├── tests/
│   ├── images/
│   │   ├── input/           # Place benchmark images here
│   │   └── output/          # Results are generated here
│   ├── filter_engine_speedtest.cpp
│   └── run_tests.bat        # Automated test script
├── dependencies/            # Third-party libraries (GLFW, ImGui, stb)
├── out/                     # Compiled executables (generated)
├── CMakeLists.txt           # Main Build Configuration
└── README.md
```
---
## 👤 Author

**Batuhan Ozer**
* GitHub: [MBatuhanOzer](https://github.com/MBatuhanOzer)
