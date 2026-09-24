#include <chrono>
#include <ctime>
#include <iostream>
#include "Camera.h"

// 构造相机对象。
Camera::Camera() = default;

// 初始化海康相机 SDK。
bool Camera::initialize()
{
    if (m_sdk_initialized)
        return true;
    const int ret = MV_CC_Initialize();
    if (ret != MV_OK)
    {
        std::cerr << "SDK 初始化失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    m_sdk_initialized = true;
    return true;
}

// 枚举可用的 USB 相机设备。
bool Camera::enumerate_devices()
{
    const int ret = MV_CC_EnumDevices(MV_USB_DEVICE, &m_device_list);
    if (ret != MV_OK || m_device_list.nDeviceNum == 0)
    {
        std::cerr << (ret != MV_OK ? "设备枚举失败" : "没有发现相机") << std::endl;
        return false;
    }
    return true;
}

// 为第一台相机创建设备句柄。
bool Camera::create_handle()
{
    if (m_device_list.nDeviceNum == 0)
        return false;
    const int ret = MV_CC_CreateHandle(&m_handle, m_device_list.pDeviceInfo[0]);
    if (ret != MV_OK)
    {
        std::cerr << "创建相机句柄失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }
    return true;
}

// 打开相机设备。
bool Camera::open_device()
{
    const int ret = MV_CC_OpenDevice(m_handle);
    if (ret != MV_OK)
    {
        std::cerr << "打开相机失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        destroy_handle();
        return false;
    }
    m_device_opened = true;
    return true;
}

// 设置相机的曝光和增益参数。
bool Camera::set_exposure_and_gain()
{
    int ret = MV_CC_SetEnumValue(m_handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    if (ret == MV_OK) ret = MV_CC_SetFloatValue(m_handle, "ExposureTime", 60000.0f);
    if (ret == MV_OK) ret = MV_CC_SetEnumValue(m_handle, "GainAuto", MV_GAIN_MODE_OFF);
    if (ret == MV_OK) ret = MV_CC_SetFloatValue(m_handle, "Gain", 0.0f);
    if (ret != MV_OK)
    {
        std::cerr << "设置曝光或增益失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    return true;
}

// 设置 USB 同步读写超时时间。
bool Camera::set_sync_time_out()
{
    const int ret = MV_USB_SetSyncTimeOut(m_handle, 1000);
    if (ret != MV_OK)
    {
        std::cerr << "设置同步超时失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    return true;
}

// 设置图像数据传输大小。
bool Camera::set_transfer_size()
{
    const int ret = MV_USB_SetTransferSize(m_handle, 1 * 1024 * 1024);
    if (ret != MV_OK)
    {
        std::cerr << "设置传输大小失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    return true;
}

// 设置 SDK 内部的传输通道数量。
bool Camera::set_transfer_ways()
{
    const int ret = MV_USB_SetTransferWays(m_handle, 2);
    if (ret != MV_OK)
    {
        std::cerr << "设置传输方式失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    return true;
}

// 设置相机图像缓存节点数量。
bool Camera::set_image_nodenum()
{
    const int ret = MV_CC_SetImageNodeNum(m_handle, 5);
    if (ret != MV_OK)
    {
        std::cerr << "设置图像缓存节点数失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    return true;
}

// 开始从相机采集图像。
bool Camera::start_grabbing()
{
    const int ret = MV_CC_StartGrabbing(m_handle);
    if (ret != MV_OK)
    {
        std::cerr << "开始抓图失败，错误码：0x" << std::hex << ret << std::dec << std::endl;
        close_device();
        destroy_handle();
        return false;
    }
    m_grabbing = true;
    return true;
}

// 获取一帧图像并转换为 OpenCV 格式。
bool Camera::get_frame(cv::Mat& image)
{
    if (!m_bayer_quality_set)
    {
        const int ret = MV_CC_SetBayerCvtQuality(m_handle, 1);
        if (ret != MV_OK)
            return false;
        m_bayer_quality_set = true;
    }

    MV_FRAME_OUT frame{};
    int ret = MV_CC_GetImageBuffer(m_handle, &frame, 100);
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

    ret = MV_CC_ConvertPixelTypeEx(m_handle, &param);
    const int free_ret = MV_CC_FreeImageBuffer(m_handle, &frame);
    if (ret != MV_OK || free_ret != MV_OK)
        return false;

    image = result;
    return true;
}

// 停止相机图像采集。
bool Camera::stop_grabbing()
{
    if (!m_grabbing)
        return true;
    const int ret = MV_CC_StopGrabbing(m_handle);
    if (ret != MV_OK)
        return false;
    m_grabbing = false;
    return true;
}

// 关闭相机设备。
bool Camera::close_device()
{
    if (!m_device_opened)
        return true;
    const int ret = MV_CC_CloseDevice(m_handle);
    if (ret != MV_OK)
        return false;
    m_device_opened = false;
    return true;
}

// 销毁相机设备句柄。
bool Camera::destroy_handle()
{
    if (m_handle == nullptr)
        return true;
    const int ret = MV_CC_DestroyHandle(m_handle);
    if (ret != MV_OK)
        return false;
    m_handle = nullptr;
    return true;
}

// 生成带时间戳的图像保存文件名。
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

// 释放相机、句柄和 SDK 资源。
Camera::~Camera()
{
    stop_grabbing();
    close_device();
    destroy_handle();
    if (m_sdk_initialized)
    {
        MV_CC_Finalize();
        m_sdk_initialized = false;
    }
}
