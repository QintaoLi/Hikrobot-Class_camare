#include <iostream>
#include "Camera.h"
#include <opencv2/opencv.hpp>
#include <thread>
int main()
{
    try
    {
        Camera camera;

        camera.Enumerate_Devices();
        camera.Create_Handle();
        camera.Open_Device();
        camera.Set_Exposure_And_Gain();
        camera.Set_Sync_Time_Out();
        camera.Set_Transfer_Size();
        camera.Set_Transfer_Ways();
        camera.Set_Image_Nodenum();

        camera.Start_Grabbing();

        cv::namedWindow("Camera Frame", cv::WINDOW_NORMAL);
        cv::resizeWindow("Camera Frame", 800, 600);
        cv::Mat Image; // 用于存储抓取的图像
        int fail_Count = 0; // 记录连续获取图像失败的次数
        const int Max_Fail_Count = 30; // 最大连续失败次数

        while (true) 
        {
            if(camera.Get_Frame(Image)) 
            {
                fail_Count = 0; // 重置失败计数
                // 显示图像
                if(!Image.empty())
                {
                    cv::imshow("Camera Frame", Image);
                }
            }
            else 
            {
                fail_Count++;
                std::cerr << "[Warning] 获取图像超时/失败 (连续第 " 
                      << fail_Count << " 次)" << std::endl;
                // 失败时做微小休眠，避免死循环把单个 CPU 核心占满到 100%
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

                if(fail_Count>=Max_Fail_Count){
                    std::cerr << "[Fatal Error] 相机长时间未响应，断开退出！" 
                    << std::endl;
                    break;
                }
            }
            int key= cv::waitKey(1);
            if(key=='s'||key=='S') // 按下 's' 键保存图像
            {
                if (!Image.empty()) 
                {
                    std::string fileName = camera.GenerateFileName();
                    cv::imwrite(fileName, Image);
                    std::cout << "图像已保存为: " << fileName << std::endl;
                } 
                else 
                {
                    std::cerr << "当前图像为空，无法保存" << std::endl;
                }
            }
            else if(key=='q'||key=='Q')
            {
                std::cout<<"已退出"<<std::endl;
                break; // 按下 'q' 键退出循环
            }
        }

        camera.Stop_Grabbing();
        camera.Close_Device();
        camera.Destroy_Handle();
        cv::destroyAllWindows();
    }
    catch (const std::exception& e)
    {
        std::cerr << "程序出错：" << e.what() << std::endl;
    }
    return 0;
}