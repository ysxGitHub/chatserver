PROJECT(SERVER)
SET(SRC_LIST muduo_server.cc)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
set(CMAKE_CXX_FLAGS ${CXX_FLAGS} -std=c++11)

# 设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 表示生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
ADD_EXECUTABLE(server ${SRC_LIST})

# 表示server这个目标程序，需要连接muduo_net muduo_base pthread这三个库文件
TARGET_LINK_LIBRARIES(server mymuduo pthread)
