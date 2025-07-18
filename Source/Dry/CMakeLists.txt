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
set (TARGET_NAME Dry)

setup_lint ()

# Prevent ICE from old MinGW versions, workaround from the GCC bugzilla https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61189
if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1 AND NOT DRY_64BIT)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpreferred-stack-boundary=2")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mpreferred-stack-boundary=2")
endif ()

check_include_files (stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
  add_definitions (-DHAVE_STDINT_H)
endif ()

# Generate JSON compilation database format specification
if (DRY_CLANG_TOOLS OR DRY_BINDINGS)
    set (CMAKE_EXPORT_COMPILE_COMMANDS 1)
    # Clang assumes all their tools to be installed in a relative path to their libs but this is not the case for our own Clang-tools
    # Clang has a hard-coded way to search for their headers (see http://clang.llvm.org/docs/LibTooling.html#builtin-includes)
    # In order for our Clang-tools to work correctly we need to make this Clang's assumption to be true
    execute_process (COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string (REGEX REPLACE "([.0123456789]+).*" \\1 LLVM_VERSION "${LLVM_VERSION}")      # Stringify against empty output variable
    execute_process (COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process (COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/tool/lib/clang/${LLVM_VERSION})
    create_symlink (${LLVM_LIBDIR}/clang/${LLVM_VERSION}/include ${CMAKE_BINARY_DIR}/bin/tool/lib/clang/${LLVM_VERSION}/include FALLBACK_TO_COPY)
endif ()

# Save keep the preprocessor macros (for using the Dry library) for later use in generating Dry.pc file
get_directory_property (DRY_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)

# Define preprocessor macros (for building the Dry library)
add_definitions (-DDRY_IS_BUILDING)
if (ANDROID AND DEFINED ENV{ASSET_DIR_INDICATOR})
    add_definitions (-DASSET_DIR_INDICATOR=\"$ENV{ASSET_DIR_INDICATOR}\")
endif ()
if ($ENV{COVERITY_SCAN_BRANCH})
    # This macro is used when scanning Dry library via Coverity-Scan
    # TODO: The Coverity-Scan modelling is not yet working properly (anyone interested in static analyzer is welcome to give it another try)
    add_definitions (-DCOVERITY_SCAN_MODEL)
endif ()
if (TARGET GLEW)
    # These macros are required because Dry (OpenGL) headers are exposed to GLEW headers
    add_definitions (-DGLEW_STATIC -DGLEW_NO_GLU)
endif ()
if (HAVE_SINCOSF)
    add_definitions (-DHAVE_SINCOSF)
elseif (HAVE___SINCOSF)
    add_definitions (-DHAVE___SINCOSF)
endif ()
# Enable/disable SIMD instruction set for STB image
if (NEON)
    if (XCODE)
        if (IOS)
            set (SDK iphoneos)
        elseif (TVOS)
            set (SDK appletvos)
        endif ()
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CFLAGS)")
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CPLUSPLUSFLAGS)")
    else ()
        add_definitions (-DSTBI_NEON)
    endif ()
elseif (MSVC AND NOT DRY_SSE)
    add_definitions (-DSTBI_NO_SIMD)
elseif (MINGW AND NOT DRY_64BIT AND DRY_SSE)
    add_definitions (-DSTBI_MINGW_ENABLE_SSE2)
endif ()
if (ODBC_VERSION AND NOT ODBC_VERSION VERSION_LESS 3)
    add_definitions (-DODBC_3_OR_LATER)
endif ()
if (WIN32)
    set (CMAKE_DEBUG_POSTFIX _d)
    if (NOT MINGW OR HAVE_RTL_OSVERSIONINFOW)
        add_definitions (-DHAVE_RTL_OSVERSIONINFOW)
    endif ()
endif ()
if (DRY_HASH_DEBUG)
    add_definitions (-DDRY_HASH_DEBUG)
endif ()

if (EXISTS ${CMAKE_SOURCE_DIR}/bin/shell.html)
    add_definitions (-DDRY_CUSTOM_SHELL)
endif()

# Define source files
foreach (DIR IK Navigation Network Physics 2D WebP)
    string (TOUPPER DRY_${DIR} OPT)
    if (NOT ${OPT})
        list (APPEND EXCLUDED_SOURCE_DIRS ${DIR})
    endif ()
endforeach ()
if (NOT DRY_ANGELSCRIPT)
    list (APPEND EXCLUDED_SOURCE_DIRS AngelScript)
endif ()
if (DRY_DATABASE)
    if (DRY_DATABASE_ODBC)
        list (APPEND EXCLUDED_SOURCE_DIRS Database/SQLite)
    else ()
        list (APPEND EXCLUDED_SOURCE_DIRS Database/ODBC)
    endif ()
else ()
    list (APPEND EXCLUDED_SOURCE_DIRS Database)
endif ()
if (APPLE AND NOT ARM)
    set (GLOB_OBJC_PATTERN *.m)     # Should only pick up MacFileWatcher.m for macOS platform at the moment
endif ()
string (REPLACE ";" "/[^;]+;" EXCLUDE_PATTERNS "${EXCLUDED_SOURCE_DIRS};")
define_source_files (EXCLUDE_PATTERNS ${EXCLUDE_PATTERNS} GLOB_CPP_PATTERNS *.cpp ${GLOB_OBJC_PATTERN} RECURSE GROUP PCH Precompiled.h)

# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P CMake/Modules/GetDryRevision.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()
set (DRY_DEPS ${STATIC_LIBRARY_TARGETS})
set_source_files_properties (${SYMBOLIC_SOURCES} PROPERTIES SYMBOLIC TRUE)
add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/librevision.h
    COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new -P CMake/Modules/GetDryRevision.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new ${CMAKE_CURRENT_BINARY_DIR}/librevision.h
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/librevision.h.new
    DEPENDS ${DRY_DEPS} ${CMAKE_SOURCE_DIR}/CMake/Modules/GetDryRevision.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "")     # Suppress the comment as the header file may or may not be re-generated
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)

# Use the AutoBinder tool to generate source files for binding script subsystems
if (DRY_BINDINGS)
    # Build the Clang-tools as external project (even when we are not cross-compiling because it needs C++11 standard on) for auto-binding generation
    include (ExternalProject)
    if (IOS OR TVOS)
        # When cross-compiling for iOS/tvOS the host environment has been altered by xcodebuild for the said platform, the following fix is required to reset the host environment before spawning another process to configure/generate project file for external project
        # Also workaround a known CMake/Xcode generator bug which prevents it from installing native tool binaries correctly
        set (ALTERNATE_COMMAND CMAKE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
    else ()
        set (ALTERNATE_COMMAND CMAKE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
    endif ()
    ExternalProject_Add (AutoBinder
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/Source/Clang-Tools
        CMAKE_ARGS -DDRY_CLANG_TOOLS=AutoBinder -DDEST_RUNTIME_DIR=${CMAKE_BINARY_DIR}/bin/tool/clang -DDEST_INCLUDE_DIR=${DEST_INCLUDE_DIR} -DBAKED_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        ${ALTERNATE_COMMAND})
    add_make_clean_files (${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder)
    file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/generated)
    foreach (SCRIPT AngelScript JavaScript)
        string (TOUPPER DRY_${SCRIPT} OPT)
        if (${OPT})
            list (APPEND SCRIPT_SUBSYSTEMS -s ${SCRIPT})
            list (APPEND GEN_CPP_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${SCRIPT}API.cpp)
        endif ()
    endforeach ()
    string (REGEX REPLACE "[^;]+API\\.cpp" "" ANNOTATED_SOURCES "${CPP_FILES}")    # Stringify to preserve the semicolons
    add_custom_command (OUTPUT ${GEN_CPP_FILES}
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_BINARY_DIR}/bin/tool/clang/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/generated ${SCRIPT_SUBSYSTEMS} ${ANNOTATED_SOURCES}
        COMMAND ${CMAKE_COMMAND} -E touch ${GEN_CPP_FILES}   # TODO: fake the output - WIP
        DEPENDS AutoBinder ${H_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating source files for binding script subsystems")
endif ()

# Generate platform specific export header file
if (MSVC)
    set (PRE_EXPORT_HEADER "\n#pragma warning(disable: 4251)\n#pragma warning(disable: 4275)\n\n#if _MSC_VER < 1900\n#define strtoll _strtoi64\n#define strtoull _strtoui64\n#endif\n\n")
endif ()
if (DRY_CLANG_TOOLS)
    set (ANNOTATE_NONSCRIPTABLE "__attribute__((annotate(\"nonscriptable\")))")
endif ()
generate_export_header (${TARGET_NAME} ${DRY_LIB_TYPE} EXPORT_MACRO_NAME DRY_API EXPORT_FILE_NAME Dry.h.new)
execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/Dry.h.new ${CMAKE_CURRENT_BINARY_DIR}/Dry.h)
file (REMOVE ${CMAKE_CURRENT_BINARY_DIR}/Dry.h.new)
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/Dry.h)

# Define generated object files
# This is a hack as it relies on internal working of CMake
if(CMAKE_MAJOR_VERSION VERSION_LESS 4)
    if (MSVC AND DRY_LIB_TYPE STREQUAL SHARED)   # MSVC linker does not have force_load/whole_archive equivalent
        foreach (TARGET ${STATIC_LIBRARY_TARGETS})
            get_target_property (SOURCES ${TARGET} SOURCES)
            if(CMAKE_MAJOR_VERSION VERSION_LESS 4)
                get_target_property (INT_DIR ${TARGET} LOCATION)
            else ()
                set (INT_DIR $<TARGET_FILE:${TARGET}>)
            endif ()
            if (INT_DIR)
                get_filename_component (INT_DIR ${INT_DIR} PATH)
                if (CMAKE_GENERATOR MATCHES 2008)
                    string (REPLACE /$(OutDir) /${TARGET}.dir/$(ConfigurationName) INT_DIR ${INT_DIR})
                elseif (VS)
                    string (REPLACE /$(Configuration) /${TARGET}.dir/$(ConfigurationName) INT_DIR ${INT_DIR})
                endif ()
                foreach (SOURCE ${SOURCES})
                    get_filename_component (NAME ${SOURCE} NAME)
                    if (NAME MATCHES \\.c.*$|\\.mm?$|\\.S$|\\.s$)
                        if (VS)
                            string (REGEX REPLACE \\.c.*$|\\.mm?$|\\.S$|\\.s$ "" NAME "${NAME}")
                            list (APPEND OBJ_FILES ${INT_DIR}/${NAME}.obj)
                        else ()
                            string (REGEX REPLACE ^.*/ThirdParty/${TARGET}/ "" SOURCE "${SOURCE}")
                            list (APPEND OBJ_FILES ${INT_DIR}/CMakeFiles/${TARGET}.dir/${SOURCE}.obj)
                        endif ()
                    elseif (NAME MATCHES \\.o.*$)
                        list (APPEND OBJ_FILES ${SOURCE})
                    endif ()
                endforeach ()
                source_group ("Object Files\\${TARGET}" FILES ${OBJ_FILES})
                list (APPEND ALL_OBJ_FILES ${OBJ_FILES})
                unset (OBJ_FILES)
            endif ()
        endforeach ()
        set_source_files_properties (${ALL_OBJ_FILES} PROPERTIES GENERATED TRUE)
    endif ()

# Aggregate all source files
    list (APPEND SOURCE_FILES ${GEN_CPP_FILES} ${ALL_OBJ_FILES} librevision.h Dry.h ${SYMBOLIC_SOURCES})
else ()
    list (APPEND SOURCE_FILES ${GEN_CPP_FILES} librevision.h Dry.h ${SYMBOLIC_SOURCES})
endif ()

# Define dependency libs
# Add include directories to find the precompiled header, export header, and installed headers from third-party libs
set (INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${THIRD_PARTY_INCLUDE_DIR})
if (DRY_PHYSICS)
    # Bullet library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/Bullet)
endif ()
if (DRY_NAVIGATION)
    # DetourTileCache and DetourCrowd libraries depend on Detour's include dir to be added in the header search path
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/Detour)
endif ()
# Workaround for GCC 5.4 and above when building a SHARED lib type for Linux platform to fix the undefined symbol "__cpu_model" issue (see #1519)
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.3.1 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND DRY_LIB_TYPE STREQUAL SHARED)  # 5.3.1 was the last known good version
    list (APPEND LIBS gcc)
endif ()

# Setup library output path
if (ANDROID)
    # Let SDK's android.toolchain.cmake controls where are the output directories
else ()
    set_output_directories (${CMAKE_BINARY_DIR}/lib ARCHIVE LIBRARY)
endif ()

# Setup target
if (IOS OR TVOS)
    set (EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
endif ()

setup_library (${DRY_LIB_TYPE} ${EXCLUDE_FROM_ALL})

if (NOT ANDROID AND NOT WEB)
    file (READ .soversion SOVERSION)
    string (STRIP ${SOVERSION} SOVERSION)
    string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED ${SOVERSION})
    if (MATCHED)
        set_target_properties (${TARGET_NAME} PROPERTIES VERSION ${MATCHED} SOVERSION ${CMAKE_MATCH_1})
    else ()
        message (FATAL_ERROR "The .soversion file is corrupted. It should contain a version number with this format major(0xFFFF).minor(0xFF).patch-level(0xFF). e.g.: 0.1.2")
    endif ()
else ()
    set_target_properties (${TARGET_NAME} PROPERTIES NO_SONAME TRUE)
endif ()
if (IOS)
    if (NOT IPHONEOS_DEPLOYMENT_TARGET STREQUAL "" AND IPHONEOS_DEPLOYMENT_TARGET VERSION_LESS 11.0)
        # Add a custom target to build Mach-O universal binary consisting of both (32-bit & 64-bit) iPhoneOS and iPhoneSimulator archs
        add_custom_target (${TARGET_NAME}_universal ALL
            COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]\; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'i386.*armv7|armv7.*i386|x86_64.*arm64|arm64.*x86_64'\; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both iphoneos and iphonesimulator archs\; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |egrep -cq 'i386|x86_64'\; then sdk=iphoneos\; else sdk=iphonesimulator\; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.new} && lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.new} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.new}\; fi\; fi
            DEPENDS ${TARGET_NAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating Mach-O universal binary library consisting of both iPhoneOS and iPhoneSimulator archs")
    else ()
        # Since version 11.x iOS does not support 32-bit archs anymore
        add_custom_target (${TARGET_NAME}_universal ALL
            COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]\; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both iphoneos and iphonesimulator archs\; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |egrep -cq x86_64\; then sdk=iphoneos\; else sdk=iphonesimulator\; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.new} && lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.new} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.new}\; fi\; fi
            DEPENDS ${TARGET_NAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating Mach-O universal binary library consisting of both iPhoneOS and iPhoneSimulator archs")
    endif ()
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
elseif (TVOS)
    # Add a custom target to build Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator archs
    add_custom_target (${TARGET_NAME}_universal ALL
        COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]\; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator archs\; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |grep -cq x86_64\; then sdk=appletvos\; else sdk=appletvsimulator\; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.new} && lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.new} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.new}\; fi\; fi
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Mach-O universal binary library consisting of both AppleTVOS and AppleTVSimulator archs")
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
    # TODO: Temporary workaround as CMake does not recognize AppleTV platform yet
    add_dependencies (${TARGET_NAME} APPLETV_POST_CMAKE_FIX)
elseif (EMSCRIPTEN)
    install (TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
else ()
    install (TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${DEST_RUNTIME_DIR} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
endif ()
if (WIN32 AND DRY_LIB_TYPE STREQUAL SHARED)
    set (DRY_DLL_INSTALLED TRUE)
endif ()

# Setup dependency frameworks and libraries
if (MINGW)
    set (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    if(CMAKE_MAJOR_VERSION NOT VERSION_LESS 4 AND URHO3D_LIB_TYPE STREQUAL SHARED)
         set_target_properties (${TARGET_NAME} PROPERTIES PREFIX "")
    endif ()
endif ()
if(CMAKE_MAJOR_VERSION VERSION_LESS 4)
    if (APPLE)
        set (FORCE_LOAD -force_load)
    elseif (NOT MSVC)
        set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES -Wl,--whole-archive)
        if (WIN32 AND DRY_LIB_TYPE STREQUAL SHARED)
            set_target_properties (${TARGET_NAME} PROPERTIES PREFIX "")
        endif ()
    endif ()
else ()
# Setup dependency libraries to be linked or archived as a whole inside Urho3D library for SHARED or STATIC lib type, respectively
    if (APPLE)
        set (WHOLE_ARCHIVE -force_load)
    elseif (MSVC)
        set (WHOLE_ARCHIVE /WHOLEARCHIVE)   # Require VS 2015 Update 2 onward
    else ()
        set (WHOLE_ARCHIVE -Wl,--whole-archive)
    endif ()
    set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES ${WHOLE_ARCHIVE})
endif ()
foreach (TARGET ${STATIC_LIBRARY_TARGETS})
    if(CMAKE_MAJOR_VERSION VERSION_LESS 4)
        get_target_property (ARCHIVE ${TARGET} LOCATION)
        if (ARCHIVE)
            if (NOT MSVC)
                set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES ${FORCE_LOAD} ${ARCHIVE})    # Only works in Apple (Xcode and Makefile) and GCC while building shared library
            endif ()
            if (MSVC_VERSION VERSION_EQUAL 1500)
                if (ARCHIVE MATCHES " ")
                    message (FATAL_ERROR "CMake/VS2008 generator does not support spaces in the source and/or build tree path")
                endif ()
                set_property (TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS " ${ARCHIVE}")  # Fallback for VS2008
            elseif (XCODE OR MSVC)
                set_property (TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS " \"${ARCHIVE}\"")  # Only works in Apple (Xcode only) and MSVC while building static library
            else ()
                list (APPEND ARCHIVES ${ARCHIVE})
            endif ()
        endif ()
    else ()
        set (ARCHIVE $<TARGET_FILE:${TARGET}>)
        if (NOT ${TARGET} MATCHES boost|rapidjson|STB)    # Exlucde header-only "fake" static lib targets
            set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES ${ARCHIVE})    # Should work for all platforms while building shared library
            if (URHO3D_LIB_TYPE STREQUAL STATIC)
                if (XCODE OR MSVC)  # Only works in Xcode and MSVC while building static library
                    set_property (TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS " \"${ARCHIVE}\"")
                else ()
                    list (APPEND ARCHIVES ${ARCHIVE})   # Accumulate the target locations for archiving later
                endif ()
            endif ()
        endif ()
    endif ()
endforeach ()
if (NOT XCODE AND NOT MSVC)
    if (NOT APPLE)
        set_property (TARGET ${TARGET_NAME} APPEND PROPERTY LINK_LIBRARIES -Wl,--no-whole-archive)
    endif ()
    if (DRY_LIB_TYPE STREQUAL STATIC)
        if (APPLE)
            # Apple libtool takes archives directly as input
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND mv $<TARGET_FILE:${TARGET_NAME}>{,.engine}
                COMMAND libtool -static $<TARGET_FILE:${TARGET_NAME}>.engine ${ARCHIVES} -o $<TARGET_FILE:${TARGET_NAME}>
                COMMAND rm $<TARGET_FILE:${TARGET_NAME}>.engine
                COMMENT "Merging all archives into a single static library using libtool")
        elseif (EMSCRIPTEN OR CMAKE_BINARY_DIR MATCHES "[ +*;,]")
            # Do it the hard way by first extracting the object (bitcode) files and appending them to final archive:
            # a) For Emscripten build as Emscripten emar (llvm-ar) neither takes archives directly as input nor supports MRI-style script
            # b) When the build tree path contains special characters for MRI scripts (space, '+', '*', ';', ',') as escaping is not supported
            get_filename_component (AR ${CMAKE_AR} NAME_WE)
            if (CMAKE_HOST_WIN32)
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives
                    COMMAND ${CMAKE_COMMAND} -E echo "Merging all archives into a single static library using ${AR}")
                foreach (ARCHIVE ${ARCHIVES})
                    # Merge per library because Window cmd shell does not handle long argument list
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory archives)
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_AR} x ${ARCHIVE}
                        COMMAND ${CMAKE_COMMAND} -E echo_append "\\\"${CMAKE_AR}\\\"" q \\\"$<TARGET_FILE:${TARGET_NAME}>\\\" " " >merge.bat
                        COMMAND for /F %%i in ('dir /B *.o*') do ${CMAKE_COMMAND} -E echo_append %%i" " >>merge.bat
                        COMMAND merge.bat WORKING_DIRECTORY archives)
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove_directory archives)
                endforeach ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:${TARGET_NAME}>)
            else ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives
                    COMMAND ${CMAKE_COMMAND} -E make_directory archives COMMENT "Merging all archives into a single static library using ${AR}")
                foreach (ARCHIVE ${ARCHIVES})
                    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_AR} x ${ARCHIVE} WORKING_DIRECTORY archives)
                endforeach ()
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_AR} q $<TARGET_FILE:${TARGET_NAME}> `ls -1` WORKING_DIRECTORY archives)
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:${TARGET_NAME}>
                    COMMAND ${CMAKE_COMMAND} -E remove_directory archives)
            endif ()
        else ()
            # GCC ar does not take archives directly as input like Apple libtool, however, it can be scripted to do so
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.engine
                COMMAND echo CREATE $<TARGET_FILE:${TARGET_NAME}> >script.ar
                COMMAND echo ADDLIB $<TARGET_FILE:${TARGET_NAME}>.engine >>script.ar)
            foreach (ARCHIVE ${ARCHIVES})
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND echo ADDLIB ${ARCHIVE} >>script.ar)
            endforeach ()
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND echo SAVE >>script.ar
                COMMAND echo END >>script.ar
                COMMAND ${CMAKE_AR} -M <script.ar
                COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${TARGET_NAME}>.engine script.ar
                COMMENT "Merging all archives into a single static library using ar")
        endif ()
    endif ()
endif ()
# Use PIC on platforms that support it (shared library type has this property set to true by default, so we only have to deal with those static ones that the shared library links against)
if (NOT DRY_LIB_TYPE STREQUAL STATIC)
    set_target_properties (${STATIC_LIBRARY_TARGETS} PROPERTIES POSITION_INDEPENDENT_CODE true)
endif ()

# Install headers for using the Dry library
install_header_files (FILES ${CMAKE_CURRENT_BINARY_DIR}/Dry.h DESTINATION ${DEST_INCLUDE_DIR} ACCUMULATE GENERATED_HEADERS)
install_header_files (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h ACCUMULATE URHO_HEADERS)    # Note: the trailing slash is significant
# Generate the include-all-headers header file even though we do not encourage Dry library users to use it
list (SORT URHO_HEADERS)
list (REVERSE URHO_HEADERS)
set (OPTIONAL_SUBS AngelScript Database IK Navigation Network Physics 2D)
foreach (SUB ${OPTIONAL_SUBS})
    if (URHO_HEADERS MATCHES "(include/Dry/${SUB}[^;]+)")
        list (FIND URHO_HEADERS ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT URHO_HEADERS ${FOUND_INDEX} "#endif")
    endif ()
endforeach ()
list (REVERSE URHO_HEADERS)
foreach (SUB ${OPTIONAL_SUBS})
    if (URHO_HEADERS MATCHES "(include/Dry/${SUB}[^;]+)")
        string (TOUPPER ${SUB} SUB)
        list (FIND URHO_HEADERS ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT URHO_HEADERS ${FOUND_INDEX} "#if DRY_${SUB}")
    endif ()
endforeach ()
string (REGEX REPLACE "include/[^;]+(DebugNew|GraphicsImpl|IKConverters|ODBC|OpenGL|Precompiled|SQLite|Dry|librevision)[^;]+;" "" URHO_HEADERS "${URHO_HEADERS};")
string (REGEX REPLACE "include/([^;]+)" "#include <\\1>" URHO_HEADERS "${GENERATED_HEADERS};;${URHO_HEADERS}")
string (REPLACE ";" \n URHO_HEADERS "${URHO_HEADERS}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/DryAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/DryAll.h)
install_header_files (FILES ${CMAKE_CURRENT_BINARY_DIR}/DryAll.h ${CMAKE_CURRENT_BINARY_DIR}/librevision.h DESTINATION ${DEST_INCLUDE_DIR})

# Generate platform specific pkg-config file for the benefit of Dry library users via SDK without CMake
if (ABSOLUTE_PATH_LIBS)
    string (REPLACE ";" "\" \"" DRY_ABS_PATH_LIBS "\"${ABSOLUTE_PATH_LIBS}\"")   # Note: need to always "stringify" a variable in list context for replace to work correctly, besides the list could be empty
    string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" DRY_ABS_PATH_LIBS "${DRY_ABS_PATH_LIBS}")
endif ()
if (MSVC)
    set (DASH /)
    set (LIB_DIR "/LIBPATH:\"\${libdir}\"")
    string (REPLACE ";" ".lib\" \"" DRY_LIBS "\"@LIB_NAME@\" \"${LIBS}.lib\"")
    set (PC_SUFFIX .msvc)
else ()
    set (LIB_NAME ${TARGET_NAME})
    if (MINGW)
        if (CMAKE_BUILD_TYPE STREQUAL Debug)
            set (LIB_NAME ${LIB_NAME}_d)
        endif ()
        if (DRY_LIB_TYPE STREQUAL SHARED)
            set (LIB_NAME ${LIB_NAME}.dll)
        endif ()
    endif ()
    set (DASH -)
    set (LIB_DIR "-L\"\${libdir}\"")
    string (REPLACE ";" " -l" DRY_LIBS "-l${LIB_NAME};${LIBS}")
endif ()
string (REPLACE ";" " ${DASH}D" DRY_COMPILE_DEFINITIONS "${DASH}D${DRY_COMPILE_DEFINITIONS}")
get_directory_property (GLOBAL_INCLUDE_DIRS DIRECTORY ${CMAKE_SOURCE_DIR} INCLUDE_DIRECTORIES)
if (GLOBAL_INCLUDE_DIRS)
    string (REPLACE ";" "\" ${DASH}I\"" GLOBAL_INCLUDE_DIRS "${DASH}I\"${GLOBAL_INCLUDE_DIRS}\"")
    string (REPLACE "${SYSROOT}" "" GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS})
endif ()
set (ENGINE_INCLUDE_DIRS "${DASH}I\"\${includedir}\" ${DASH}I\"\${includedir}/Dry/ThirdParty\"")
if (DRY_PHYSICS)
    # Bullet library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    set (ENGINE_INCLUDE_DIRS "${ENGINE_INCLUDE_DIRS} ${DASH}I\"\${includedir}/Dry/ThirdParty/Bullet\"")
endif ()
string (REGEX REPLACE " -include \"[^\"]+\"" "" CLEANED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_CXX_FLAGS "${CLEANED_CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Dry.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Dry.pc${PC_SUFFIX} @ONLY)
if (MSVC)
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -DLIB_NAME=$<TARGET_LINKER_FILE_NAME:${TARGET_NAME}> -P ${CMAKE_SOURCE_DIR}/CMake/Modules/AdjustPkgConfigForMSVC.cmake)
endif ()
if (ANDROID)
    # After migration to Gradle build system we do not support Android cross-compiling build via pkgconfig anymore, however, this file is still being provided as reference only
    set (RENAME RENAME Dry-${ANDROID_ABI}.pc)
endif ()
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Dry.pc DESTINATION ${DEST_PKGCONFIG_DIR} ${RENAME})

# Define post build steps
if (ANDROID AND GRADLE_BUILD_DIR)
    # Copy the library to DRY_HOME directory
    set (ABI_LIB_DIR ${GRADLE_BUILD_DIR}/tree/${CMAKE_BUILD_TYPE}/${ANDROID_ABI}/lib)
    file (MAKE_DIRECTORY ${ABI_LIB_DIR})
    # Workaround execute_process does not support generator expression
    if (DRY_LIB_TYPE STREQUAL STATIC)
        set (LIB_EXT .a)
    else ()
        set (LIB_EXT .so)
    endif ()
    # Put a placeholder during Gradle configuration phase so that initial IDE "gradle sync" works cleanly
    execute_process (COMMAND ${CMAKE_COMMAND} -E touch ${ABI_LIB_DIR}/lib${TARGET_NAME}${LIB_EXT})
    # Replace the placeholder with the actual target file during Gradle execution phase
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${TARGET_NAME}> ${ABI_LIB_DIR}
        COMMENT "Copying Dry library to DRY_HOME directory")
    add_make_clean_files (${ABI_LIB_DIR}/$<TARGET_FILE_NAME:${TARGET_NAME}>)
endif ()
if (DRY_LIB_TYPE STREQUAL SHARED AND ARM)
    # Strip the output shared library for embedded ARM platforms (we don't do this for Android anymore, assuming Android plugin will take care of it)
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>
        COMMENT "Stripping Dry shared library")
endif ()
if (DRY_SCP_TO_TARGET)
    # Ensure SCP is the last command
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND scp $<TARGET_FILE:${TARGET_NAME}> ${DRY_SCP_TO_TARGET} || exit 0
        COMMENT "Scp-ing Dry library to target system")
endif ()
