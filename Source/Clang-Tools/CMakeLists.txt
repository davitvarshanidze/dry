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

if (CMAKE_PROJECT_NAME STREQUAL Dry)
    # Set project name
    project (Dry-Clang-Tools)

    # All Clang-tools must be natively built and output to bin/tool/clang subdir to differentiate them from the rest
    set_output_directories (${CMAKE_BINARY_DIR}/bin/tool/clang RUNTIME PDB)
else ()
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
    project (Dry-ExternalProject-${DRY_CLANG_TOOLS})

    # Set CMake modules search path
    set (CMAKE_MODULE_PATH ${BAKED_CMAKE_SOURCE_DIR}/CMake/Modules)

    # Include DryCommon.cmake module after setting project name
    include (DryCommon)

    # Setup SDK-like include dir in the build tree for building the Clang-tools
    file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)
endif ()

# LLVM/Clang is assumed to be installed in a system-wide location when not explicitly defined using env-var
if (DEFINED ENV{LLVM_CLANG_ROOT})
    set (CMAKE_SYSROOT $ENV{LLVM_CLANG_ROOT})
endif ()
execute_process (COMMAND ${LLVM_CONFIG} --bindir OUTPUT_VARIABLE LLVM_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

# No exception and no RTTI
if (MSVC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c- /GR-")
else ()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif ()

# Define common dependency libs
set (LIBS clangTooling clangFrontend clangDriver clangParse clangSerialization clangSema clangEdit clangAnalysis clangToolingCore
        clangRewrite clangLex clangASTMatchers clangAST clangBasic
        LLVMBitReader LLVMMC LLVMMCParser LLVMOption LLVMSupport)
execute_process (COMMAND ${LLVM_CONFIG} --system-libs OUTPUT_VARIABLE LLVM_SYSLIBS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
string (REGEX REPLACE " *-l" ";" LLVM_SYSLIBS "${LLVM_SYSLIBS}")   # Stringify against empty output variable
list (APPEND LIBS ${LLVM_SYSLIBS})
set (INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)

# Clang-tools can be built in two ways: on the fly in normal build one at a time or build all of them in a special Clang-tools build tree (for development)
if (CMAKE_PROJECT_NAME MATCHES ExternalProject)
    # Externally build the Clang-tool for actual use in a normal build
    add_subdirectory (${DRY_CLANG_TOOLS})
else ()
    # Define source files for the tools
    get_target_property (SOURCES Dry SOURCES)
    string (REGEX REPLACE "[^;]+\\.h" "" SOURCES "${SOURCES}")   # Stringify to preserve the semicolons
    string (REGEX REPLACE "[^;]+generated[^;]+\\.cpp" "" SOURCES "${SOURCES}")
    file (GLOB BINDING_SOURCES RELATIVE ${CMAKE_SOURCE_DIR}/Source/Dry ${CMAKE_SOURCE_DIR}/Source/Dry/Script/*API.cpp)
    string (REGEX REPLACE "[^;]+API\\.cpp" "" ANNOTATED_SOURCES "${SOURCES}")

    # List of tools
    add_subdirectory (Annotator)
    add_subdirectory (AutoBinder)

    # List of targets
    if (EXISTS ${LLVM_BINDIR}/clang-query)  # This tool is from clang-tools-extra repository which user may have not installed
        add_custom_target (ast-query
            COMMAND ${CMAKE_COMMAND} -E echo "Building AST for query, please be patient..."
            COMMAND ${LLVM_BINDIR}/clang-query -p ${CMAKE_BINARY_DIR} $$option ${SOURCES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
            COMMENT "Executing clang-query on Dry library source files")
    endif ()
    add_custom_target (ast
        COMMAND ${CMAKE_COMMAND} -E echo "Usage: option=-help make ast"
        COMMAND ${LLVM_BINDIR}/clang-check -p ${CMAKE_BINARY_DIR} $$option ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Executing clang-check on Dry library source files")
    add_custom_target (binding-ast
        COMMAND ${CMAKE_COMMAND} -E echo "Usage: option=-help make binding-ast"
        COMMAND ${LLVM_BINDIR}/clang-check -p ${CMAKE_BINARY_DIR} $$option ${BINDING_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Executing clang-check on (existing) AngelScript API bindings source files")
    add_custom_target (annotate
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/Annotator -p ${CMAKE_BINARY_DIR} ${SOURCES}
        DEPENDS Annotator
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Annotating Dry library source files")
    add_custom_target (autobinder
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Source/Dry/generated
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_BINARY_DIR}/Source/Dry/generated -s AngelScript -s LuaScript -s JavaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Auto-binding for all script subsystems")
    add_custom_target (autobinder-angelscript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Source/Dry/generated
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_BINARY_DIR}/Source/Dry/generated -s AngelScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Auto-binding for AngelScript")
    add_custom_target (autobinder-luascript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Source/Dry/generated
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_BINARY_DIR}/Source/Dry/generated -s LuaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Auto-binding for LuaScript")
    add_custom_target (autobinder-javascript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Source/Dry/generated
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_BINARY_DIR}/Source/Dry/generated -s JavaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Dry
        COMMENT "Auto-binding for JavaScript")
endif ()
