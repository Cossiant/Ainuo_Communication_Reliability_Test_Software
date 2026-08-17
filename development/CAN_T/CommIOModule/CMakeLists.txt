cmake_minimum_required(VERSION 3.5)
project(CommIOModule LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 组件
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core SerialPort Network)

# NI-VISA 路径
set(VISA_INCLUDE_DIR "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/include")
set(VISA_LIB_DIR "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib/msc")

# ZLGCAN x64 路径
set(CAN_DIR "${CMAKE_SOURCE_DIR}/development/CAN_T/CAN_lib/zlgcan_x64")

# 头文件
set(HEADERS
    include/CommIOModule/CommTypes.h
    include/CommIOModule/PerfClock.h
    include/CommIOModule/CommConfig.h
    include/CommIOModule/ICommChannel.h
    include/CommIOModule/CommChannelFactory.h
    include/CommIOModule/SerialPortChannel.h
    include/CommIOModule/TCPChannel.h
    include/CommIOModule/GPIBChannel.h
    include/CommIOModule/GPIBWorker.h
    include/CommIOModule/CANChannel.h
    include/CommIOModule/CANWorker.h
    include/CommIOModule/CANReadThread.h
)

# 源文件
set(SOURCES
    src/CommTypes.cpp
    src/PerfClock.cpp
    src/ICommChannel.cpp
    src/CommChannelFactory.cpp
    src/SerialPortChannel.cpp
    src/TCPChannel.cpp
    src/GPIBChannel.cpp
    src/GPIBWorker.cpp
    src/CANChannel.cpp
    src/CANWorker.cpp
    src/CANReadThread.cpp
)

# 创建静态库
add_library(CommIOModule STATIC ${SOURCES} ${HEADERS})

# PUBLIC 表示：我自己编译要用这些头文件，别人链接我时也要用
target_include_directories(CommIOModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# PRIVATE 表示：我自己编译要用这些头文件，但别人链接后不应该用到(visa.h、zlgcan.h都只在cpp中引入，exe中不应该直接包含这些头文件))
target_include_directories(CommIOModule PRIVATE
    ${VISA_INCLUDE_DIR}
    ${CAN_DIR}/include
)

# 链接 Qt 模块
target_link_libraries(CommIOModule PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
)

# 链接 NI-VISA (PUBLIC: 静态库依赖需特别传递给最终链接目标(如果是动态链接则应该是PRIVAT))
# target_link_directories 用来告诉编译器去哪个文件夹找 .lib 或 .so 文件
target_link_directories(CommIOModule PUBLIC ${VISA_LIB_DIR})
target_link_libraries(CommIOModule PUBLIC visa32)

# ZLGCAN 使用运行时动态加载，不在此处链接

