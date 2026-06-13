#pragma once

#include "stages/stage.hpp"

class EdgeDetectionStage : public Stage {
public:
    explicit EdgeDetectionStage(double low_threshold = 50.0, double high_threshold = 150.0);

    void process(const cv::Mat& input, cv::Mat& output) override;
    std::string name() const override { return "Edge Detection"; }

private:
    double low_threshold_;
    double high_threshold_;
};
