#
# Copyright (c) 2008-2020 the Urho3D project.
# Copyright (c) 2020-2021 LucKey Productions.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if (NOT CMAKE_PROJECT_NAME STREQUAL Dry)
    # Set CMake minimum version and CMake policy required by DryCommon module
    cmake_minimum_required (VERSION 3.5.1)
    if(COMMAND cmake_policy AND CMAKE_MAJOR_VERSION VERSION_LESS 4)
        # Libraries linked via full path no longer produce linker search paths
        cmake_policy (SET CMP0003 NEW)
        # INTERFACE_LINK_LIBRARIES defines the link interface
        cmake_policy (SET CMP0022 NEW)
        # MACOSX_RPATH is enabled by default
        cmake_policy (SET CMP0042 NEW)
        # Disallow use of the LOCATION target property - so we set to OLD as we still need it
        cmake_policy (SET CMP0026 OLD)
        # Honor the visibility properties for SHARED target types only
        cmake_policy (SET CMP0063 OLD)
    endif ()

    # Set project name
    project (Dry-ExternalProject-PackageTool)

    # Set CMake modules search path
    set (CMAKE_MODULE_PATH ${BAKED_CMAKE_SOURCE_DIR}/CMake/Modules)

    # Include DryCommon.cmake module after setting project name
    include (DryCommon)

    # Define additional source files
    set (MINI_URHO_CPP_FILES
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Container/Allocator.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Container/HashBase.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Container/RefCounted.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Container/Str.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Container/VectorBase.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Context.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/EventProfiler.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Mutex.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Object.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/ProcessUtils.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Profiler.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/StringHashRegister.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/StringUtils.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Thread.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Timer.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Core/Variant.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/Deserializer.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/File.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/FileSystem.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/Log.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/MemoryBuffer.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/PackageFile.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/Serializer.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/IO/VectorBuffer.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Color.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Matrix3.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Matrix3x4.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Matrix4.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Quaternion.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Rect.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/StringHash.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Vector2.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Vector3.cpp
        ${BAKED_CMAKE_SOURCE_DIR}/Source/Dry/Math/Vector4.cpp
    )

    # Define preprocessor macros
    add_definitions (-DMINI_URHO)

    # Setup SDK-like include dir in the build tree for building the mini-urho
    set (DEST_INCLUDE_DIR include/Dry)
    file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)

    # Add dependency targets
    if (NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
        include (CheckIncludeFile)
        check_include_file (stdint.h HAVE_STDINT_H)
        add_subdirectory (${BAKED_CMAKE_SOURCE_DIR}/Source/ThirdParty/LibCpuId host/LibCpuId)
        set (LIBS LibCpuId)
    endif ()
    add_subdirectory (${BAKED_CMAKE_SOURCE_DIR}/Source/ThirdParty/LZ4 host/LZ4)
    set (INCLUDE_DIRS ${BAKED_CMAKE_BINARY_DIR}/include ${BAKED_CMAKE_BINARY_DIR}/include/Dry ${CMAKE_BINARY_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)
endif ()

# Define target name
if (TARGET PackageTool)     # The target name is already taken by host-tool external project, so use a postfix for the target-tool
    set (POSTFIX _target)
endif ()
set (TARGET_NAME PackageTool${POSTFIX})

# Define source files
define_source_files (EXTRA_CPP_FILES ${MINI_URHO_CPP_FILES})

# Define dependency libs
list (APPEND LIBS LZ4)

# Setup target
setup_executable (TOOL)
adjust_target_name ()   # Remove postfix from the executable/binary name
