# Set the minimum required version of CMake and project name
cmake_minimum_required(VERSION 3.16)
project(SteganographyTool)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- IMPORTANT: Set the path to your SFML installation ---
# !! Uncomment the line below and replace the path with the correct one on your system !!
# This path should point to the folder containing SFMLConfig.cmake
#
# set(SFML_DIR "D:/path/to/your/SFML-2.6.1/lib/cmake/SFML")


# --- Find the required SFML components ---
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# --- Create your application's executable ---
add_executable(StegTool main.cpp)

# For Windows, this creates a windowed app instead of a console app
if(WIN32)
    target_link_options(StegTool PRIVATE -mwindows)
endif()

# --- Configure third-party libraries (ImGui, etc.) ---
add_library(ImGui
        libs/imgui/imgui.cpp
        libs/imgui/imgui_draw.cpp
        libs/imgui/imgui_tables.cpp
        libs/imgui/imgui_widgets.cpp
        # Add necessary backend files for linking
        libs/imgui/backends/imgui_impl_opengl2.cpp
        libs/imgui/backends/imgui_impl_win32.cpp
)
# FIX: Tell the ImGui library where to find its own headers
target_include_directories(ImGui PUBLIC libs/imgui)

add_library(ImGui-SFML libs/imgui-sfml/imgui-SFML.cpp)


# --- Link all libraries to your executable ---
target_include_directories(ImGui-SFML PUBLIC libs/imgui)
target_link_libraries(ImGui-SFML PUBLIC sfml-graphics)

# Link StegTool in the correct order
target_link_libraries(StegTool PRIVATE
        ImGui-SFML
        ImGui
        sfml-graphics
        sfml-window
        sfml-system
        opengl32
)

# Tell StegTool where to find headers
target_include_directories(StegTool PRIVATE
        libs/imgui
        libs/imgui-sfml
        libs/pfd
)