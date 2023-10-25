cmake_minimum_required(VERSION 3.27)

project(CheatNG C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### freetype2 and lunasvg (for rendering twemoji)
add_subdirectory(thirdparty/freetype EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/lunasvg EXCLUDE_FROM_ALL)

set(GLFW_SUPPORTED ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(GLFW_SUPPORTED OFF)
endif()

###### imgui
file(GLOB SOURCES_IMGUI "thirdparty/imgui/*.cpp"
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
    "thirdparty/freetype/include"
    "thirdparty/cereal/include"
)

set(IMGUI_DEFINITIONS
    "IMGUI_ENABLE_FREETYPE"
    "IMGUI_USE_WCHAR32"
    "IMGUI_ENABLE_FREETYPE_LUNASVG"
)

if (GLFW_SUPPORTED)
    list(APPEND SOURCES_IMGUI "thirdparty/imgui/backends/imgui_impl_glfw.cpp")
    list(APPEND IMGUI_DEFINITIONS "IMGUI_IMPL_OPENGL_ES2")
else()
    list(APPEND SOURCES_IMGUI "thirdparty/imgui/backends/imgui_impl_android.cpp")
    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        list(APPEND SOURCES_IMGUI $ENV{ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    endif()
    list(APPEND IMGUI_DEFINITIONS "IMGUI_IMPL_OPENGL_ES3")
endif()


add_library(imgui STATIC ${SOURCES_IMGUI})
target_include_directories(imgui PUBLIC ${HEADER_DIRS_IMGUI})
target_compile_definitions(imgui PUBLIC ${IMGUI_DEFINITIONS})
set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
# glfw and opengl
if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    message(STATUS "Using Switch GLFW")
    set(HEADER_DIRS_GL ${DKP_INSTALL_PREFIX_INIT}/include)
    set(LIBRARY_DIRS_GL ${DKP_INSTALL_PREFIX_INIT}/lib)
    set(IMGUI_LIBRARIES "freetype;lunasvg;glfw3;GLESv2;EGL;glapi;drm_nouveau")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS "Using Android OpenGL3") # GLFW does not support Android
    set(IMGUI_LIBRARIES "android;log;freetype;lunasvg;GLESv3;EGL")
    set(HEADER_DIRS_GL $ENV{ANDROID_NDK}/sources/android/native_app_glue)
    # <format> is not supported by the LLVM libc++.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexperimental-library")
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fexperimental-library")
else()
    message(STATUS "Using Host GLFW")
    set(GLFW_DIR "thirdparty/glfw") # Set this to point to an up-to-date GLFW repo
    set(HEADER_DIRS_GL "${GLFW_DIR}/include")
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ON)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
    add_subdirectory(${GLFW_DIR} EXCLUDE_FROM_ALL)
    set(IMGUI_LIBRARIES "freetype;lunasvg;glfw;GL")
endif()
include_directories(${HEADER_DIRS_GL})
link_directories(${LIBRARY_DIRS_GL})


###### lib
file(GLOB SOURCES_LIB "lib/*.cpp" "lib/imp/linux/*.cpp")
set(HEADER_DIRS_LIB "lib")

add_library(mem STATIC ${SOURCES_LIB})
target_include_directories(mem PUBLIC ${HEADER_DIRS_LIB})


# gui
file(GLOB_RECURSE SOURCES_GUI "gui/*.cpp")
file(GLOB SOURCES_GUI_MAIN "gui/main_*.cpp")
list(REMOVE_ITEM SOURCES_GUI ${SOURCES_GUI_MAIN})
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND SOURCES_GUI "gui/main_android_imgui.cpp")
else()
    list(APPEND SOURCES_GUI "gui/main_glfw_imgui.cpp")
endif()

set(HEADER_DIRS_GUI "gui")

add_executable(CheatNG ${SOURCES_GUI})
target_include_directories(CheatNG PUBLIC ${HEADER_DIRS_GUI} ${HEADER_DIRS_LIB} ${HEADER_DIRS_IMGUI} ${HEADER_DIRS_GLFW})
target_link_libraries(CheatNG imgui mem ${IMGUI_LIBRARIES})
target_compile_definitions(CheatNG PRIVATE ${IMGUI_DEFINITIONS})

if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    nx_create_nro(CheatNG)
endif()
