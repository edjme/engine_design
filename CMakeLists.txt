cmake_minimum_required(VERSION 3.22)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Добавляем директорию core ко всем target'ам
include_directories(${CMAKE_SOURCE_DIR})
project(engine_design CXX)
add_definitions(-DUNICODE -D_UNICODE)


# включить цвета и нормальные warning-и
add_compile_options(-Wall -Wextra -Wpedantic)

# =====================================================================================
# 1. kinematic.exe
# =====================================================================================

add_executable(kinematic
    core/Kinematic/Calculations/main_kinematic.cpp
    core/Kinematic/input_data/kinematic_input.cpp
    core/Kinematic/output_data/kinematic_output.cpp
    core/Kinematic/Calculations/kinematic_formulas.cpp
    core/common/common_types.cpp
    path_manager.cpp
)

# =====================================================================================
# 2. ind_diag.exe
# =====================================================================================

add_executable(ind_diag
    core/IND_Diagr/calculations/ind_diagr_main.cpp
    core/IND_Diagr/input_data/ind_diag_input.cpp
    core/IND_Diagr/output_data/ind_diag_output.cpp
    core/IND_Diagr/calculations/calc_ind_diagr.cpp
    core/common/ind_diag_common_types.cpp
    ind_path_manager.cpp
)

# =====================================================================================
# 3. Palpha_Diag.exe
# =====================================================================================

add_executable(Palpha_Diag
    core/Diag_Palpha/Calculations/main_Palpha.cpp
    core/Diag_Palpha/Input_data/Palpha_input.cpp
    core/Diag_Palpha/Output_data/Palpha_output.cpp
    core/Diag_Palpha/Calculations/calc_Palpha.cpp
    core/common/common_types.cpp
    ind_Palpha_path_manager.cpp
)

# =====================================================================================
# 4. wxWidgets GUI (wx_app.exe)
# =====================================================================================

# ищем wxWidgets через wx-config
find_package(wxWidgets REQUIRED COMPONENTS core base)

include(${wxWidgets_USE_FILE})

add_executable(wx_app gui_wx/gui_main.cpp)
target_link_libraries(wx_app ${wxWidgets_LIBRARIES})

#======================================================================================
add_executable(kinematic_gui
    gui_wx/kinematic_gui.cpp
)

target_link_libraries(kinematic_gui PRIVATE ${wxWidgets_LIBRARIES})
