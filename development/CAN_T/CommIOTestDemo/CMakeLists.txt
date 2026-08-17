cmake_minimum_required(VERSION 3.5)
project(CommIOTestDemo LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core SerialPort Network)

add_executable(CommIOTestDemo main.cpp)

# 链接 IO 模块静态库（会自动传递 PUBLIC 依赖）
target_link_libraries(CommIOTestDemo PRIVATE CommIOModule)

# 拷贝 ZLGCAN dll 到可执行文件输出目录
set(CAN_BIN_DIR "${CMAKE_SOURCE_DIR}/development/CAN_T/CAN_lib/zlgcan_x64")
if(EXISTS ${CAN_BIN_DIR})
    add_custom_command(TARGET CommIOTestDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CAN_BIN_DIR} $<TARGET_FILE_DIR:CommIOTestDemo>
        COMMENT "拷贝 ZLGCAN DLL 到输出目录"
    )
endif()
