#include "Camera.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>

Camera::Camera() = default;

bool Camera::initialize()
{
    if (sdk_initialized)
        return true;
    std::memset(&device_list, 0, sizeof(device_list));
    const int ret = MV_CC_Initialize();
    if (ret != MV_OK)
    {
        std::cerr << "SDK 初始化失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    sdk_initialized = true;
    return true;
}

bool Camera::enumerate_devices()
{
    const int ret = MV_CC_EnumDevices(MV_USB_DEVICE, &device_list);
    if (ret != MV_OK || device_list.nDeviceNum == 0)
    {
        std::cerr << (ret != MV_OK ? "设备枚举失败" : "没有发现相机") << std::endl;
        return false;
    }
    return true;
}

bool Camera::create_handle()
{
    if (device_list.nDeviceNum == 0)
        return false;
    const int ret = MV_CC_CreateHandle(&handle, device_list.pDeviceInfo[0]);
    if (ret != MV_OK)
    {
        std::cerr << "创建相机句柄失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::open_device()
{
    const int ret = MV_CC_OpenDevice(handle);
    if (ret != MV_OK)
    {
        std::cerr << "打开相机失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    device_opened = true;
    return true;
}

bool Camera::set_exposure_and_gain()
{
    int ret = MV_CC_SetEnumValue(handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    if (ret == MV_OK) ret = MV_CC_SetFloatValue(handle, "ExposureTime", 60000.0f);
    if (ret == MV_OK) ret = MV_CC_SetEnumValue(handle, "GainAuto", MV_GAIN_MODE_OFF);
    if (ret == MV_OK) ret = MV_CC_SetFloatValue(handle, "Gain", 0.0f);
    if (ret != MV_OK)
    {
        std::cerr << "设置曝光或增益失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::set_sync_time_out()
{
    const int ret = MV_USB_SetSyncTimeOut(handle, 1000);
    if (ret != MV_OK)
    {
        std::cerr << "设置同步超时失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::set_transfer_size()
{
    const int ret = MV_USB_SetTransferSize(handle, 1 * 1024 * 1024);
    if (ret != MV_OK)
    {
        std::cerr << "设置传输大小失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::set_transfer_ways()
{
    const int ret = MV_USB_SetTransferWays(handle, 2);
    if (ret != MV_OK)
    {
        std::cerr << "设置传输方式失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::set_image_nodenum()
{
    const int ret = MV_CC_SetImageNodeNum(handle, 5);
    if (ret != MV_OK)
    {
        std::cerr << "设置图像缓存节点数失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

bool Camera::start_grabbing()
{
    const int ret = MV_CC_StartGrabbing(handle);
    if (ret != MV_OK)
    {
        std::cerr << "开始抓图失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    grabbing = true;
    return true;
}

bool Camera::get_frame(cv::Mat& image)
{
    if (!bayer_quality_set)
    {
        const int ret = MV_CC_SetBayerCvtQuality(handle, 1);
        if (ret != MV_OK)
            return false;
        bayer_quality_set = true;
    }

    MV_FRAME_OUT frame{};
    int ret = MV_CC_GetImageBuffer(handle, &frame, 100);
    if (ret != MV_OK)
        return false;

    const auto& info = frame.stFrameInfo;
    cv::Mat result(info.nHeight, info.nWidth, CV_8UC3);
    MV_CC_PIXEL_CONVERT_PARAM_EX param{};
    param.nWidth = info.nWidth;
    param.nHeight = info.nHeight;
    param.enSrcPixelType = info.enPixelType;
    param.pSrcData = frame.pBufAddr;
    param.nSrcDataLen = info.nFrameLen;
    param.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    param.pDstBuffer = result.data;
    param.nDstBufferSize = static_cast<unsigned int>(result.total() * result.elemSize());

    ret = MV_CC_ConvertPixelTypeEx(handle, &param);
    const int free_ret = MV_CC_FreeImageBuffer(handle, &frame);
    if (ret != MV_OK || free_ret != MV_OK)
        return false;

    image = result;
    return true;
}

bool Camera::stop_grabbing()
{
    if (!grabbing)
        return true;
    const int ret = MV_CC_StopGrabbing(handle);
    if (ret != MV_OK)
        return false;
    grabbing = false;
    return true;
}

bool Camera::close_device()
{
    if (!device_opened)
        return true;
    const int ret = MV_CC_CloseDevice(handle);
    if (ret != MV_OK)
        return false;
    device_opened = false;
    return true;
}

bool Camera::destroy_handle()
{
    if (handle == nullptr)
        return true;
    const int ret = MV_CC_DestroyHandle(handle);
    if (ret != MV_OK)
        return false;
    handle = nullptr;
    return true;
}

std::string Camera::generate_file_name() const
{
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
    const std::time_t time_value = std::chrono::system_clock::to_time_t(now);
    std::tm time_info{};
    localtime_r(&time_value, &time_info);
    char buffer[64]{};
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &time_info);
    char full_buffer[128]{};
    std::snprintf(full_buffer, sizeof(full_buffer), "../../img_saved/capture_%s_%03lld.jpg", buffer, static_cast<long long>(milliseconds));
    return full_buffer;
}

Camera::~Camera()
{
    stop_grabbing();
    close_device();
    destroy_handle();
    if (sdk_initialized)
    {
        MV_CC_Finalize();
        sdk_initialized = false;
    }
}
