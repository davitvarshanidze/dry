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

# Find Generic Buffer Management development library
#
#  GBM_FOUND
#  GBM_INCLUDE_DIRS
#  GBM_LIBRARIES
#

find_path (GBM_INCLUDE_DIRS NAMES gbm.h DOC "GenericBufferManagement include directory")
find_library (GBM_LIBRARIES NAMES gbm DOC "GenericBufferManagement library")

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GenericBufferManagement REQUIRED_VARS GBM_LIBRARIES GBM_INCLUDE_DIRS FAIL_MESSAGE "Could NOT find Direct Generic Buffer Management development library")

mark_as_advanced (GBM_INCLUDE_DIRS GBM_LIBRARIES)
