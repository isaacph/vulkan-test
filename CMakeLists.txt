cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0092 NEW) # to not warn about /W3 flags being overriden
project(VulkanTest
    VERSION 1.0
    DESCRIPTION "Learning vulkan and MSVC toolchain"
    LANGUAGES C)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED True)

# use Ninja generator with MSVC with this to get intellisense on windows
# outside of Visual Studio
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_PREFIX_PATH "/prefix")
# set(as_subproject
#     Vulkan::ValidationLayers
#     Vulkan::Headers
#     Vulkan::Loader
#     volk::volk
#     Vulkan::LayerSettings
#     Vulkan::UtilityHeaders
#     SPIRV-Headers::SPIRV-Headers
#     SPIRV-Tools::SPIRV-Tools
#     )
# 
# foreach(proj ${as_subproject})
#     set(CMAKE_DISABLE_FIND_PACKAGE_${proj} TRUE)
#     message(CMAKE_DISABLE_FIND_PACKAGE_${proj} ${CMAKE_DISABLE_FIND_PACKAGE_${proj}})
# endforeach()

# # this doesn't work on rerun
# # disable find package for non-subprojects
# macro(find_package)
#     if(NOT "${ARG0}" IN_LIST as_subproject)
#         _find_package(${ARGV})
#     endif()
# endmacro()

add_subdirectory(subprojects/Vulkan-ValidationLayers)
add_subdirectory(subprojects/Vulkan-Headers)
add_subdirectory(subprojects/Vulkan-Loader)
add_subdirectory(subprojects/Vulkan-Utility-Libraries)
add_subdirectory(subprojects/SPIRV-Headers)
add_subdirectory(subprojects/SPIRV-Tools)
# set(VOLK_PULL_IN_VULKAN OFF)
set(VOLK_INSTALL ON)
add_subdirectory(subprojects/volk)

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


configure_file(src/config.h.in config.h)
add_executable(VulkanTest
    src/main.c
    src/render.c
    src/render_util.c
    src/segfault.c
    src/backtrace.c
    src/segfault.c
    )
# target_include_directories(VulkanTest PRIVATE src)
# target_include_directories(VulkanTest PRIVATE "subprojects/Vulkan-Headers/include")
# target_include_directories(VulkanTest PRIVATE "subprojects/volk")
target_include_directories(VulkanTest PRIVATE src)

target_link_libraries(VulkanTest
    Vulkan::Loader
    Vulkan::Headers
    # vvl # should be Vulkan::Validationlayers :( actually can't link this lol
    volk::volk
    )

if (WIN32)
    target_link_libraries(VulkanTest dbghelp)
endif (WIN32)
install(TARGETS VulkanTest)

# use this to print targets wherever you paste it
function(print_all_targets DIR)
    get_property(TGTS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(TGT IN LISTS TGTS)
        message(STATUS "Target: ${TGT}")
        # TODO: Do something about it
    endforeach()

    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        print_all_targets("${SUBDIR}")
    endforeach()
endfunction()

message("ALL TARGETS BELOW")
# print_all_targets(.)
