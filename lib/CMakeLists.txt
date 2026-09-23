# 将源码文件列表保存到 SRC_LIST 变量
aux_source_directory(src SRC_LIST)

# 生成动态库
add_library(mylib SHARED ${SRC_LIST})

# 找到opencv的包
find_package(OpenCV REQUIRED)

# 指定头文件路径
target_include_directories(mylib PUBLIC
    include
    /opt/MVS/include
    ${OpenCV_INCLUDE_DIRS}
)

# 包含库文件路径 
target_link_directories(mylib PUBLIC
    /opt/MVS/lib/64
)

# 链接目标库
target_link_libraries(mylib PUBLIC
    MvCameraControl
    ${OpenCV_LIBS}
)
