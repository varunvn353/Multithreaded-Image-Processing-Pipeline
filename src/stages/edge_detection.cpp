#include "stages/edge_detection.hpp"

#include <opencv2/imgproc.hpp>

EdgeDetectionStage::EdgeDetectionStage(double low_threshold, double high_threshold)
    : low_threshold_(low_threshold), high_threshold_(high_threshold) {}

void EdgeDetectionStage::process(const cv::Mat& input, cv::Mat& output) {
    cv::Mat blurred;
    cv::GaussianBlur(input, blurred, cv::Size(3, 3), 0);
    cv::Canny(blurred, output, low_threshold_, high_threshold_);
}
