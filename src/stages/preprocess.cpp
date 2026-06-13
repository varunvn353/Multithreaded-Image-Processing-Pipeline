#include "stages/preprocess.hpp"

#include <opencv2/imgproc.hpp>

PreprocessStage::PreprocessStage(int blur_kernel, double scale)
    : blur_kernel_(blur_kernel), scale_(scale) {}

void PreprocessStage::process(const cv::Mat& input, cv::Mat& output) {
    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input;
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(blur_kernel_, blur_kernel_), 0);

    if (scale_ != 1.0) {
        cv::resize(blurred, output, cv::Size(), scale_, scale_, cv::INTER_LINEAR);
    } else {
        output = blurred;
    }

    cv::equalizeHist(output, output);
}
