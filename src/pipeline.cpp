#include "pipeline.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>

Pipeline::Pipeline()
    : preprocess_(std::make_unique<PreprocessStage>()),
      edge_detection_(std::make_unique<EdgeDetectionStage>()),
      object_detection_(std::make_unique<ObjectDetectionStage>()) {}

void Pipeline::processFrameSingle(const cv::Mat& input, PipelineFrame& frame, Profiler& profiler) {
    frame.input = input;

    profiler.start(preprocess_->name());
    preprocess_->process(frame.input, frame.preprocessed);
    profiler.stop(preprocess_->name());

    profiler.start(edge_detection_->name());
    edge_detection_->process(frame.preprocessed, frame.edges);
    profiler.stop(edge_detection_->name());

    cv::Mat detection_vis;
    profiler.start(object_detection_->name());
    object_detection_->process(frame.edges, detection_vis);
    profiler.stop(object_detection_->name());

    frame.detections = object_detection_->lastDetections();
    frame.output = renderOutput(frame.input, frame.edges, frame.detections);
}

cv::Mat Pipeline::renderOutput(const cv::Mat& original, const cv::Mat& edges,
                               const std::vector<DetectedObject>& detections) const {
    cv::Mat display;
    if (original.channels() == 1) {
        cv::cvtColor(original, display, cv::COLOR_GRAY2BGR);
    } else {
        display = original.clone();
    }

    cv::Mat edges_color;
    cv::cvtColor(edges, edges_color, cv::COLOR_GRAY2BGR);

    for (const auto& obj : detections) {
        cv::rectangle(display, obj.bounding_box, cv::Scalar(0, 255, 0), 2);
        const std::string label = "obj " + std::to_string(static_cast<int>(obj.area));
        cv::putText(display, label, obj.bounding_box.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 255, 0), 1);
    }

    if (display.cols != edges_color.cols || display.rows != edges_color.rows) {
        cv::resize(edges_color, edges_color, display.size());
    }

    cv::Mat combined;
    cv::hconcat(display, edges_color, combined);
    return combined;
}

PipelineResult Pipeline::runSingleThread(const std::vector<cv::Mat>& frames, Profiler& profiler) {
    PipelineResult result;
    if (frames.empty()) {
        return result;
    }

    const auto pipeline_start = std::chrono::high_resolution_clock::now();

    PipelineFrame frame;
    for (const auto& input : frames) {
        const auto frame_start = std::chrono::high_resolution_clock::now();

        processFrameSingle(input, frame, profiler);

        const auto frame_end = std::chrono::high_resolution_clock::now();
        const double frame_ms =
            std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
        profiler.recordFrame(frame_ms);
        ++result.frames_processed;
    }

    const auto pipeline_end = std::chrono::high_resolution_clock::now();
    const double total_ms =
        std::chrono::duration<double, std::milli>(pipeline_end - pipeline_start).count();
    profiler.setWallClockMs(total_ms, result.frames_processed);

    result.output = frame.output;
    result.detections = frame.detections;
    return result;
}

PipelineResult Pipeline::runMultiThread(const std::vector<cv::Mat>& frames, Profiler& profiler,
                                          int num_worker_threads) {
    PipelineResult result;
    if (frames.empty()) {
        return result;
    }

    ThreadSafeQueue<PipelineFrame> input_queue;
    ThreadSafeQueue<PipelineFrame> preprocess_queue;
    ThreadSafeQueue<PipelineFrame> edge_queue;
    ThreadSafeQueue<PipelineFrame> output_queue;

    std::atomic<int> frames_completed{0};
    PipelineFrame last_frame;

    auto preprocess_worker = [&] {
        PipelineFrame frame;
        while (input_queue.pop(frame)) {
            profiler.start(preprocess_->name());
            preprocess_->process(frame.input, frame.preprocessed);
            profiler.stop(preprocess_->name());
            preprocess_queue.push(std::move(frame));
        }
        preprocess_queue.setDone();
    };

    auto edge_worker = [&] {
        PipelineFrame frame;
        while (preprocess_queue.pop(frame)) {
            profiler.start(edge_detection_->name());
            edge_detection_->process(frame.preprocessed, frame.edges);
            profiler.stop(edge_detection_->name());
            edge_queue.push(std::move(frame));
        }
        edge_queue.setDone();
    };

    auto object_worker = [&] {
        PipelineFrame frame;
        while (edge_queue.pop(frame)) {
            cv::Mat detection_vis;
            profiler.start(object_detection_->name());
            object_detection_->process(frame.edges, detection_vis);
            profiler.stop(object_detection_->name());

            frame.detections = object_detection_->lastDetections();
            frame.output = renderOutput(frame.input, frame.edges, frame.detections);
            output_queue.push(std::move(frame));
        }
        output_queue.setDone();
    };

    auto output_worker = [&] {
        PipelineFrame frame;
        while (output_queue.pop(frame)) {
            last_frame = frame;
            ++frames_completed;
        }
    };

    const auto pipeline_start = std::chrono::high_resolution_clock::now();

    std::thread preprocess_thread(preprocess_worker);
    std::thread edge_thread(edge_worker);
    std::thread object_thread(object_worker);
    std::thread output_thread(output_worker);

    for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
        PipelineFrame frame;
        frame.input = frames[static_cast<size_t>(i)];
        frame.frame_id = i;
        input_queue.push(std::move(frame));
    }
    input_queue.setDone();

    preprocess_thread.join();
    edge_thread.join();
    object_thread.join();
    output_thread.join();

    const auto pipeline_end = std::chrono::high_resolution_clock::now();
    const double total_ms =
        std::chrono::duration<double, std::milli>(pipeline_end - pipeline_start).count();

    profiler.setWallClockMs(total_ms, frames_completed);
    result.frames_processed = frames_completed;
    result.output = last_frame.output;
    result.detections = last_frame.detections;
    (void)num_worker_threads;

    return result;
}

void Pipeline::saveOutput(const cv::Mat& image, const std::string& path) const {
    cv::imwrite(path, image);
}
