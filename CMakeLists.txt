cmake_minimum_required(VERSION 3.31)
project(polyshow LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
    OR CMAKE_INSTALL_PREFIX STREQUAL "C:/Program Files/${PROJECT_NAME}"
    OR CMAKE_INSTALL_PREFIX STREQUAL "C:/Program Files (x86)/${PROJECT_NAME}")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Version)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/6.8.3/msvc2022_64")
set(POLYSHOW_TARGET_NAME polyshow)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Svg Widgets)
include(DeployRuntime)

set(POLYSHOW_HEADERS
    include/core/GeometryTypes.h
    include/core/PrimitiveEditing.h
    include/core/LayerEditing.h
    include/core/GeometryScene.h
    include/parsers/PlyParser.h
    include/parsers/PlySerializer.h
    include/style/AppStyle.h
    include/style/DarkStyle.h
    include/style/LightStyle.h
    include/style/RenderTheme.h
    include/style/ThemeMode.h
    include/ui/GeometryViewer.h
    include/ui/ColorField.h
    include/ui/EditorPanelHeader.h
    include/ui/IconButton.h
    include/ui/InspectorCheckBox.h
    include/ui/InspectorPanel.h
    include/ui/InspectorSection.h
    include/ui/IpcListenerWorker.h
    include/ui/LayerSidebar.h
    include/ui/LogPanel.h
    include/ui/MaterialIcon.h
    include/ui/MaterialIconLabel.h
    include/ui/OutlinerFilterProxyModel.h
    include/ui/OutlinerItemDelegate.h
    include/ui/OutlinerTreeModel.h
    include/ui/PropertyPanel.h
    include/ui/MainWindow.h
    include/ui/PanelFrame.h
    include/ui/PillButton.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/core/Version.h"
)

set(POLYSHOW_SOURCES
    main.cpp
    src/core/LayerEditing.cpp
    src/core/GeometryScene.cpp
    src/core/PrimitiveEditing.cpp
    src/parsers/PlyParser.cpp
    src/parsers/PlySerializer.cpp
    resources/fonts/fonts.qrc
    resources/icons/icons.qrc
    resources/overlay/overlay.qrc
    resources/style/style.qrc
    src/style/AppStyle.cpp
    src/style/DarkStyle.cpp
    src/style/LightStyle.cpp
    src/style/RenderTheme.cpp
    src/ui/ColorField.cpp
    src/ui/EditorPanelHeader.cpp
    src/ui/GeometryViewer.cpp
    src/ui/IconButton.cpp
    src/ui/InspectorCheckBox.cpp
    src/ui/InspectorPanel.cpp
    src/ui/InspectorSection.cpp
    src/ui/IpcListenerWorker.cpp
    src/ui/LayerSidebar.cpp
    src/ui/LogPanel.cpp
    src/ui/MaterialIcon.cpp
    src/ui/MaterialIconLabel.cpp
    src/ui/OutlinerFilterProxyModel.cpp
    src/ui/OutlinerItemDelegate.cpp
    src/ui/OutlinerTreeModel.cpp
    src/ui/PropertyPanel.cpp
    src/ui/MainWindow.cpp
    src/ui/PanelFrame.cpp
    src/ui/PillButton.cpp
)

set(POLYSHOW_IPC_SENDER_TARGET polyshow_ipc_sender)
set(POLYSHOW_IPC_SENDER_SOURCES
    PolyShowIpcSenderMain.cpp
)

add_executable(
    ${POLYSHOW_TARGET_NAME}
    ${POLYSHOW_HEADERS}
    ${POLYSHOW_SOURCES}
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/core/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/core/Version.h"
    @ONLY
)

add_executable(
    ${POLYSHOW_IPC_SENDER_TARGET}
    ${POLYSHOW_IPC_SENDER_SOURCES}
    PolyShowIpcClient.h
)

target_include_directories(${POLYSHOW_TARGET_NAME} PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/include")
target_include_directories(${POLYSHOW_IPC_SENDER_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    ${POLYSHOW_TARGET_NAME}
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Network
        Qt::Svg
        Qt::Widgets
)

deploy_runtime_targets(
    TARGETS
        ${POLYSHOW_TARGET_NAME}
        ${POLYSHOW_IPC_SENDER_TARGET}
    QT_TARGETS
        ${POLYSHOW_TARGET_NAME}
)
