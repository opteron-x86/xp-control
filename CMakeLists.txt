cmake_minimum_required(VERSION 3.28)
project(XPControl VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(XPCONTROL_BUILD_PLUGIN "Build the Windows SKSE DLL" ${WIN32})
option(XPCONTROL_BUILD_TESTS "Build portable logic tests" ON)

add_library(xp_control_core STATIC src/Settings.cpp)
target_include_directories(xp_control_core PUBLIC src)
if(XPCONTROL_BUILD_TESTS)
    enable_testing()
    add_executable(xp_control_tests tests/CoreTests.cpp)
    target_link_libraries(xp_control_tests PRIVATE xp_control_core)
    add_test(NAME curve_settings_session COMMAND xp_control_tests)
endif()

if(XPCONTROL_BUILD_PLUGIN)
    if(NOT WIN32)
        message(FATAL_ERROR "Use a Windows compiler/toolchain to build the plugin.")
    endif()
    include(FetchContent)
    # Compile the pinned dependency with SE, AE, and VR layouts.
    set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
    set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
    set(ENABLE_SKYRIM_VR ON CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "CommonLib tests" FORCE)
    set(COMMONLIB_PREBUILT OFF CACHE BOOL "Build the pinned source" FORCE)
    set(COMMONLIB_PREBUILT_DIR "" CACHE PATH "Build the pinned source" FORCE)
    set(SKSE_SUPPORT_PATCH_SAFETY OFF CACHE BOOL "Instruction decoder support" FORCE)
    set(COMMONLIB_ENABLE_IPO OFF CACHE BOOL "CommonLib interprocedural optimization" FORCE)
    FetchContent_Declare(CommonLibSSE
        GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
        GIT_TAG 12d61f3c39f7d4ba1b1ab543df2bca0e613dc747
        GIT_SUBMODULES extern/openvr
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(CommonLibSSE)
    # DirectX headers use the Windows 10 SDK declarations.
    foreach(property COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
        get_target_property(definitions CommonLibSSE ${property})
        list(FILTER definitions EXCLUDE REGEX "^(WINVER|_WIN32_WINNT)=")
        list(APPEND definitions WINVER=0x0A00 _WIN32_WINNT=0x0A00)
        set_property(TARGET CommonLibSSE PROPERTY ${property} "${definitions}")
    endforeach()
    add_library(XPControl SHARED src/Plugin.cpp src/Runtime.cpp src/Menu.cpp)
    target_include_directories(XPControl PRIVATE third_party)
    target_compile_definitions(XPControl PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_compile_definitions(XPControl PRIVATE
        XPCONTROL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        XPCONTROL_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        XPCONTROL_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        XPCONTROL_VERSION="${PROJECT_VERSION}")
    target_compile_options(XPControl PRIVATE /EHsc /utf-8 /permissive- /Zc:__cplusplus
        /W4 /wd4100 /wd4200 /wd4201 /wd4324 /wd4458 /wd4625 /wd4626 /wd4820 /wd4996 /wd5026
        /wd5027 /wd5039 /wd5045 /wd5267 /Zi)
    target_compile_options(xp_control_core PRIVATE /EHsc /utf-8 /W4 /Zi)
    target_compile_definitions(xp_control_core PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_link_options(XPControl PRIVATE /DEBUG:FULL)
    target_link_libraries(XPControl PRIVATE xp_control_core CommonLibSSE)
    set_target_properties(XPControl PROPERTIES PREFIX "" OUTPUT_NAME "XPControl")
    install(TARGETS XPControl RUNTIME DESTINATION SKSE/Plugins LIBRARY DESTINATION SKSE/Plugins)
    install(FILES package/SKSE/Plugins/XPControl.ini DESTINATION SKSE/Plugins)
endif()
