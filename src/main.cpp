#include "pipeline.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "  --image <path>     Input image file (default: generated test pattern)\n"
              << "  --video <path>     Input video file\n"
              << "  --camera <index>   Use webcam (default index: 0)\n"
              << "  --frames <n>       Number of frames to benchmark (default: 100)\n"
              << "  --output <path>    Save final output image (default: output/result.png)\n"
              << "  --help             Show this help message\n";
}

cv::Mat generateTestImage(int width = 960, int height = 540) {
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(30, 30, 30));

    for (int y = 0; y < height; y += 40) {
        cv::line(image, cv::Point(0, y), cv::Point(width, y), cv::Scalar(60, 60, 60), 1);
    }
    for (int x = 0; x < width; x += 40) {
        cv::line(image, cv::Point(x, 0), cv::Point(x, height), cv::Scalar(60, 60, 60), 1);
    }

    cv::rectangle(image, cv::Rect(120, 80, 220, 180), cv::Scalar(0, 180, 255), -1);
    cv::circle(image, cv::Point(620, 260), 90, cv::Scalar(80, 220, 80), -1);
    cv::ellipse(image, cv::Point(420, 360), cv::Size(140, 70), 0, 0, 360, cv::Scalar(200, 80, 80), -1);
    cv::putText(image, "Pipeline Benchmark", cv::Point(40, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                cv::Scalar(255, 255, 255), 2);

    return image;
}

std::vector<cv::Mat> loadFrames(const std::string& image_path, const std::string& video_path,
                                int camera_index, int frame_count) {
    std::vector<cv::Mat> frames;
    frames.reserve(static_cast<size_t>(frame_count));

    if (!video_path.empty()) {
        cv::VideoCapture cap(video_path);
        if (!cap.isOpened()) {
            throw std::runtime_error("Failed to open video: " + video_path);
        }

        cv::Mat frame;
        for (int i = 0; i < frame_count && cap.read(frame); ++i) {
            frames.push_back(frame.clone());
        }
        return frames;
    }

    if (camera_index >= 0) {
        cv::VideoCapture cap(camera_index);
        if (!cap.isOpened()) {
            throw std::runtime_error("Failed to open camera index: " + std::to_string(camera_index));
        }

        cv::Mat frame;
        for (int i = 0; i < frame_count && cap.read(frame); ++i) {
            frames.push_back(frame.clone());
        }
        return frames;
    }

    cv::Mat source;
    if (!image_path.empty()) {
        source = cv::imread(image_path, cv::IMREAD_COLOR);
        if (source.empty()) {
            throw std::runtime_error("Failed to load image: " + image_path);
        }
    } else {
        source = generateTestImage();
        std::cout << "No input provided. Using generated test pattern (" << source.cols << "x"
                  << source.rows << ").\n";
    }

    for (int i = 0; i < frame_count; ++i) {
        frames.push_back(source.clone());
    }
    return frames;
}

void printComparison(double single_fps, double multi_fps, double speedup) {
    std::cout << "\n=== Performance Comparison ===\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Single-thread FPS : " << single_fps << "\n";
    std::cout << "Multi-thread FPS  : " << multi_fps << "\n";
    std::cout << "Speedup           : " << speedup << "x\n";

    if (speedup > 1.05) {
        std::cout << "Result: Multithreaded pipeline is faster.\n";
    } else if (speedup < 0.95) {
        std::cout << "Result: Single-threaded mode is faster (overhead dominates at this workload).\n";
    } else {
        std::cout << "Result: Performance is similar for this workload.\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string image_path;
    std::string video_path;
    std::string output_path = "output/result.png";
    int camera_index = -1;
    int frame_count = 100;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--image" && i + 1 < argc) {
            image_path = argv[++i];
        } else if (arg == "--video" && i + 1 < argc) {
            video_path = argv[++i];
        } else if (arg == "--camera") {
            camera_index = (i + 1 < argc) ? std::stoi(argv[++i]) : 0;
        } else if (arg == "--frames" && i + 1 < argc) {
            frame_count = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        const auto frames = loadFrames(image_path, video_path, camera_index, frame_count);
        if (frames.empty()) {
            std::cerr << "No frames available for processing.\n";
            return 1;
        }

        std::cout << "Loaded " << frames.size() << " frame(s) for benchmarking.\n";
        std::cout << "Pipeline: Preprocessing -> Edge Detection -> Object Detection -> Output\n";

        Pipeline pipeline;

        Profiler single_profiler;
        const auto single_result = pipeline.runSingleThread(frames, single_profiler);
        single_profiler.printReport("Single-Thread Profile");

        Profiler multi_profiler;
        const auto multi_result = pipeline.runMultiThread(frames, multi_profiler);
        multi_profiler.printReport("Multi-Thread Profile");

        const double speedup =
            single_profiler.fps() > 0.0 ? multi_profiler.fps() / single_profiler.fps() : 0.0;
        printComparison(single_profiler.fps(), multi_profiler.fps(), speedup);

        std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
        pipeline.saveOutput(multi_result.output, output_path);

        std::cout << "\nDetected objects (last frame): " << multi_result.detections.size() << "\n";
        std::cout << "Output saved to: " << output_path << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
