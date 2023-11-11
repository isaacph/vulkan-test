cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0092 NEW) # to not warn about /W3 flags being overriden
project(VulkanTest
    VERSION 1.0
    DESCRIPTION "Learning vulkan and MSVC toolchain"
    LANGUAGES C)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)
# # use Ninja generator with MSVC with this to get intellisense on windows
# # outside of Visual Studio
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)



# Settings for configuring projects generated for an IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories($ENV{UniversalCRTSdkDir}/Include/ucrt)
include_directories($ENV{UniversalCRTSdkDir}/Include/10.0.10056.0/ucrt)

# # this doesn't work on rerun
# # disable find package for non-subprojects
# macro(find_package)
#     if(NOT "${ARG0}" IN_LIST as_subproject)
#         _find_package(${ARGV})
#     endif()
# endmacro()

# set(CMAKE_PREFIX_PATH "/prefix")
# add_subdirectory(subprojects/Vulkan-ValidationLayers)
# add_subdirectory(subprojects/Vulkan-Headers)
# add_subdirectory(subprojects/Vulkan-Loader)
# add_subdirectory(subprojects/Vulkan-Utility-Libraries)
# add_subdirectory(subprojects/SPIRV-Headers)
# add_subdirectory(subprojects/SPIRV-Tools)
# set(VOLK_INSTALL ON)
# add_subdirectory(subprojects/volk)

find_package(VulkanHeaders)
find_package(VulkanLoader)

# include(CMakeFindDependencyMacro)

# include(ExternalProject)

# # GIT_RESPOSITORY "https://github.com/KhronosGroup/Vulkan-Headers.git"
# ExternalProject_Add(
#     VulkanHeaders
#     SOURCE_DIR "${CMAKE_SOURCE_DIR}/subprojects/Vulkan-Headers"
#     CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}"
#     )
# ExternalProject_Add(
#     VulkanLoader
#     SOURCE_DIR "${CMAKE_SOURCE_DIR}/subprojects/Vulkan-Loader"
#     CMAKE_ARGS -DVULKAN_HEADERS_INSTALL_DIR=${CMAKE_CURRENT_BINARY_DIR}
#                -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
#    DEPENDS VulkanHeaders)
# add_library(vulkan "${CMAKE_CURRENT_BINARY_DIR}/bin/vulkan-1.dll")
# 
# # include("subprojects/vulkan/vulkan.cmake")
# 
# link_directories("")

# include_directories("subprojects/Vulkan-Headers/include")

#set(ENV{VULKAN_SDK} "${CMAKE_SOURCE_DIR}/subprojects/Vulkan-Headers")

# set(SUBSYSTEM_LINKER_OPTIONS "/SUBSYSTEM:WINDOWS")

configure_file(src/config.h.in config.h)
add_executable(VulkanTest
    src/main.c
    src/render.c
    src/render_util.c
    src/backtrace.c
    # src/segfault.c
    src/render_functions.c
    # src/ccomexample.c
    )
# target_compile_definitions(VulkanTest PRIVATE UNICODE _UNICODE)
# target_include_directories(VulkanTest PRIVATE src)

target_include_directories(VulkanTest PRIVATE src)
# target_include_directories(VulkanTest PRIVATE Vulkan::Loader)
# target_include_directories(VulkanTest PRIVATE Vulkan::Headers)
# # target_include_directories(VulkanTest PRIVATE "subprojects/Vulkan-Headers/include")
# # target_include_directories(VulkanTest PRIVATE "subprojects/volk")

target_link_libraries(VulkanTest
    Vulkan::Loader
    Vulkan::Headers
# vvl # should be Vulkan::Validationlayers :( actually can't link this lol
#    volk::volk
    )

if (WIN32)
    target_link_libraries(VulkanTest dbghelp legacy_stdio_definitions)
endif (WIN32)

# annoying, reevaluate later
# if(MSVC)
#     target_compile_options(VulkanTest PRIVATE /W4 /WX /Wall)
# else()
#     target_compile_options(VulkanTest PRIVATE -Wall -Wextra -Wpedantic -Werror)
# endif()

install(TARGETS VulkanTest)
 
# # use this to print targets wherever you paste it
# function(print_all_targets DIR)
#     get_property(TGTS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
#     foreach(TGT IN LISTS TGTS)
#         message(STATUS "Target: ${TGT}")
#         # TODO: Do something about it
#     endforeach()
# 
#     get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
#     foreach(SUBDIR IN LISTS SUBDIRS)
#         print_all_targets("${SUBDIR}")
#     endforeach()
# endfunction()
# 
# message("ALL TARGETS BELOW")
# print_all_targets(.)
