cmake_minimum_required(VERSION 3.31)
project(polyshow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/6.9.1/msvc2022_64")
set(POLYSHOW_TARGET_NAME polyshow)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Svg Widgets)

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
    include/ui/InspectorPanel.h
    include/ui/IpcListenerWorker.h
    include/ui/LayerSidebar.h
    include/ui/LogPanel.h
    include/ui/PropertyPanel.h
    include/ui/MainWindow.h
    include/ui/PanelFrame.h
    include/ui/PillButton.h
)

set(POLYSHOW_SOURCES
    main.cpp
    src/core/LayerEditing.cpp
    src/core/GeometryScene.cpp
    src/core/PrimitiveEditing.cpp
    src/parsers/PlyParser.cpp
    src/parsers/PlySerializer.cpp
    resources/overlay/overlay.qrc
    resources/style/style.qrc
    src/style/AppStyle.cpp
    src/style/DarkStyle.cpp
    src/style/LightStyle.cpp
    src/style/RenderTheme.cpp
    src/ui/ColorField.cpp
    src/ui/GeometryViewer.cpp
    src/ui/InspectorPanel.cpp
    src/ui/IpcListenerWorker.cpp
    src/ui/LayerSidebar.cpp
    src/ui/LogPanel.cpp
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

add_executable(
    ${POLYSHOW_IPC_SENDER_TARGET}
    ${POLYSHOW_IPC_SENDER_SOURCES}
    PolyShowIpcClient.h
)

target_include_directories(${POLYSHOW_TARGET_NAME} PRIVATE include)
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

if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif()
    endif()

    if(EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll"
        OR EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindowsd.dll")
        add_custom_command(
            TARGET ${POLYSHOW_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${POLYSHOW_TARGET_NAME}>/plugins/platforms/"
        )
        add_custom_command(
            TARGET ${POLYSHOW_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
                    "$<TARGET_FILE_DIR:${POLYSHOW_TARGET_NAME}>/plugins/platforms/"
        )
    endif()

    foreach(QT_LIB Core Gui Network Svg Widgets)
        add_custom_command(
            TARGET ${POLYSHOW_TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}$<$<CONFIG:Debug>:d>.dll"
                    "$<TARGET_FILE_DIR:${POLYSHOW_TARGET_NAME}>"
        )
    endforeach()
endif()
