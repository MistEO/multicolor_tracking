cmake_minimum_required(VERSION 2.8)	# CMake工具最低版本要求
project(sample)				        # 工程项目名,编译出来的可执行文件名字
aux_source_directory(. SRC_LIST)	# 源文件列表
#include_directories(./header/)		# 头文件目录
#link_directories(/usr/local/lib)	# 库文件的目录
add_executable(${PROJECT_NAME} ${SRC_LIST})     # 工程项目名和我们要编译的文件名

IF (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")	# gcc添加支持c++11
ENDIF (UNIX)

IF (WIN32)
    set(OpenCV_DIR F:\libraries\opencv\build)               # windows下设置OpenCv的路径
ENDIF(WIN32)

find_package(OpenCV REQUIRED)				            # 搜索指定的外部依赖库头文件和库文件
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})	# 链接到OpenCV库
