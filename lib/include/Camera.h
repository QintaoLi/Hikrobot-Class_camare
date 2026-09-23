#pragma once
#include "MvCameraControl.h"
#include <opencv2/opencv.hpp>
#include <cstring>
#include <string>

class Camera
{
public:
    Camera();
    bool initialize();
    bool enumerate_devices();
    bool create_handle();
    bool open_device();
    bool set_exposure_and_gain();
    bool set_sync_time_out();
    bool set_transfer_size();
    bool set_transfer_ways();
    bool set_image_nodenum();
    bool start_grabbing();
    bool get_frame(cv::Mat& image);
    bool stop_grabbing();
    bool close_device();
    bool destroy_handle();
    std::string generate_file_name() const;
    ~Camera();

private:
    void* handle = nullptr;
    bool sdk_initialized = false;
    bool device_opened = false;
    bool grabbing = false;
    bool bayer_quality_set = false;
    MV_CC_DEVICE_INFO_LIST device_list{};
};
