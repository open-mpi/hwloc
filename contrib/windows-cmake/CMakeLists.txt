#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2021-2024 Inria.  All rights reserved.
# Copyright © 2025 Siemens Corporation and/or its affiliates.  All rights reserved.
# See COPYING in top-level directory.
#

cmake_minimum_required(VERSION 3.15)
project(hwloc
    LANGUAGES C
    VERSION 3.0.0)

enable_testing()

# --- Include modular CMake files ---
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
include(Options)
include(Dependencies)

# --- Source Directory ---
set(TOPDIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# --- Configure Files ---
configure_file(${TOPDIR}/contrib/windows/hwloc_config.h
               ${CMAKE_CURRENT_BINARY_DIR}/include/hwloc/autogen/config.h
               COPYONLY)

# --- Library ---
add_compile_definitions($<$<CONFIG:DEBUG>:HWLOC_DEBUG=1>)

add_library(hwloc
    ${TOPDIR}/hwloc/topology.c
    ${TOPDIR}/hwloc/traversal.c
    ${TOPDIR}/hwloc/distances.c
    ${TOPDIR}/hwloc/memattrs.c
    ${TOPDIR}/hwloc/cpukinds.c
    ${TOPDIR}/hwloc/components.c
    ${TOPDIR}/hwloc/bind.c
    ${TOPDIR}/hwloc/bitmap.c
    ${TOPDIR}/hwloc/pci-common.c
    ${TOPDIR}/hwloc/diff.c
    ${TOPDIR}/hwloc/shmem.c
    ${TOPDIR}/hwloc/misc.c
    ${TOPDIR}/hwloc/base64.c
    ${TOPDIR}/hwloc/topology-noos.c
    ${TOPDIR}/hwloc/topology-synthetic.c
    ${TOPDIR}/hwloc/topology-xml.c
    ${TOPDIR}/hwloc/topology-xml-nolibxml.c
    ${TOPDIR}/hwloc/topology-windows.c
    $<$<BOOL:${HWLOC_HAVE_X86_CPUID}>:${TOPDIR}/hwloc/topology-x86.c>
    $<$<AND:$<BOOL:${HWLOC_HAVE_LIBXML2}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:${TOPDIR}/hwloc/topology-xml-libxml.c>
    $<$<AND:$<BOOL:${HWLOC_HAVE_OPENCL}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:${TOPDIR}/hwloc/topology-opencl.c>
    $<$<AND:$<BOOL:${HWLOC_HAVE_VULKAN}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:${TOPDIR}/hwloc/topology-vulkan.c>
    $<$<AND:$<BOOL:${HAVE_CUDA}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:${TOPDIR}/hwloc/topology-cuda.c>
)

target_link_libraries(hwloc PRIVATE
    $<$<AND:$<BOOL:${HWLOC_HAVE_LIBXML2}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:LibXml2::LibXml2>
    $<$<AND:$<BOOL:${HWLOC_HAVE_OPENCL}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:OpenCL::OpenCL>
    $<$<AND:$<BOOL:${HWLOC_HAVE_VULKAN}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:Vulkan::Vulkan>
    $<$<AND:$<BOOL:${HAVE_CUDA}>,$<NOT:$<BOOL:${HWLOC_HAVE_PLUGINS}>>>:CUDA::cudart;CUDA::cuda_driver>"
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(hwloc PRIVATE $<$<BOOL:${MSVC}>:_USRDLL>)
endif()

target_include_directories(hwloc PUBLIC
    "$<BUILD_INTERFACE:${TOPDIR}/include;${CMAKE_CURRENT_BINARY_DIR}/include>"
    $<INSTALL_INTERFACE:include>
)

# --- Plugin Support ---
if(HWLOC_ENABLE_PLUGINS)
    message(STATUS "Enabled plugins:")
    foreach(plugin IN LISTS HWLOC_ENABLED_PLUGINS_LIST)
        message(STATUS "  - ${plugin}")
    endforeach()

    foreach(plugin IN LISTS HWLOC_ENABLED_PLUGINS_LIST)
        if(plugin STREQUAL "cuda")
            add_library(hwloc_${plugin} MODULE ${TOPDIR}/hwloc/topology-${plugin}.c)
            target_link_libraries(hwloc_${plugin} PRIVATE hwloc CUDA::cudart CUDA::cuda_driver)
        elseif(plugin STREQUAL "opencl")
            add_library(hwloc_${plugin} MODULE ${TOPDIR}/hwloc/topology-${plugin}.c)
            target_link_libraries(hwloc_${plugin} PRIVATE hwloc OpenCL::OpenCL)
        elseif(plugin STREQUAL "vulkan")
            add_library(hwloc_${plugin} MODULE ${TOPDIR}/hwloc/topology-${plugin}.c)
            target_link_libraries(hwloc_${plugin} PRIVATE hwloc Vulkan::Vulkan)
        elseif(plugin STREQUAL "xml_libxml")
            add_library(hwloc_${plugin} MODULE ${TOPDIR}/hwloc/topology-xml-libxml.c)
            target_link_libraries(hwloc_${plugin} PRIVATE hwloc LibXml2::LibXml2)
        endif()

        target_compile_definitions(hwloc_${plugin} PRIVATE "HWLOC_INSIDE_PLUGIN")
        target_compile_definitions(hwloc_${plugin} PRIVATE $<$<BOOL:${MSVC}>:_USRDLL>)
    endforeach()
endif()

# --- Tools ---
if(NOT HWLOC_SKIP_TOOLS)
    set(TOOLS
        hwloc-bind
        hwloc-calc
        hwloc-diff
        hwloc-distrib
        hwloc-info
        hwloc-patch
    )

    if(HWLOC_HAVE_X86_CPUID)
        list(APPEND TOOLS hwloc-gather-cpuid)
    endif()

    foreach(tool IN ITEMS ${TOOLS})
        add_executable(${tool} ${TOPDIR}/utils/hwloc/${tool}.c)
        target_link_libraries(${tool} PRIVATE hwloc)
    endforeach()
endif()

# --- lstopo ---
if(NOT HWLOC_SKIP_LSTOPO)
    set(LSTOPOS lstopo-no-graphics lstopo lstopo-win)
    set(LSTOPO_COMMON_SOURCES
        ${TOPDIR}/utils/lstopo/lstopo.c
        ${TOPDIR}/utils/lstopo/lstopo-draw.c
        ${TOPDIR}/utils/lstopo/lstopo-tikz.c
        ${TOPDIR}/utils/lstopo/lstopo-fig.c
        ${TOPDIR}/utils/lstopo/lstopo-svg.c
        ${TOPDIR}/utils/lstopo/lstopo-ascii.c
        ${TOPDIR}/utils/lstopo/lstopo-text.c
        ${TOPDIR}/utils/lstopo/lstopo-xml.c
        ${TOPDIR}/utils/hwloc/common-ps.c
    )

    add_executable(lstopo-no-graphics ${LSTOPO_COMMON_SOURCES})
    target_link_libraries(lstopo-no-graphics PRIVATE hwloc)

    add_executable(lstopo ${LSTOPO_COMMON_SOURCES} ${TOPDIR}/utils/lstopo/lstopo-windows.c)
    target_compile_definitions(lstopo PRIVATE LSTOPO_HAVE_GRAPHICS)
    target_link_libraries(lstopo PRIVATE hwloc)

    add_executable(lstopo-win WIN32 ${LSTOPO_COMMON_SOURCES} ${TOPDIR}/utils/lstopo/lstopo-windows.c)
    target_compile_definitions(lstopo-win PRIVATE LSTOPO_HAVE_GRAPHICS)
    target_link_options(lstopo-win PRIVATE "$<$<BOOL:${MSVC}>:/subsystem:windows;/entry:mainCRTStartup>")
    target_link_libraries(lstopo-win PRIVATE hwloc)

    foreach(tool IN ITEMS ${LSTOPOS})
        target_include_directories(${tool} PRIVATE ${TOPDIR}/utils/hwloc)
        target_link_libraries(${tool} PRIVATE hwloc)
    endforeach()
endif()

# --- Misc ---
foreach(target IN ITEMS hwloc ${TOOLS} ${LSTOPOS})
    target_compile_definitions(${target} PRIVATE $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>)
endforeach()

# --- Install ---
install(TARGETS hwloc)

if(HWLOC_ENABLE_PLUGINS)
    foreach(plugin IN LISTS HWLOC_ENABLED_PLUGINS_LIST)
        install(TARGETS hwloc_${plugin}
                LIBRARY DESTINATION lib/hwloc
                ARCHIVE DESTINATION lib/hwloc
                RUNTIME DESTINATION bin/hwloc)
    endforeach()
endif()

if(NOT HWLOC_SKIP_TOOLS)
    install(TARGETS ${TOOLS})
endif()

if(NOT HWLOC_SKIP_LSTOPO)
    install(TARGETS ${LSTOPOS})
endif()

if(NOT HWLOC_SKIP_INCLUDES)
    install(FILES ${TOPDIR}/include/hwloc.h TYPE INCLUDE)
    install(DIRECTORY ${TOPDIR}/include/hwloc TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/hwloc/autogen/config.h DESTINATION include/hwloc/autogen)
endif()

if(MSVC AND BUILD_SHARED_LIBS)
    install(FILES $<TARGET_PDB_FILE:hwloc> TYPE BIN OPTIONAL)
endif()

# --- Testing ---
if(HWLOC_ENABLE_TESTING)
    add_subdirectory(tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
