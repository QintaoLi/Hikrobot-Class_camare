#include "Camera.h"
Camera::Camera()
{
    // 构造函数的实现
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_Initialize();
    if(nRet != MV_OK)
    {
        throw std::runtime_error("SDK 初始化失败");
    }
    sdkInitialized = true;
}

void Camera::Enumerate_Devices()
{
    int nRet = MV_CC_EnumDevices(
        MV_USB_DEVICE,
        &this->stDeviceList
    );
    if(nRet != MV_OK)
    {
        throw std::runtime_error("设备枚举失败");
    }
    if (this->stDeviceList.nDeviceNum == 0)
    {
        throw std::runtime_error("没有发现相机");
    }
}

void Camera::Create_Handle()
{
    this->handle = nullptr; // 初始化 handle 为 nullptr
    int nRet = MV_CC_CreateHandle(
        &this->handle,
        this->stDeviceList.pDeviceInfo[0]
    );
    if(nRet != MV_OK)
    {
        throw std::runtime_error("创建相机句柄失败");
    }
}

void Camera::Open_Device()
{
    int nRet = MV_CC_OpenDevice(this->handle);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("打开相机失败");
    }
    deviceOpened = true;
}

void Camera::Set_Exposure_And_Gain()
{
    // 关闭自动曝光，使用手动曝光值
    int ret = MV_CC_SetEnumValue(
        this->handle, 
        "ExposureAuto",
        MV_EXPOSURE_AUTO_MODE_OFF
    );

    if (ret != MV_OK)
    {
        throw std::runtime_error("关闭自动曝光失败");
    }

    // 曝光时间 60000 微秒
    ret = MV_CC_SetFloatValue(
        this->handle,
        "ExposureTime", 
        60000.0f
    );

    if (ret != MV_OK)
    {
        throw std::runtime_error("设置曝光时间失败");
    }

    // 关闭自动增益
    ret = MV_CC_SetEnumValue(
        this->handle,
        "GainAuto", 
        MV_GAIN_MODE_OFF
    );

    if (ret != MV_OK)
    {
        throw std::runtime_error("关闭自动增益失败");
    }

    // 增益先设为 0，优先通过曝光和光圈调整亮度
    ret = MV_CC_SetFloatValue(
        this->handle, 
        "Gain", 
        0.0f
    );

    if (ret != MV_OK)
    {
        throw std::runtime_error("设置增益失败");
    }
}

void Camera::Set_Sync_Time_Out(){
    // 默认超时时间1000ms
    unsigned int nMills = 1000;
    // 设置相机同步读写超时时间
    int nRet = MV_USB_SetSyncTimeOut(this->handle, nMills);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("设置同步超时失败");
    }
}

void Camera::Set_Transfer_Size(){
    // 配置相机流数据的最大数据包大小
    unsigned int nTransferSize = 1*1024*1024;   // 默认1MB
    int nRet = MV_USB_SetTransferSize(this->handle, nTransferSize);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("设置传输大小失败");
    }
}

void Camera::Set_Transfer_Ways(){
    // 配置SDK内部的流通道缓存个数
    unsigned int nTransferWay = 2; 
    int nRet = MV_USB_SetTransferWays(this->handle, nTransferWay);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("设置传输方式失败");
    }
}

void Camera::Close_Device()
{
    int nRet = MV_CC_CloseDevice(this->handle);
    this->deviceOpened = false;
    if(nRet != MV_OK)
    {
        throw std::runtime_error("关闭相机失败");
    }
}

void Camera::Set_Image_Nodenum()
{
    // 设置图像缓存节点数
    unsigned int nImageNodeNum = 5; // 设置为5个缓存节点
    int nRet = MV_CC_SetImageNodeNum(this->handle, nImageNodeNum);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("设置图像缓存节点数失败");
    }
}

void Camera::Start_Grabbing()
{
    // 开始抓图
    int nRet = MV_CC_StartGrabbing(this->handle);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("开始抓图失败");
    }
    this->grabbing = true;
}

bool Camera::Get_Frame(cv::Mat& image)
{

    // 1. 确保 Bayer 插值算法质量只在第一次进入时配置一次（1: 均衡模式）
    if (!this->m_bBayerQualitySet)
    {
        int nRet = MV_CC_SetBayerCvtQuality(this->handle, 1);
        if (nRet != MV_OK)
        {
            std::stringstream ss;
            ss << "设置 Bayer 插值质量失败，错误码：0x" 
               << std::hex << std::uppercase << nRet;
            throw std::runtime_error(ss.str());
        }
        this->m_bBayerQualitySet = true;
    }

    MV_FRAME_OUT frame = {};

    int ret = MV_CC_GetImageBuffer(this->handle, &frame, 100);

    // 本次没有图像，交给主循环继续尝试
    if (ret == MV_E_NODATA)
    {
        return false;
    }

    if (ret != MV_OK)
    {
        throw std::runtime_error(
            "获取图像失败，错误码：" + std::to_string(ret));
    }

    cv::Mat result;

    try
    {
        const auto& info = frame.stFrameInfo;

        // 分配由 OpenCV 管理的三通道图像内存
        result.create(info.nHeight, info.nWidth, CV_8UC3);

        MV_CC_PIXEL_CONVERT_PARAM_EX param = {};

        param.nWidth = info.nWidth;
        param.nHeight = info.nHeight;

        // 输入：相机原始数据，当前配置为 BayerRG8
        param.enSrcPixelType = info.enPixelType;
        param.pSrcData = frame.pBufAddr;
        param.nSrcDataLen = info.nFrameLen;

        // 输出：BGR8，转换结果直接写入 result
        param.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
        param.pDstBuffer = result.data;
        param.nDstBufferSize = static_cast<unsigned int>(
            result.total() * result.elemSize());

        // ret 已经定义过，这里直接赋值
        ret = MV_CC_ConvertPixelTypeEx(this->handle, &param);

        if (ret != MV_OK)
        {
            throw std::runtime_error(
                "像素转换失败，错误码：" + std::to_string(ret));
        }
    }
    catch (...)
    {
        // 即使分配内存或转换时发生异常，也归还缓冲区
        MV_CC_FreeImageBuffer(this->handle, &frame);
        throw;
    }

    ret = MV_CC_FreeImageBuffer(this->handle, &frame);

    if (ret != MV_OK)
    {
        throw std::runtime_error(
            "释放图像缓冲区失败，错误码：" + std::to_string(ret));
    }

    // 将转换结果交给调用方
    image = result;

    return true;
}

void Camera::Stop_Grabbing()
{
    // 停止抓图
    int nRet = MV_CC_StopGrabbing(this->handle);
    if(nRet != MV_OK)
    {
        throw std::runtime_error("停止抓图失败");
    }
    this->grabbing = false;
}

void Camera::Destroy_Handle()
{
    int nRet = MV_CC_DestroyHandle(this->handle);
    this->handle = nullptr;
    if(nRet != MV_OK)
    {
        throw std::runtime_error("销毁相机句柄失败");
    }
}

std::string Camera::GenerateFileName()
{
    // 1. 获取高精度时间点
    auto now = std::chrono::system_clock::now();

    // 2. 提取秒数和毫秒余数
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();

    // 3. 转换为线程安全的本地时间
    std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm time_info;

    localtime_r(&in_time_t, &time_info); // Linux 线程安全

    // 4. 格式化到毫秒级别
    char buf[64];
    // %Y%m%d_%H%M%S -> 20260923_000601
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &time_info);

    char full_buf[128];
    // 追加 3 位毫秒前导零，例如 _125
    snprintf(full_buf, sizeof(full_buf), "capture_%s_%03lld.jpg", buf, (long long)ms);

    return std::string(full_buf);
}

Camera::~Camera() 
{
    // 析构函数的实现
    if (deviceOpened)
    {
        MV_CC_CloseDevice(this->handle);
    }
    if (handle != nullptr)
    {
        MV_CC_DestroyHandle(this->handle);
    }
    if (sdkInitialized)
    {           
        MV_CC_Finalize();
    }
}

