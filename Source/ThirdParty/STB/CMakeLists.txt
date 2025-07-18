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

# Define target name
set (TARGET_NAME STB)

# Setup target
add_custom_target (${TARGET_NAME} ALL)   # Dummy target just so that its post-build step can install headers to the build tree
set (STATIC_LIBRARY_TARGETS ${STATIC_LIBRARY_TARGETS} ${TARGET_NAME} PARENT_SCOPE)

# Install headers for building the Dry library
install_header_files (DIRECTORY ./ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/STB FILES_MATCHING PATTERN *.h USE_FILE_SYMLINK BUILD_TREE_ONLY)  # Note: the trailing slash is significant
