cmake_minimum_required(VERSION 3.10)
project(untitled)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Apps/Work/Qt/5.15.2/mingw81_64")
add_subdirectory(./QXlsx/QXlsx)
add_subdirectory(./ElaWidgetTools ${CMAKE_BINARY_DIR}/ElaWidgetTools)

# ===== 修复：告诉 CMake MinGW 系统库的位置 =====
list(APPEND CMAKE_LIBRARY_PATH
        "D:/Apps/Work/JetBrains/CLion 2026.1.2/bin/mingw/x86_64-w64-mingw32/lib"
        "D:/Apps/Work/JetBrains/CLion 2026.1.2/bin/mingw/lib"
)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        SerialPort
        Network
        AxContainer
        REQUIRED
)
# 这里需要在编译的时候加上WIN32，如果是debug需要去掉，否则不显示qdebug内容！
add_executable(untitled WIN32
        development/Other_T/main.cpp
        development/Other_T/ElaWidgetToolsDemo.cpp
        development/Other_T/ElaWidgetToolsDemo.h
        development/ico/app.rc
        development/Old/src/connectnetwork.cpp
        development/Old/include/connectnetwork.h
        development/Old/src/excelsendworker.cpp
        development/Old/include/excelsendworker.h
        development/Other_T/LED.cpp
        development/Other_T/LED.h
        development/Old/src/readexceldata.cpp
        development/Old/include/readexceldata.h
        development/Old/src/responsevalidator.cpp
        development/Old/include/responsevalidator.h
        development/Old/src/saveworker.cpp
        development/Old/include/saveworker.h
        development/Old/src/serialworker.cpp
        development/Old/include/serialworker.h
        development/Other_T/StatCard.cpp
        development/Other_T/StatCard.h
        development/Serial_T/SerialPage.cpp
        development/Serial_T/SerialPage.h
        development/Network_T/NetworkPage.cpp
        development/Network_T/NetworkPage.h
        development/CAN_T/CANPage.cpp
        development/CAN_T/CANPage.h
        development/GPIB_T/GPIBPage.cpp
        development/GPIB_T/GPIBPage.h
        development/USER_T/USERPage.cpp
        development/USER_T/USERPage.h
        development/Other_T/MainPage.cpp
        development/Other_T/MainPage.h
        development/Serial_T/SerialWork.cpp
        development/Serial_T/SerialWork.h
        development/Serial_T/SerialExcel.cpp
        development/Serial_T/SerialExcel.h
        development/Network_T/NetWorkWork.cpp
        development/Network_T/NetWorkWork.h
        development/Network_T/NetworkExcel.cpp
        development/Network_T/NetworkExcel.h
)

target_link_libraries(untitled
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::SerialPort
        Qt5::Network
        Qt5::AxContainer
        QXlsx::QXlsx
        ElaWidgetTools
)

# ① 复制 ElaWidgetTools DLL
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ElaWidgetTools>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying ElaWidgetTools DLL..."
)

# ② windeployqt
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "D:/Apps/Work/Qt/5.15.2/mingw81_64/bin/windeployqt.exe"
        --debug --no-translations
        "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt..."
)
