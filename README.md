# Multithreaded Image Processing Pipeline

A C++ image processing pipeline demonstrating OOP design, `std::thread` concurrency, and performance profiling with OpenCV.

## Pipeline

```
Input Image
    ↓
Preprocessing   (grayscale, blur, histogram equalization)
    ↓
Edge Detection  (Canny)
    ↓
Object Detection (contour-based bounding boxes)
    ↓
Output          (annotated visualization)
```

## Skills Demonstrated

- **C++ / OOP** — `Stage` base class with specialized stage implementations
- **Multithreading** — producer-consumer pipeline with `std::thread` and thread-safe queues
- **Performance** — FPS measurement, per-stage profiling, single vs multi-thread comparison

## Requirements

- CMake 3.16+
- C++17 compiler (MSVC, GCC, or Clang)
- [OpenCV](https://opencv.org/) 4.x

### Windows (vcpkg)

```powershell
vcpkg install opencv4:x64-windows
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Linux

```bash
sudo apt install libopencv-dev
cmake -B build -S .
cmake --build build
```

## Build & Run

```powershell
cmake -B build -S .
cmake --build build --config Release
.\build\Release\image_pipeline.exe
```

```bash
cmake -B build -S .
cmake --build build
./build/image_pipeline
```

## Usage

```text
image_pipeline [options]
  --image <path>     Input image file (default: generated test pattern)
  --video <path>     Input video file
  --camera <index>   Use webcam (default index: 0)
  --frames <n>       Number of frames to benchmark (default: 100)
  --output <path>    Save final output image (default: output/result.png)
  --help             Show help
```

### Examples

```powershell
# Benchmark with built-in test pattern
.\build\Release\image_pipeline.exe --frames 200

# Process a static image
.\build\Release\image_pipeline.exe --image data/sample.jpg --frames 150

# Process a video file
.\build\Release\image_pipeline.exe --video data/sample.mp4 --frames 300
```

## Architecture

| Component | Description |
|-----------|-------------|
| `Stage` | Abstract base class for pipeline stages |
| `PreprocessStage` | Grayscale conversion, Gaussian blur, histogram equalization |
| `EdgeDetectionStage` | Canny edge detector |
| `ObjectDetectionStage` | Contour analysis with area filtering |
| `Pipeline` | Orchestrates single-thread and multi-thread execution |
| `Profiler` | Thread-safe timing and FPS reporting |
| `ThreadSafeQueue` | Bounded producer-consumer queue between stage threads |

### Single-Thread Mode

Each frame passes through all stages sequentially on one thread. Per-stage and per-frame timings are recorded.

### Multi-Thread Mode

Four worker threads form an assembly-line pipeline:

1. **Preprocess thread** — reads input queue, writes preprocessed frames
2. **Edge thread** — runs Canny on preprocessed frames
3. **Object thread** — finds contours and draws bounding boxes
4. **Output thread** — collects final frames

Stages overlap across frames, improving throughput on multi-core CPUs.

## Output

The program prints:

- Frames processed
- Average FPS (wall-clock throughput)
- Per-stage average latency (ms)
- Single-thread vs multi-thread speedup

A side-by-side output image is saved showing detected objects and edge map.

## Project Structure

```
project1/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── pipeline.hpp
│   ├── profiler.hpp
│   └── stages/
│       ├── stage.hpp
│       ├── preprocess.hpp
│       ├── edge_detection.hpp
│       └── object_detection.hpp
└── src/
    ├── main.cpp
    ├── pipeline.cpp
    ├── profiler.cpp
    └── stages/
        ├── preprocess.cpp
        ├── edge_detection.cpp
        └── object_detection.cpp
```
