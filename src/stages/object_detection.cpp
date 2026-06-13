#include "stages/object_detection.hpp"

#include <opencv2/imgproc.hpp>

ObjectDetectionStage::ObjectDetectionStage(int min_area, int max_area)
    : min_area_(min_area), max_area_(max_area) {}

void ObjectDetectionStage::process(const cv::Mat& input, cv::Mat& output) {
    detections_.clear();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(input, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat color_input;
    cv::cvtColor(input, color_input, cv::COLOR_GRAY2BGR);
    output = color_input.clone();

    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < min_area_ || area > max_area_) {
            continue;
        }

        const cv::Rect box = cv::boundingRect(contour);
        detections_.push_back({box, area});
        cv::rectangle(output, box, cv::Scalar(0, 255, 0), 2);
    }
}
