cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0092 NEW) # to not warn about /W3 flags being overriden
project(VulkanTest
    VERSION 1.0
    DESCRIPTION "Learning vulkan and CMake on windows"
    LANGUAGES C)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)
# # use Ninja generator with MSVC with this to get intellisense on windows
# # outside of Visual Studio
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# this breaks vulkan
# if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
#     set(CMAKE_ENABLE_EXPORTS TRUE)
# endif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")

if (WIN32)
# Settings for configuring projects generated for an IDE
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    include_directories($ENV{UniversalCRTSdkDir}/Include/ucrt)
    include_directories($ENV{UniversalCRTSdkDir}/Include/10.0.10056.0/ucrt)
endif (WIN32)

find_package(VulkanHeaders)
find_package(VulkanLoader)

# if (UNIX)
#     find_package(PkgConfig)
# endif (UNIX)

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

# set(SUBSYSTEM_LINKER_OPTIONS "/SUBSYSTEM:WINDOWS")

configure_file(src/config.h.in config.h)
add_library(Main STATIC
    src/util/backtrace.c
    src/util/memory.c
    src/util/utf8.c
    src/util/utf16.c
    src/render/device.c
    src/render/functions.c
    src/render/image.c
    src/render/instance.c
    src/render/loop.c
    src/render/swapchain.c
    src/render/util.c
    src/render/win32.c
    src/render/wayland.c
    src/util/stack.c
    )
add_dependencies(Main Shaders)
target_include_directories(Main PUBLIC src)
target_include_directories(Main PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(Main
    Vulkan::Headers
    )
if (WIN32)
    target_link_libraries(Main dbghelp legacy_stdio_definitions)
endif (WIN32)
if (UNIX)
    target_link_libraries(Main backtrace)
endif (UNIX)

add_executable(Game src/main.c)
target_link_libraries(Game Main)
# target_compile_definitions(VulkanTest PRIVATE UNICODE _UNICODE)
# target_include_directories(VulkanTest PRIVATE src)


add_subdirectory(test)

# annoying, reevaluate later
# if(MSVC)
#     target_compile_options(VulkanTest PRIVATE /W4 /WX /Wall)
# else()
#     target_compile_options(VulkanTest PRIVATE -Wall -Wextra -Wpedantic -Werror)
# endif()

install(TARGETS Game)
 
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

