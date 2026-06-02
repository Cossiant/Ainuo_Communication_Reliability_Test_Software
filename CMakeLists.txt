cmake_minimum_required(VERSION 3.10)
project(untitled)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Apps/Work/Qt/5.15.2/mingw81_64")

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

add_subdirectory(./ElaWidgetTools ${CMAKE_BINARY_DIR}/ElaWidgetTools)

add_executable(untitled WIN32
        development/src/main.cpp
        development/src/ElaWidgetToolsDemo.cpp
        development/include/ElaWidgetToolsDemo.h
        development/src/connectnetwork.cpp
        development/include/connectnetwork.h
        development/src/excelsendworker.cpp
        development/include/excelsendworker.h
        development/src/led.cpp
        development/include/led.h
        development/src/readexceldata.cpp
        development/include/readexceldata.h
        development/src/responsevalidator.cpp
        development/include/responsevalidator.h
        development/src/saveworker.cpp
        development/include/saveworker.h
        development/src/serialworker.cpp
        development/include/serialworker.h
)

target_link_libraries(untitled
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::SerialPort
        Qt5::Network
        Qt5::AxContainer
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
