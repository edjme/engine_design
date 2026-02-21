cmake_minimum_required(VERSION 3.16)
project(engine_design LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets CONFIG REQUIRED)

# ============================================================
# Библиотека core (вся логика расчётов)
# ============================================================

# Явно перечисляем все исходники core, чтобы избежать проблем с GLOB
set(CORE_SOURCES
    # Kinematic models
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/AxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/DeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedDeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedAttachedKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedAttachedDeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/KSMModel.cpp"
    # Kinematic calculator
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/KinematicCalculator.cpp"
    # Kinematic formulas (старый код)
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/kinematic_formulas.cpp"
    # Input/output
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/input_data/kinematic_input.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/output_data/kinematic_output.cpp"
    # Dynamic calculator
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Dynamic/Calculations/DynamicCalculator.cpp"
    # Другие файлы core (если есть) можно добавить сюда же
)

# Файлы менеджера путей (лежат в корне проекта)
set(PATH_MANAGER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/path_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ind_path_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ind_Palpha_path_manager.cpp"
)

add_library(engine_core STATIC
    ${CORE_SOURCES}
    ${PATH_MANAGER_SOURCES}
)

target_include_directories(engine_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}        # чтобы находились "core/....h" и path_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

# ============================================================
# Исполняемый файл GUI
# ============================================================

add_executable(engine_gui WIN32
    gui/main_gui.cpp
    gui/MainFrame.cpp
    gui/MainFrame.h
    gui/KinematicPlotPanel.cpp
    gui/KinematicPlotPanel.h
    gui/KinematicTablePanel.cpp      
    gui/KinematicTablePanel.h
    gui/dynamic_input.cpp
    gui/dynamic_input.h
)

target_link_libraries(engine_gui PRIVATE
    engine_core
    wx::core
    wx::base
)