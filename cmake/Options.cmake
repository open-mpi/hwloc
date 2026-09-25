#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2025-2026 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

# Build options
option(HWLOC_ENABLE_TESTING "Enable testing" ON)
option(HWLOC_ENABLE_PLUGINS "Enable plugin support" OFF)
option(HWLOC_SKIP_LSTOPO "don't build/install lstopo")
option(HWLOC_SKIP_TOOLS "don't build/install other hwloc tools")
option(HWLOC_SKIP_INCLUDES "don't install headers")

# Feature options
option(HWLOC_WITH_LIBXML2 "Use libxml2 instead of minimal XML" OFF)

# Hardware support options (users provide paths via -D)
option(HWLOC_WITH_OPENCL "Enable OpenCL support" OFF)
set(HWLOC_OPENCL_PATH "" CACHE PATH "Path to OpenCL SDK (optional)")

option(HWLOC_WITH_VULKAN "Enable Vulkan support" OFF)
set(HWLOC_VULKAN_PATH "" CACHE PATH "Path to Vulkan SDK (optional)")

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    option(HWLOC_WITH_CUDA "Enable CUDA support" OFF)
    set(HWLOC_CUDA_PATH "" CACHE PATH "Path to CUDA toolkit (optional)")
endif()
