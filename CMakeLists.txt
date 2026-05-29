cmake_minimum_required(VERSION 3.30)

project(stock-tool VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)

qt_standard_project_setup()

set(STOCK_TOOL_MODULE_IFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/App/MainWindow.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Domain/FactorModel.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Domain/PriceCsv.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Dock/SymbolSearchPane.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Dock/WatchListPane.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Dock/ChartPane.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Dock/AnalysisPane.ixx
)

set(STOCK_TOOL_MODULE_IMPLS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MainWindow.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Domain/FactorModel.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Domain/PriceCsv.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Dock/SymbolSearchPane.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Dock/WatchListPane.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Dock/ChartPane.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Dock/AnalysisPane.cppm
)

qt_add_executable(stock-tool
    src/main.cpp
)

target_sources(stock-tool
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ${STOCK_TOOL_MODULE_IFACES}
    PRIVATE
        ${STOCK_TOOL_MODULE_IMPLS}
)

target_include_directories(stock-tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(stock-tool
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ads::qtadvanceddocking-qt6
)

if(MSVC)
    target_compile_options(stock-tool PRIVATE /W4 /permissive-)
else()
    target_compile_options(stock-tool PRIVATE -Wall -Wextra -Wpedantic)
endif()
