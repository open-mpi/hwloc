#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2025-2026 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

# --- Platform Detection ---
if(MSVC)
    set(HWLOC_COMPILER_MSVC 1)
else()
    set(HWLOC_COMPILER_MSVC 0)
endif()

# Detect x86/ARM architecture
set(HWLOC_HAVE_X86_CPUID 0)
set(HWLOC_X86_32_ARCH 0)
set(HWLOC_X86_64_ARCH 0)

if(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "^(x64|x86_64)$")
    # "x64"    for Windows (MSVC)
    # "x86_64" for Windows (MinGW), Linux, macOS
    set(HWLOC_X86_64_ARCH 1)
    set(HWLOC_HAVE_X86_CPUID 1)
elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "^(X86|i[3-6]86)$")
    # "X86"      for Windows (MSVC)
    # "i[3-6]86" for Windows (MinGW), Linux, macOS
    set(HWLOC_X86_32_ARCH 1)
    set(HWLOC_HAVE_X86_CPUID 1)
else()
    # For "arm64", "aarch64", "ARM64"...
    set(HWLOC_HAVE_X86_CPUID 0)
endif()

message(STATUS "Target Arch ID = ${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
message(STATUS "HWLOC_HAVE_X86_CPUID = ${HWLOC_HAVE_X86_CPUID}")

# --- Optional Dependencies ---

# LibXml2
set(HWLOC_HAVE_LIBXML2 0)
if(HWLOC_WITH_LIBXML2)
    find_package(LibXml2 QUIET)
    if(LibXml2_FOUND)
        set(HWLOC_HAVE_LIBXML2 1)
        message(STATUS "Found LibXml2: ${LibXml2_INCLUDE_DIRS}")
    else()
        message(WARNING "LibXml2 requested but not found. XML support will be disabled.")
        set(HWLOC_WITH_LIBXML2 OFF CACHE BOOL "Use libxml2" FORCE)
    endif()
endif()

# OpenCL
set(HWLOC_HAVE_OPENCL 0)
if(HWLOC_WITH_OPENCL)
    if(HWLOC_OPENCL_PATH)
        find_package(OpenCL QUIET PATHS ${HWLOC_OPENCL_PATH} NO_DEFAULT_PATH)
    else()
        find_package(OpenCL QUIET)
    endif()
    if(OpenCL_FOUND)
        set(HWLOC_HAVE_OPENCL 1)
        message(STATUS "Found OpenCL: ${OpenCL_INCLUDE_DIRS}")
    else()
        message(WARNING "OpenCL requested but not found. OpenCL support will be disabled.")
        set(HWLOC_WITH_OPENCL OFF CACHE BOOL "Enable OpenCL support" FORCE)
    endif()
endif()

# Vulkan
set(HWLOC_HAVE_VULKAN 0)
if(HWLOC_WITH_VULKAN)
    if(HWLOC_VULKAN_PATH)
        find_package(Vulkan QUIET PATHS ${HWLOC_VULKAN_PATH} NO_DEFAULT_PATH)
    else()
        find_package(Vulkan QUIET)
    endif()
    if(Vulkan_FOUND)
        set(HWLOC_HAVE_VULKAN 1)
        message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS}")
    else()
        message(WARNING "Vulkan requested but not found. Vulkan support will be disabled.")
        set(HWLOC_WITH_VULKAN OFF CACHE BOOL "Enable Vulkan support" FORCE)
    endif()
endif()

# CUDA
set(HAVE_CUDA 0)
set(HAVE_CUDA_H 0)
set(HAVE_CUDA_RUNTIME_API_H 0)
set(HWLOC_HAVE_CUDART 0)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    if(HWLOC_WITH_CUDA)
        if(HWLOC_CUDA_PATH)
            find_package(CUDAToolkit QUIET PATHS ${HWLOC_CUDA_PATH} NO_DEFAULT_PATH)
        else()
            find_package(CUDAToolkit QUIET)
        endif()
        if(CUDAToolkit_FOUND)
            set(HAVE_CUDA 1)
            set(HAVE_CUDA_H 1)
            set(HAVE_CUDA_RUNTIME_API_H 1)
            set(HWLOC_HAVE_CUDART 1)
            message(STATUS "Found CUDA Toolkit: ${CUDAToolkit_INCLUDE_DIRS}")
        else()
            message(WARNING "CUDA requested but not found. CUDA support will be disabled.")
            set(HWLOC_WITH_CUDA OFF CACHE BOOL "Enable CUDA support" FORCE)
        endif()
    endif()
endif()

# --- Plugin Support ---
set(HWLOC_HAVE_PLUGINS 0)
set(HWLOC_ENABLED_PLUGINS_LIST)

if(HWLOC_ENABLE_PLUGINS)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        message(STATUS "HWLOC plugin support requires BUILD_SHARED_LIBS to be enabled, but it was not set. Setting BUILD_SHARED_LIBS to ON")
        set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
    elseif(NOT BUILD_SHARED_LIBS)
        message(FATAL_ERROR "HWLOC plugin support requires BUILD_SHARED_LIBS to be enabled, but BUILD_SHARED_LIBS is set to ${BUILD_SHARED_LIBS}.")
    endif()

    set(HWLOC_HAVE_PLUGINS 1)
    set(HWLOC_PLUGINS_PATH ${CMAKE_INSTALL_PREFIX}/lib/hwloc)

    if(HWLOC_HAVE_CUDART)
        list(APPEND HWLOC_ENABLED_PLUGINS_LIST "cuda")
    endif()

    if(HWLOC_HAVE_OPENCL)
        list(APPEND HWLOC_ENABLED_PLUGINS_LIST "opencl")
    endif()

    if(HWLOC_HAVE_VULKAN)
        list(APPEND HWLOC_ENABLED_PLUGINS_LIST "vulkan")
    endif()

    if(HWLOC_HAVE_LIBXML2)
        list(APPEND HWLOC_ENABLED_PLUGINS_LIST "xml_libxml")
    endif()
endif()

# --- Compiler Checks ---
include(CheckIncludeFile)
include(CheckCSourceCompiles)

check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("malloc.h" HAVE_MALLOC_H)
check_include_file("memory.h" HAVE_MEMORY_H)

# --- Configure Files ---
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contrib/windows-cmake/private_config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/private/autogen/config.h
               @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contrib/windows-cmake/static-components.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/static-components.h
               @ONLY)
