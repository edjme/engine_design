cmake_minimum_required(VERSION 3.16)
project(engine_design LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets CONFIG REQUIRED)

# --- core (логика расчётов) ---

# Все .cpp внутри core/
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
)

# Убираем консольные main-файлы, они нам не нужны в библиотеке
list(REMOVE_ITEM CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/main_kinematic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/IND_Diagr/calculations/ind_diagr_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Diag_Palpha/Calculations/main_Palpha.cpp"
)

# ЯВНО ДОБАВЛЯЕМ НОВЫЕ ФАЙЛЫ МОДЕЛЕЙ (если GLOB их не подхватил)
# Но GLOB должен подхватить, если они лежат в правильных папках.
# Для надёжности можно добавить конкретные пути:

set(KINEMATIC_MODELS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/AxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/DeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedDeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedAttachedKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/VShapedAttachedDeaxialKSMModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Kinematic/Calculations/models/KSMModel.cpp"
)

# Добавляем их в CORE_SOURCES
list(APPEND CORE_SOURCES ${KINEMATIC_MODELS_SOURCES})

# Отдельно добавляем файлы менеджера путей (они лежат в корне)
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

# --- GUI-приложение ---

add_executable(engine_gui WIN32
    gui/main_gui.cpp
    gui/MainFrame.cpp
    gui/MainFrame.h
    gui/KinematicPlotPanel.cpp
    gui/KinematicPlotPanel.h
    gui/KinematicTablePanel.cpp      
    gui/KinematicTablePanel.h
)

target_link_libraries(engine_gui PRIVATE
    engine_core
    wx::core
    wx::base
)