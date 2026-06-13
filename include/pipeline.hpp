#pragma once

#include "profiler.hpp"
#include "stages/edge_detection.hpp"
#include "stages/object_detection.hpp"
#include "stages/preprocess.hpp"
#include "stages/stage.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct PipelineFrame {
    cv::Mat input;
    cv::Mat preprocessed;
    cv::Mat edges;
    cv::Mat output;
    std::vector<DetectedObject> detections;
    int frame_id = 0;
};

struct PipelineResult {
    cv::Mat output;
    std::vector<DetectedObject> detections;
    int frames_processed = 0;
};

enum class PipelineMode { SingleThread, MultiThread };

class Pipeline {
public:
    Pipeline();

    PipelineResult runSingleThread(const std::vector<cv::Mat>& frames, Profiler& profiler);
    PipelineResult runMultiThread(const std::vector<cv::Mat>& frames, Profiler& profiler,
                                  int num_worker_threads = 4);

    void saveOutput(const cv::Mat& image, const std::string& path) const;

private:
    std::unique_ptr<PreprocessStage> preprocess_;
    std::unique_ptr<EdgeDetectionStage> edge_detection_;
    std::unique_ptr<ObjectDetectionStage> object_detection_;

    void processFrameSingle(const cv::Mat& input, PipelineFrame& frame, Profiler& profiler);
    cv::Mat renderOutput(const cv::Mat& original, const cv::Mat& edges,
                         const std::vector<DetectedObject>& detections) const;
};

template <typename T>
class ThreadSafeQueue {
public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || done_; });
        if (queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void setDone() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            done_ = true;
        }
        cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        done_ = false;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool done_ = false;
};
