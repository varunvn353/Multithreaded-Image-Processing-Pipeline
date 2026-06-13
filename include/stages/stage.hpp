#pragma once

#include <opencv2/core.hpp>
#include <string>

class Stage {
public:
    virtual ~Stage() = default;
    virtual void process(const cv::Mat& input, cv::Mat& output) = 0;
    virtual std::string name() const = 0;
};
