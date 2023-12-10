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

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(SHADERS_GENERATED "${PROJECT_BINARY_DIR}/include/shaders_generated.h")
file(WRITE ${SHADERS_GENERATED} "\
#ifndef SHADERS_GENERATED_H_INCLUDED
#define SHADERS_GENERATED_H_INCLUDED
")

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
    )

set(CAT_SCRIPT ${PROJECT_SOURCE_DIR}/cat.cmake)
set(SHADERS_GENERATED "${PROJECT_BINARY_DIR}/include/shaders_generated.h")
set(SHADERS_GENERATED_prev "${PROJECT_BINARY_DIR}/shaders/shaders_generated_start.h")
set(SHADERS_GENERATED_end "${PROJECT_BINARY_DIR}/shaders/shaders_generated_end.h")
file(WRITE ${SHADERS_GENERATED_prev}
  "#ifndef SHADERS_GENERATED_H_INCLUDED\n"
  "#define SHADERS_GENERATED_H_INCLUDED\n")
file(WRITE ${SHADERS_GENERATED_end} "#endif\n")

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  set(SPIRV_INC "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv.h")
  set(SHADERS_GENERATED_next "${PROJECT_BINARY_DIR}/shaders/shaders_generated_${FILE_NAME}.h")
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV_INC}
    COMMAND xxd -i -n SHADER_${FILE_NAME} ${SPIRV} ${SPIRV_INC}
    DEPENDS ${SPIRV})
  add_custom_command(
    OUTPUT ${SHADERS_GENERATED_next}
    COMMAND ${CMAKE_COMMAND} -DOUT_FILE=${SHADERS_GENERATED_next} -DIN_FILE=${SHADERS_GENERATED_prev} -DCAT_FILE=${SPIRV_INC} -P ${CAT_SCRIPT}
    DEPENDS ${SPIRV_INC} ${SHADERS_GENERATED_prev}
    )

  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  list(APPEND SPIRV_INCLUDE_FILES ${SPIRV_INC})

  set(SHADERS_GENERATED_prev ${SHADERS_GENERATED_next})
endforeach(GLSL)

add_custom_command(
  OUTPUT ${SHADERS_GENERATED}
  COMMAND ${CMAKE_COMMAND} -DOUT_FILE=${SHADERS_GENERATED} -DIN_FILE=${SHADERS_GENERATED_prev} -DCAT_FILE=${SHADERS_GENERATED_end} -P ${CAT_SCRIPT}
  DEPENDS ${SHADERS_GENERATED_prev})

add_custom_target(
    Shaders
    DEPENDS
      ${SHADERS_GENERATED}
    )

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
    src/shader.c
    shaders/triangle.frag.spv.h
    shaders/triangle.vert.spv.h
    # src/ccomexample.c
    )
add_dependencies(VulkanTest Shaders)
# target_compile_definitions(VulkanTest PRIVATE UNICODE _UNICODE)
# target_include_directories(VulkanTest PRIVATE src)

target_include_directories(VulkanTest PRIVATE src)
target_include_directories(VulkanTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
# target_include_directories(VulkanTest PRIVATE Vulkan::Loader)
# target_include_directories(VulkanTest PRIVATE Vulkan::Headers)
# # target_include_directories(VulkanTest PRIVATE "subprojects/Vulkan-Headers/include")
# # target_include_directories(VulkanTest PRIVATE "subprojects/volk")

target_link_libraries(VulkanTest
    #    Vulkan::Loader
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
