#pragma once

#include "stages/stage.hpp"

class PreprocessStage : public Stage {
public:
    explicit PreprocessStage(int blur_kernel = 5, double scale = 1.0);

    void process(const cv::Mat& input, cv::Mat& output) override;
    std::string name() const override { return "Preprocessing"; }

private:
    int blur_kernel_;
    double scale_;
};
