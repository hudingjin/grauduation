cmake_minimum_required(VERSION 3.5)

project(aircraftSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用Ruckig支持选项
option(ENABLE_RUCKIG "Enable Ruckig trajectory generator" ON)
if(ENABLE_RUCKIG)
    message(STATUS "Ruckig trajectory generator enabled")
    add_definitions(-DHAVE_RUCKIG -DRUCKIG_DISABLE_CLOUD_CLIENT)


    # 手动编译Ruckig核心文件
    set(RUCKIG_SOURCES
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/brake.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_first_step1.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_first_step2.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_second_step1.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_second_step2.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_third_step1.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/position_third_step2.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/velocity_second_step1.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/velocity_second_step2.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/velocity_third_step1.cpp
            ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/src/ruckig/velocity_third_step2.cpp
    )

    # 创建Ruckig库
    add_library(ruckig STATIC ${RUCKIG_SOURCES})
    target_include_directories(ruckig
            PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/include
    )

    # Windows上链接socket库
    if(WIN32)
        target_link_libraries(ruckig PUBLIC ws2_32)
    endif()

else()
    message(STATUS "Ruckig trajectory generator disabled, using simple interpolation")
endif()

# 查找Qt组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        Widgets
        Core
        Sql
        Concurrent
        Charts
)

# 检查是否找到所需组件
if(TARGET Qt${QT_VERSION_MAJOR}::Core)
    message(STATUS "Qt Core found")
else()
    message(FATAL_ERROR "Qt Core not found")
endif()

# 设置项目源文件
set(PROJECT_SOURCES
        src/main.cpp
        src/gui/mainwindow.cpp
        src/gui/mainwindow.h
        src/gui/mainwindow.ui
        src/gui/widgets/TitleBarWidget.cpp
        src/gui/widgets/TitleBarWidget.h
        src/core/FlightParams.cpp
        src/core/FlightParams.h
        src/core/FlightConfigRepository.cpp
        src/core/FlightConfigRepository.h
        src/core/SimulationManager.cpp
        src/core/SimulationManager.h
        src/core/TrajectoryGenerator.cpp
        src/core/TrajectoryGenerator.h
        src/gui/widgets/ChartWidget.cpp
        src/gui/widgets/ChartWidget.h
        src/gui/widgets/DataTableWidget.cpp
        src/gui/widgets/DataTableWidget.h
        src/gui/widgets/TrajectoryTableModel.cpp
        src/gui/widgets/TrajectoryTableModel.h
        src/gui/widgets/StatusWidget.cpp
        src/gui/widgets/StatusWidget.h
        src/gui/dialogs/AttitudeDialog.cpp
        src/gui/dialogs/AttitudeDialog.h
        src/gui/widgets/FlightDisplayWidget.cpp
        src/gui/widgets/FlightDisplayWidget.h
)

# 设置包含目录
include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/gui
        ${CMAKE_SOURCE_DIR}/src/gui/widgets
        ${CMAKE_SOURCE_DIR}/thirdparty/ruckig-main/include
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(aircraftSystem
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            Resourse.qrc
    )
else()
    add_executable(aircraftSystem
            ${PROJECT_SOURCES}
    )
endif()

# 链接Qt库
target_link_libraries(aircraftSystem PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Charts
)

# 链接Ruckig库
if(ENABLE_RUCKIG AND TARGET ruckig)
    target_link_libraries(aircraftSystem PRIVATE ruckig)
    message(STATUS "Linking with Ruckig library")
endif()

# Windows上额外链接
if(WIN32)
    target_link_libraries(aircraftSystem PRIVATE ws2_32 dwmapi)
endif()

# 设置目标属性
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.aircraftSystem)
endif()

set_target_properties(aircraftSystem PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS aircraftSystem
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(aircraftSystem)
endif()
