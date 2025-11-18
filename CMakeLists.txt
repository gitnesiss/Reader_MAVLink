cmake_minimum_required(VERSION 3.16)

project(MavlinkReader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Network
    SerialPort
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMavlinkReader
    src/main.cpp
    src/mavlinkhandler.cpp
    src/networkmanager.cpp
)

# Создаем необходимые папки если не существуют
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/research")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/research")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/info")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/info")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/logs")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/logs")
endif()

# Ресурсы приложения
qt_add_resources(appMavlinkReader "qml"
    PREFIX "/"
    FILES
        qml/Main.qml
        qml/ConnectionPanel.qml
        qml/DataDisplay.qml
)

# Ресурсы изображений (если будут использоваться)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/images")
    qt_add_resources(appMavlinkReader "images"
        PREFIX "/"
        FILES
            images/connected.png
            images/disconnected.png
            images/logo.png
    )
endif()

# QML модуль
qt_add_qml_module(appMavlinkReader
    URI MavlinkReader
    VERSION 1.0
    SOURCES
        src/mavlinkhandler.cpp
        src/mavlinkhandler.h
        src/networkmanager.cpp
        src/networkmanager.h
    QML_FILES
        qml/Main.qml
        qml/ConnectionPanel.qml
        qml/DataDisplay.qml
)

target_link_libraries(appMavlinkReader
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::SerialPort
)

# Для Windows добавляем библиотеки сокетов
if(WIN32)
    target_link_libraries(appMavlinkReader PRIVATE ws2_32)
endif()

set_target_properties(appMavlinkReader PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS appMavlinkReader
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Копируем research директорию
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/research")
    add_custom_command(TARGET appMavlinkReader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/research"
        "$<TARGET_FILE_DIR:appMavlinkReader>/research"
        COMMENT "Copying research directory to build directory"
    )
endif()

# Копируем info директорию с документацией
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/info")
    add_custom_command(TARGET appMavlinkReader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/info"
        "$<TARGET_FILE_DIR:appMavlinkReader>/info"
        COMMENT "Copying info directory to build directory"
    )
endif()

# Копируем logs директорию
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/logs")
    add_custom_command(TARGET appMavlinkReader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/logs"
        "$<TARGET_FILE_DIR:appMavlinkReader>/logs"
        COMMENT "Copying logs directory to build directory"
    )
endif()

# Создаем необходимые директории в билд директории
add_custom_command(TARGET appMavlinkReader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:appMavlinkReader>/research"
    COMMENT "Creating research directory in build directory"
)

add_custom_command(TARGET appMavlinkReader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:appMavlinkReader>/info"
    COMMENT "Creating info directory in build directory"
)

add_custom_command(TARGET appMavlinkReader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:appMavlinkReader>/logs"
    COMMENT "Creating logs directory in build directory"
)

# Копируем include директорию с MAVLink заголовками
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    add_custom_command(TARGET appMavlinkReader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "$<TARGET_FILE_DIR:appMavlinkReader>/include"
        COMMENT "Copying include directory to build directory"
    )
endif()
