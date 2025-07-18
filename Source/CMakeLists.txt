#
# Copyright (c) 2008-2020 the Urho3D project.
# Copyright (c) 2020-2023 LucKey Productions.
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

# Check existence of various header files and their functions required by some of the third-party libraries and Dry library
# Set the CMake variables in this scope but only add the compiler defines in the respective library's scope
include (CheckIncludeFile)
foreach (HEADER stdint.h inttypes.h malloc.h)
    string (TOUPPER HAVE_${HEADER} HAVE_HEADER)
    string (REPLACE . _ HAVE_HEADER ${HAVE_HEADER})
    check_include_file (${HEADER} ${HAVE_HEADER})
endforeach ()
include (CheckFunctionExists)
foreach (FUNCT __sincosf malloc_usable_size)
    string (TOUPPER HAVE_${FUNCT} HAVE_FUNCT)
    check_function_exists (${FUNCT} ${HAVE_FUNCT})
endforeach ()
include (CheckLibraryExists)
check_library_exists (m sincosf "" HAVE_SINCOSF)
if (MINGW)
    include (CheckStructHasMember)
    check_struct_has_member (RTL_OSVERSIONINFOW dwOSVersionInfoSize minwindef.h\;winnt.h HAVE_RTL_OSVERSIONINFOW)
endif ()

# Setup RPATH settings
if (DRY_LIB_TYPE STREQUAL SHARED AND NOT WIN32 AND NOT ANDROID AND NOT IOS AND NOT TVOS AND NOT WEB)
    # Add RPATH entries when building
    set (CMAKE_SKIP_BUILD_RPATH FALSE)
    # And set the RPATH entries so that the executable works both in the build tree and install destination
    set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    if (APPLE)
        set (CMAKE_MACOSX_RPATH TRUE)
        set (ORIGIN @loader_path)
    else ()
        set (ORIGIN $ORIGIN)
    endif ()
    # Library location relative to the executable in the build tree
    set (CMAKE_INSTALL_RPATH ${ORIGIN}/../lib)
    # Library location relative to the executable in the tool directory in the build tree
    list (APPEND CMAKE_INSTALL_RPATH ${ORIGIN}/../../lib)                               # The tools are installed one directory further down from normal executable
    # Library location relative to the executable in the install destination
    list (APPEND CMAKE_INSTALL_RPATH ${ORIGIN}/../lib${LIB_SUFFIX}/${PATH_SUFFIX})      # The library location is based on SDK install destination
    # Library location relative to the executable in the tool directory in the install destination
    list (APPEND CMAKE_INSTALL_RPATH ${ORIGIN}/../../lib${LIB_SUFFIX}/${PATH_SUFFIX})
    # The last entry to the install destination of the library, if the destination location is not in the system default search path, e.g. /usr/local/lib
    list (FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX} isSystemDir)
    if (isSystemDir STREQUAL -1)
        list (APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${DEST_LIBRARY_DIR})
    endif ()
endif ()

# Add targets
foreach (TARGET ETCPACK FreeType LZ4 PugiXml rapidjson SDL StanHull STB)
    add_subdirectory (ThirdParty/${TARGET})
endforeach ()

if (DRY_ANGELSCRIPT)
    add_subdirectory (ThirdParty/AngelScript)
    if (WEB OR (ARM AND DRY_64BIT))
        add_subdirectory (ThirdParty/boost)
    endif ()
endif ()

if (DRY_NETWORK)
    add_subdirectory (ThirdParty/Civetweb)
    add_subdirectory (ThirdParty/SLikeNet)
endif ()

if (DRY_DATABASE_ODBC)
    add_subdirectory (ThirdParty/nanodbc)
endif ()
if (DRY_DATABASE_SQLITE)
    add_subdirectory (ThirdParty/SQLite)
endif ()

if (DRY_IK)
    add_subdirectory (ThirdParty/ik)
endif ()

if (DRY_NAVIGATION)
    add_subdirectory (ThirdParty/Detour)
    add_subdirectory (ThirdParty/DetourCrowd)
    add_subdirectory (ThirdParty/DetourTileCache)
    add_subdirectory (ThirdParty/Recast)
endif ()

if (DRY_2D)
    add_subdirectory (ThirdParty/Box2D)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spine)
        add_subdirectory (ThirdParty/spine)
        # Add definition for Spine
        add_definitions (-DDRY_SPINE)
    endif ()
endif ()

if (DRY_WEBP)
    add_subdirectory (ThirdParty/WebP)
endif ()

if (DRY_PHYSICS)
    add_subdirectory (ThirdParty/Bullet)
endif ()

if (NOT ANDROID AND NOT ARM AND NOT WEB)
    if (DRY_OPENGL)
        add_subdirectory (ThirdParty/GLEW)
    elseif (NOT DRY_D3D11)
        add_subdirectory (ThirdParty/MojoShader)
    endif ()
    if (NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
        add_subdirectory (ThirdParty/LibCpuId)
    endif ()
endif ()

# Dry game engine library
add_subdirectory (Dry)

# Dry Clang tools
if (DRY_CLANG_TOOLS)
    add_subdirectory (Clang-Tools)
endif ()

# Dry tools
add_subdirectory (Tools)

# Dry samples
add_subdirectory (Samples)

# Dry extras
if (DRY_EXTRAS)
    add_subdirectory (Extras)
endif ()
