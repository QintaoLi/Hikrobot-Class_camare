#include "Camera.h"
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    Camera camera;

    // 这里只要报错就会返回错误，然后提示失败，结束程序
    if (!camera.initialize() ||
        !camera.enumerate_devices() ||
        !camera.create_handle() ||
        !camera.open_device() ||
        !camera.set_exposure_and_gain() ||
        !camera.set_sync_time_out() ||
        !camera.set_transfer_size() ||
        !camera.set_transfer_ways() ||
        !camera.set_image_nodenum() ||
        !camera.start_grabbing()
    )
    {
        std::cerr << "相机初始化失败，程序退出" << std::endl;
        return -1;
    }

    cv::namedWindow("Camera Frame", cv::WINDOW_NORMAL);
    cv::resizeWindow("Camera Frame", 800, 600);

    cv::Mat image;
    int fail_count = 0;
    constexpr int max_fail_count = 30;

    // 从sdk处获取图像显示在窗口，然后按 s|S 保存图像至img_saved文件夹，按 q|Q 退出程序
    while (true)
    {
        // 这里是当相机采不到图像时先等等（最多30次，一次camera.get_frame（）等100ms+sleep_for等5ms,加起来大约3秒），如果还是没采到就报错结束程序
        if (camera.get_frame(image))
        {
            fail_count = 0;
            if (!image.empty())
                cv::imshow("Camera Frame", image);
        }
        else
        {
            ++fail_count;
            if (fail_count >= max_fail_count)
            {
                std::cerr << "相机长时间未返回图像，程序退出" << std::endl;
                break;
            }
            //等5ms，防止循环连续高速执行导致 CPU 占用率升高。
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        const int key = cv::waitKey(1);
        if (key == 's' || key == 'S')
        {
            if (!image.empty() && cv::imwrite(camera.generate_file_name(), image))
                std::cout << "图像已保存" << std::endl;
            else
                std::cerr << "图像保存失败" << std::endl;
        }
        else if (key == 'q' || key == 'Q')
        {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
