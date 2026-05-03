cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME project)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# 模块化源文件配置
# sloop 内核
set(SLOOP_KERNEL_SOURCES
    user/sloop/kernel/sloop.c
    user/sloop/sl_slice.c
)

# SEGGER RTT
set(SLOOP_RTT_SOURCES
    user/sloop/RTT/SEGGER_RTT.c
    user/sloop/RTT/SEGGER_RTT_printf.c
)

# user/app 应用
set(USER_APP_SOURCES
    user/app/_main.c
    user/app/tasks/task_baseInit.c
    user/app/tasks/task_demo.c
    user/app/tasks/task_flow.c
    user/app/tasks/task_idle.c
    user/app/tasks/task_slice.c
)

# 合并所有源文件
set(ALL_SOURCES
    ${SLOOP_KERNEL_SOURCES}
    ${SLOOP_RTT_SOURCES}
    ${USER_APP_SOURCES}
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${ALL_SOURCES})

# 模块化头文件目录
set(SLOOP_INCLUDE_DIRS
    user/sloop
    user/sloop/RTT
    user/app
    user/app/config
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${SLOOP_INCLUDE_DIRS})

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
