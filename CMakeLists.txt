# 1. CMake 最低版本要求
cmake_minimum_required(VERSION 3.6)

# 2. 工程名称
project(mymuduo)

# 设置动态库输出路径
# 将编译生成的 .so 文件统一存放在项目根目录下的 lib 文件夹中
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 4. 配置编译选项与 C++ 标准
# -g: 生成调试信息；-Wall: 显示所有警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIC")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 5. 定义参与编译的源文件变量
# 自动搜集当前目录下（.）所有的源文件并存入 SRC_LIST 变量中
aux_source_directory(. SRC_LIST)

# 6. 生成动态库
# SHARED 表示生成动态库（libmymuduo.so），默认不写则可能生成静态库
add_library(mymuduo SHARED ${SRC_LIST})