cmake_minimum_required(VERSION 3.24)
project(STLCutStudioAI VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "Esta base requiere Windows y MSVC x64.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Configure un entorno MSVC x64.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Kit oficial instalado; build Release local verificado. Véase docs/WINDOWS_SETUP.md.
find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
get_target_property(qt_core_type Qt6::Core TYPE)
if(NOT qt_core_type STREQUAL "SHARED_LIBRARY")
    message(FATAL_ERROR "Esta configuracion requiere Qt dinamico.")
endif()

qt_standard_project_setup()
add_library(stlcut_core STATIC core/io/StlReader.cpp)
set_target_properties(stlcut_core PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(stlcut_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(stlcut_core PRIVATE /W4 /permissive- /utf-8)
qt_add_executable(STLCutStudioAI WIN32 apps/desktop/main.cpp apps/desktop/MeshView.cpp apps/desktop/MeshView.h)
target_link_libraries(STLCutStudioAI PRIVATE Qt6::Widgets Qt6::OpenGLWidgets Qt6::OpenGL stlcut_core)
target_compile_options(STLCutStudioAI PRIVATE /W4 /permissive- /utf-8)
target_compile_definitions(STLCutStudioAI PRIVATE
    STL_CUT_STUDIO_VERSION="${PROJECT_VERSION}")
install(TARGETS STLCutStudioAI RUNTIME DESTINATION .)
