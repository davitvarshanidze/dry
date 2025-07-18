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

# Define target name
set (TARGET_NAME LibCpuId)

# Define preprocessor macros
add_definitions (-DVERSION="0.4.0+")
if (HAVE_STDINT_H)
    add_definitions (-DHAVE_STDINT_H)
endif ()

# Define generated source files
if (MSVC AND DRY_64BIT)
    enable_language (ASM_MASM)
    # Commented out due to cmake bug
    #set (ASM_FILES src/masm-x64.asm)
    # As a temporary workaround
    add_custom_command (OUTPUT masm-x64.obj COMMAND ${CMAKE_ASM_MASM_COMPILER} -Fo ${CMAKE_CURRENT_BINARY_DIR}/masm-x64.obj -c ${CMAKE_CURRENT_SOURCE_DIR}/src/masm-x64.asm DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/masm-x64.asm COMMENT "Temporary workaround for MASM")
    set (ASM_FILES ${CMAKE_CURRENT_BINARY_DIR}/masm-x64.obj ${CMAKE_CURRENT_SOURCE_DIR}/src/masm-x64.asm)
endif ()

# Define source files
define_source_files (GLOB_CPP_PATTERNS src/*.c GLOB_H_PATTERNS src/*.h)
list (APPEND SOURCE_FILES ${ASM_FILES})

# Define dependency libs
set (INCLUDE_DIRS src)

# Setup target
setup_library ()

# Install headers for building the Dry library
install_header_files (DIRECTORY src/ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/LibCpuId FILES_MATCHING PATTERN *.h BUILD_TREE_ONLY)  # Note: the trailing slash is significant
