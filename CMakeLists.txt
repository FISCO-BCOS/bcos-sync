#------------------------------------------------------------------------------
# Top-level CMake file for bcos-sync
# ------------------------------------------------------------------------------
# Copyright (C) 2021 bcos-sync
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.3" CACHE STRING "Minimum OS X deployment version")

if (NOT DEFINED URL_BASE)
    set(URL_BASE "github.com")
    # set(URL_BASE "github.com.cnpmjs.org")
endif()

include(FetchContent)
set(BCOS_CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/bcos-cmake-scripts)
FetchContent_Declare(bcos-cmake-scripts
    GIT_REPOSITORY https://${URL_BASE}/FISCO-BCOS/bcos-cmake-scripts.git
    GIT_TAG dev
    SOURCE_DIR ${BCOS_CMAKE_SCRIPTS_DIR}
)
FetchContent_MakeAvailable(bcos-cmake-scripts)

set(BCOS_SYNC_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "The cmake path for the library")
list(APPEND CMAKE_MODULE_PATH ${BCOS_CMAKE_SCRIPTS_DIR})
list(APPEND CMAKE_MODULE_PATH ${BCOS_SYNC_CMAKE_DIR})
#init hunter
include(HunterGate)
HunterGate(
    URL "https://${URL_BASE}/FISCO-BCOS/hunter/archive/60389000be6e74983b8790e695ba1bd10e88fab7.tar.gz"
    SHA1 "8477caf1a595856cdc2c3b09568bbf9bf15f1f35"
    FILEPATH "${BCOS_SYNC_CMAKE_DIR}/config.cmake"
)
project(bcos-sync VERSION "3.0.0")
set(VERSION_SUFFIX "")

# basic settings
include(Options)
configure_project()
include(CompilerSettings)

# copy .clang-format into the project path
include(CopyClangFormat)

include_directories(${CMAKE_INSTALL_INCLUDEDIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# install dependencies
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)
include(InstallBcosFrameworkDependencies)

# define the generated proto file path
set(PROTO_PATH ${PROJECT_SOURCE_DIR})
set(PROTO_GENERATE_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(PROTO_SUB_DIR "bcos-sync/protocol/proto")

set(BLOCK_SYNC_TARGET "block-sync")
add_subdirectory(bcos-sync)

# install the target
include(InstallConfig)
install(
    TARGETS ${BLOCK_SYNC_TARGET}
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
include(InstallInclude)
if (TESTS) 
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(test) 
endif()

# for doxygen
include(BuildDocs)
buildDoc(bcos-sync-doc)

# for code coverage
if (COVERAGE)
    include(Coverage)
    config_coverage("sync-cov" "'/usr*' '${CMAKE_CURRENT_SOURCE_DIR}/bcos-cmake-scripts*' '${CMAKE_CURRENT_SOURCE_DIR}/test/bcos-test*'")
endif ()
