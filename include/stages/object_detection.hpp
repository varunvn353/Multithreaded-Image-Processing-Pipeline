#pragma once
#include "stages/stage.hpp"
#include <vector>
struct DetectedObject {
    cv::Rect bounding_box;
    double area;
};
class ObjectDetectionStage : public Stage {
public:
    explicit ObjectDetectionStage(int min_area = 500, int max_area = 50000);
    void process(const cv::Mat& input, cv::Mat& output) override;
    std::string name() const override { return "Object Detection"; }
    const std::vector<DetectedObject>& lastDetections() const { return detections_; }
private:
    int min_area_;
    int max_area_;
    std::vector<DetectedObject> detections_;
};
