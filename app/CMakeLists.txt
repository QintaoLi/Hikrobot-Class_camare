# 将源码文件列表保存到SRC_LIST变量
aux_source_directory(src SRC_LIST)

# 生成可执行文件
add_executable(myapp ${SRC_LIST})

# 链接动态库
target_link_libraries(myapp PRIVATE mylib)

# 设置运行时动态库搜索路径
set_target_properties(myapp PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/opt/MVS/lib/64;$ORIGIN/../lib"
)

