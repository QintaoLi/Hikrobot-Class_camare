#pragma once
#include "MvCameraControl.h"
#include <cstring>
#include <string>
#include <opencv2/opencv.hpp>

class Camera
{
public:
    // 构造相机对象。
    Camera();
    // 禁止拷贝导致相机对象共用一个句柄
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    // 初始化海康相机 SDK。
    bool initialize();
    // 枚举可用的 USB 相机设备。
    bool enumerate_devices();
    // 创建相机设备句柄。
    bool create_handle();
    // 打开相机设备。
    bool open_device();
    // 设置相机曝光和增益。
    bool set_exposure_and_gain();
    // 设置 USB 同步读写超时时间。
    bool set_sync_time_out();
    // 设置图像传输大小。
    bool set_transfer_size();
    // 设置 SDK 传输通道数量。
    bool set_transfer_ways();
    // 设置图像缓存节点数量。
    bool set_image_nodenum();
    // 开始采集图像。
    bool start_grabbing();
    // 获取并转换一帧图像。
    bool get_frame(cv::Mat& image);
    // 停止采集图像。
    bool stop_grabbing();
    // 关闭相机设备。
    bool close_device();
    // 销毁相机设备句柄。
    bool destroy_handle();
    // 生成带时间戳的保存文件名。
    std::string generate_file_name() const;
    // 释放相机相关资源。
    ~Camera();

private:
    void* m_handle = nullptr;
    bool m_sdk_initialized = false;
    bool m_device_opened = false;
    bool m_grabbing = false;
    bool m_bayer_quality_set = false;
    MV_CC_DEVICE_INFO_LIST m_device_list{};
};
