#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class Profiler {
public:
    void start(const std::string& label);
    void stop(const std::string& label);

    void recordFrame(double frame_time_ms);
    void setWallClockMs(double total_ms, int frame_count);
    void reset();

    double fps() const;
    double avgFrameTimeMs() const;
    std::map<std::string, double> stageAverages() const;

    void printReport(const std::string& title) const;

private:
    using Clock = std::chrono::high_resolution_clock;

    mutable std::mutex mutex_;
    std::map<std::string, Clock::time_point> active_;
    std::map<std::string, std::vector<double>> stage_times_ms_;
    std::vector<double> frame_times_ms_;
    double wall_clock_ms_ = 0.0;
    int wall_clock_frames_ = 0;
};
