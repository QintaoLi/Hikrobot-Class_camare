#include "Camera.h"
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    Camera camera;

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

    while (true)
    {
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
