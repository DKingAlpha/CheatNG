cmake_minimum_required(VERSION 3.27.6)

option(BUILD_FOR_NINTENDO_SWITCH "Build for Nintendo Switch" OFF)

if (BUILD_FOR_NINTENDO_SWITCH)
    include($ENV{DEVKITPRO}/cmake/Switch.cmake)
    include($ENV{DEVKITPRO}/cmake/Platform/NintendoSwitch.cmake)
endif()

project(CheatNG C CXX)

set(CMAKE_CXX_STANDARD 20)

### freetype2 and lunasvg (for rendering twemoji)
add_subdirectory(thirdparty/lunasvg)

find_package(Freetype REQUIRED)

###### imgui
file(GLOB SOURCES_IMGUI "thirdparty/imgui/*.cpp"
    "thirdparty/imgui/backends/imgui_impl_glfw.cpp"
    "thirdparty/imgui/backends/imgui_impl_opengl3.cpp"
    "thirdparty/imgui/misc/freetype/imgui_freetype.cpp"
    "thirdparty/imgui/misc/cpp/imgui_stdlib.cpp"
)
set(HEADER_DIRS_IMGUI
    "thirdparty/imgui"
    "thirdparty/imgui/backends"
    "thirdparty/imgui/misc/freetype"
    "thirdparty/imgui/misc/cpp"
    "thirdparty/lunasvg/include"
    ${FREETYPE_INCLUDE_DIR_ft2build}
    ${FREETYPE_INCLUDE_DIR_freetype2}
)

set(IMGUI_DEFINITIONS
    "IMGUI_ENABLE_FREETYPE"
    "IMGUI_USE_WCHAR32"
    "IMGUI_ENABLE_FREETYPE_LUNASVG"
    "IMGUI_IMPL_OPENGL_ES2"
)

add_library(imgui STATIC ${SOURCES_IMGUI})
target_include_directories(imgui PUBLIC ${HEADER_DIRS_IMGUI})
target_compile_definitions(imgui PUBLIC ${IMGUI_DEFINITIONS})
set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
# glfw and opengl
if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    message(STATUS "Using Switch GLFW")
    set(HEADER_DIRS_GLFW ${DKP_INSTALL_PREFIX_INIT}/include)
    set(LIBRARY_DIRS_GLFW ${DKP_INSTALL_PREFIX_INIT}/lib)
    set(IMGUI_LIBRARIES "lunasvg;glfw3;GLESv2;EGL;glapi;drm_nouveau" ${FREETYPE_LIBRARY})
else()
    message(STATUS "Using Host GLFW")
    set(GLFW_DIR "thirdparty/glfw") # Set this to point to an up-to-date GLFW repo
    set(HEADER_DIRS_GLFW "${GLFW_DIR}/include")
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ON)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
    add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
    set(IMGUI_LIBRARIES "lunasvg;glfw;GL" ${FREETYPE_LIBRARY})
endif()
include_directories(${HEADER_DIRS_GLFW})
link_directories(${LIBRARY_DIRS_GLFW})


###### lib
file(GLOB SOURCES_LIB "lib/*.cpp" "lib/imp/linux/*.cpp")
set(HEADER_DIRS_LIB "lib")

add_library(mem STATIC ${SOURCES_LIB})
target_include_directories(mem PUBLIC ${HEADER_DIRS_LIB})


# gui
file(GLOB_RECURSE SOURCES_GUI "gui/*.cpp")
set(HEADER_DIRS_GUI "gui")

add_executable(CheatNG ${SOURCES_GUI})
target_include_directories(CheatNG PUBLIC ${HEADER_DIRS_GUI} ${HEADER_DIRS_LIB} ${HEADER_DIRS_IMGUI} ${HEADER_DIRS_GLFW})
target_link_libraries(CheatNG imgui mem ${IMGUI_LIBRARIES})
target_compile_definitions(CheatNG PRIVATE ${IMGUI_DEFINITIONS})

if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    nx_create_nro(CheatNG)
endif()
