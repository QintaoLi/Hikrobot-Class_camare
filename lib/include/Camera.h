#pragma once
#include "MvCameraControl.h"
#include <iostream>
#include <cstring> 
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <stdexcept>
#include <vector>
class Camera
{
public:
    Camera();
    void Enumerate_Devices();
    void Create_Handle();
    void Open_Device();
    void Set_Exposure_And_Gain();
    void Set_Sync_Time_Out();
    void Set_Transfer_Size();
    void Set_Transfer_Ways();
    void Set_Image_Nodenum();
    void Start_Grabbing();
    bool Get_Frame(cv::Mat& image);
    void Stop_Grabbing();
    void Close_Device();
    void Destroy_Handle();
    ~Camera(); 
    std::string GenerateFileName();
private:
    // 私有成员变量和方法
    void* handle = nullptr; // 相机句柄，初始化为 nullptr
    bool sdkInitialized = false;
    bool deviceOpened = false;
    bool grabbing = false;
    bool m_bBayerQualitySet = false;
    MV_CC_DEVICE_INFO_LIST stDeviceList;
};