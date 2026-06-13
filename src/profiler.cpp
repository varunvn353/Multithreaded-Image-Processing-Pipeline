#include "profiler.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>

void Profiler::start(const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_[label] = Clock::now();
}

void Profiler::stop(const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_.find(label);
    if (it == active_.end()) {
        return;
    }

    const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - it->second).count();
    stage_times_ms_[label].push_back(elapsed);
    active_.erase(it);
}

void Profiler::recordFrame(double frame_time_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_times_ms_.push_back(frame_time_ms);
}

void Profiler::setWallClockMs(double total_ms, int frame_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    wall_clock_ms_ = total_ms;
    wall_clock_frames_ = frame_count;
}

void Profiler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_.clear();
    stage_times_ms_.clear();
    frame_times_ms_.clear();
    wall_clock_ms_ = 0.0;
    wall_clock_frames_ = 0;
}

double Profiler::fps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (wall_clock_frames_ > 0 && wall_clock_ms_ > 0.0) {
        return (static_cast<double>(wall_clock_frames_) / wall_clock_ms_) * 1000.0;
    }
    if (frame_times_ms_.empty()) {
        return 0.0;
    }
    const double total = std::accumulate(frame_times_ms_.begin(), frame_times_ms_.end(), 0.0);
    const double avg_ms = total / static_cast<double>(frame_times_ms_.size());
    return avg_ms > 0.0 ? 1000.0 / avg_ms : 0.0;
}

double Profiler::avgFrameTimeMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (wall_clock_frames_ > 0 && wall_clock_ms_ > 0.0) {
        return wall_clock_ms_ / static_cast<double>(wall_clock_frames_);
    }
    if (frame_times_ms_.empty()) {
        return 0.0;
    }
    const double total = std::accumulate(frame_times_ms_.begin(), frame_times_ms_.end(), 0.0);
    return total / static_cast<double>(frame_times_ms_.size());
}

std::map<std::string, double> Profiler::stageAverages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, double> averages;

    for (const auto& [label, times] : stage_times_ms_) {
        if (times.empty()) {
            continue;
        }
        const double total = std::accumulate(times.begin(), times.end(), 0.0);
        averages[label] = total / static_cast<double>(times.size());
    }

    return averages;
}

void Profiler::printReport(const std::string& title) const {
    const auto averages = stageAverages();

    std::cout << "\n=== " << title << " ===\n";
    std::cout << std::fixed << std::setprecision(2);

    double fps_value = 0.0;
    double avg_frame_ms = 0.0;
    int frame_count = 0;
    double wall_ms = 0.0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        frame_count = wall_clock_frames_ > 0 ? wall_clock_frames_
                                             : static_cast<int>(frame_times_ms_.size());

        if (wall_clock_frames_ > 0 && wall_clock_ms_ > 0.0) {
            fps_value = (static_cast<double>(wall_clock_frames_) / wall_clock_ms_) * 1000.0;
            avg_frame_ms = wall_clock_ms_ / static_cast<double>(wall_clock_frames_);
        } else if (!frame_times_ms_.empty()) {
            const double total =
                std::accumulate(frame_times_ms_.begin(), frame_times_ms_.end(), 0.0);
            avg_frame_ms = total / static_cast<double>(frame_times_ms_.size());
            fps_value = avg_frame_ms > 0.0 ? 1000.0 / avg_frame_ms : 0.0;
        }

        wall_ms = wall_clock_ms_;
    }

    std::cout << "Frames processed : " << frame_count << "\n";
    std::cout << "Average FPS      : " << fps_value << "\n";
    std::cout << "Avg frame time   : " << avg_frame_ms << " ms\n";

    if (wall_ms > 0.0) {
        std::cout << "Wall-clock time  : " << wall_ms << " ms\n";
    }

    if (!averages.empty()) {
        std::cout << "\nPer-stage timing (avg ms):\n";
        for (const auto& [label, avg] : averages) {
            std::cout << "  " << std::left << std::setw(20) << label << ": " << avg << " ms\n";
        }
    }
}
