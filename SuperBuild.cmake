#-----------------------------------------------------------------------------
# Convenient macro allowing to download a file
#-----------------------------------------------------------------------------

if(NOT MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL)
  set(MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL https://www.mitk.org/download/thirdparty)
endif()

macro(downloadFile url dest)
  file(DOWNLOAD ${url} ${dest} STATUS status)
  list(GET status 0 error_code)
  list(GET status 1 error_msg)
  if(error_code)
    message(FATAL_ERROR "error: Failed to download ${url} - ${error_msg}")
  endif()
endmacro()

#-----------------------------------------------------------------------------
# MITK Prerequisites
#-----------------------------------------------------------------------------

if(UNIX AND NOT APPLE)

  include(mitkFunctionCheckPackageHeader)

  # Check for libxt-dev
  mitkFunctionCheckPackageHeader(StringDefs.h libxt-dev /usr/include/X11/)

  # Check for libtiff4-dev
  mitkFunctionCheckPackageHeader(tiff.h libtiff4-dev)

endif()

# We need a proper patch program. On Linux and MacOS, we assume
# that "patch" is available. On Windows, we download patch.exe
# if not patch program is found.
find_program(PATCH_COMMAND patch)
if((NOT PATCH_COMMAND OR NOT EXISTS ${PATCH_COMMAND}) AND WIN32)
  downloadFile(${MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL}/patch.exe
               ${CMAKE_CURRENT_BINARY_DIR}/patch.exe)
  find_program(PATCH_COMMAND patch ${CMAKE_CURRENT_BINARY_DIR})
endif()
if(NOT PATCH_COMMAND)
  message(FATAL_ERROR "No patch program found.")
endif()

#-----------------------------------------------------------------------------
# ExternalProjects
#-----------------------------------------------------------------------------

get_property(external_projects GLOBAL PROPERTY MITK_EXTERNAL_PROJECTS)

if(MITK_CTEST_SCRIPT_MODE)
  # Write a file containing the list of enabled external project targets.
  # This file can be read by a ctest script to separately build projects.
  set(SUPERBUILD_TARGETS )
  foreach(proj ${external_projects})
    if(MITK_USE_${proj})
      list(APPEND SUPERBUILD_TARGETS ${proj})
    endif()
  endforeach()
  file(WRITE "${CMAKE_BINARY_DIR}/SuperBuildTargets.cmake" "set(SUPERBUILD_TARGETS ${SUPERBUILD_TARGETS})")
endif()

# A list of "nice" external projects, playing well together with CMake
set(nice_external_projects ${external_projects})
list(REMOVE_ITEM nice_external_projects Boost)
foreach(proj ${nice_external_projects})
  if(MITK_USE_${proj})
    set(EXTERNAL_${proj}_DIR "${${proj}_DIR}" CACHE PATH "Path to ${proj} build directory")
    mark_as_advanced(EXTERNAL_${proj}_DIR)
    if(EXTERNAL_${proj}_DIR)
      set(${proj}_DIR ${EXTERNAL_${proj}_DIR})
    endif()
  endif()
endforeach()

set(EXTERNAL_Boost_ROOT "${Boost_ROOT}" CACHE PATH "Path to Boost directory")
mark_as_advanced(EXTERNAL_Boost_ROOT)
if(EXTERNAL_Boost_ROOT)
  set(Boost_ROOT ${EXTERNAL_Boost_ROOT})
endif()

if(BUILD_TESTING)
  set(EXTERNAL_MITK_DATA_DIR "${MITK_DATA_DIR}" CACHE PATH "Path to the MITK data directory")
  mark_as_advanced(EXTERNAL_MITK_DATA_DIR)
  if(EXTERNAL_MITK_DATA_DIR)
    set(MITK_DATA_DIR ${EXTERNAL_MITK_DATA_DIR})
  endif()
endif()

#-----------------------------------------------------------------------------
# External project settings
#-----------------------------------------------------------------------------

include(ExternalProject)
include(mitkMacroQueryCustomEPVars)
include(mitkFunctionInstallExternalCMakeProject)
include(mitkFunctionCleanExternalProject)

option(MITK_AUTOCLEAN_EXTERNAL_PROJECTS "Experimental: Clean external project builds if updated" ON)

set(ep_prefix "${CMAKE_BINARY_DIR}/ep")
set_property(DIRECTORY PROPERTY EP_PREFIX ${ep_prefix})

# Compute -G arg for configuring external projects with the same CMake generator:
if(CMAKE_EXTRA_GENERATOR)
  set(gen "${CMAKE_EXTRA_GENERATOR} - ${CMAKE_GENERATOR}")
else()
  set(gen "${CMAKE_GENERATOR}")
endif()

set(gen_platform ${CMAKE_GENERATOR_PLATFORM})

# Use this value where semi-colons are needed in ep_add args:
set(sep "^^")

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

  if(MSVC_VERSION VERSION_LESS 1928)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()
endif()

# This is a workaround for passing linker flags
# actually down to the linker invocation
set(_cmake_required_flags_orig ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "-Wl,-rpath")
mitkFunctionCheckCompilerFlags(${CMAKE_REQUIRED_FLAGS} _has_rpath_flag)
set(CMAKE_REQUIRED_FLAGS ${_cmake_required_flags_orig})

set(_install_rpath_linkflag )
if(_has_rpath_flag)
  if(APPLE)
    set(_install_rpath_linkflag "-Wl,-rpath,@loader_path/../lib")
  else()
    set(_install_rpath_linkflag "-Wl,-rpath='$ORIGIN/../lib")
    if(Qt6_DIR)
      set(_install_rpath_linkflag "${_install_rpath_linkflag}:${Qt6_DIR}/../..")
    endif()
    set(_install_rpath_linkflag "${_install_rpath_linkflag}'")
  endif()
endif()

set(_install_rpath)
if(APPLE)
  set(_install_rpath "@loader_path/../lib")
elseif(UNIX)
  # this work for libraries as well as executables
  set(_install_rpath "\$ORIGIN/../lib")
  if(Qt6_DIR)
    set(_install_rpath "${_install_rpath}:${Qt6_DIR}/../..")
  endif()
endif()

set(ep_common_args
  -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=OLD
  -DCMAKE_CXX_EXTENSIONS:STRING=${CMAKE_CXX_EXTENSIONS}
  -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
  -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
  -DCMAKE_MACOSX_RPATH:BOOL=TRUE
  "-DCMAKE_INSTALL_RPATH:STRING=${_install_rpath}"
  -DBUILD_TESTING:BOOL=OFF
  -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  -DBUILD_SHARED_LIBS:BOOL=ON
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
  "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${MITK_CXX${MITK_CXX_STANDARD}_FLAG}"
  #debug flags
  -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
  -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
  #release flags
  -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
  -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
  #relwithdebinfo
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
  -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
  #link flags
  -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
  -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
  -DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
)

if(MSVC)
  list(APPEND ep_common_args
    -DCMAKE_DEBUG_POSTFIX:STRING=d
  )

  set(DCMTK_CMAKE_DEBUG_POSTFIX d)

  if(MSVC_VERSION VERSION_GREATER_EQUAL 1928)
    list(APPEND ep_common_args
      "-DCMAKE_VS_GLOBALS:STRING=UseMultiToolTask=true${sep}EnforceProcessCountAcrossBuilds=true"
    )
  endif()
endif()

set(ep_common_cache_args
)

set(ep_common_cache_default_args
  "-DCMAKE_PREFIX_PATH:PATH=<INSTALL_DIR>;${CMAKE_PREFIX_PATH}"
  "-DCMAKE_INCLUDE_PATH:PATH=${CMAKE_INCLUDE_PATH}"
  "-DCMAKE_LIBRARY_PATH:PATH=${CMAKE_LIBRARY_PATH}"
)

# Pass the CMAKE_OSX variables to external projects
if(APPLE)
  set(MAC_OSX_ARCHITECTURE_ARGS
        -DCMAKE_OSX_ARCHITECTURES:PATH=${CMAKE_OSX_ARCHITECTURES}
        -DCMAKE_OSX_DEPLOYMENT_TARGET:PATH=${CMAKE_OSX_DEPLOYMENT_TARGET}
        -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
  )
  set(ep_common_args
        ${MAC_OSX_ARCHITECTURE_ARGS}
        ${ep_common_args}
  )
endif()

set(mitk_superbuild_ep_args)
set(mitk_depends )

# Include external projects
include(CMakeExternals/MITKData.cmake)
foreach(p ${external_projects})
  set(p_hash "")

  set(p_file "${CMAKE_SOURCE_DIR}/CMakeExternals/${p}.cmake")
  if(EXISTS ${p_file})
    file(MD5 ${p_file} p_hash)
  else()
    foreach(MITK_EXTENSION_DIR ${MITK_ABSOLUTE_EXTENSION_DIRS})
      set(MITK_CMAKE_EXTERNALS_EXTENSION_DIR "${MITK_EXTENSION_DIR}/CMakeExternals")
      set(p_file "${MITK_CMAKE_EXTERNALS_EXTENSION_DIR}/${p}.cmake")
      if(EXISTS "${p_file}")
        file(MD5 "${p_file}" p_hash)
        break()
      endif()
    endforeach()
  endif()

  if(p_hash)
    set(p_hash_file "${ep_prefix}/tmp/${p}-hash.txt")
    if(MITK_AUTOCLEAN_EXTERNAL_PROJECTS)
      if(EXISTS "${p_hash_file}")
        file(READ "${p_hash_file}" p_prev_hash)
        if(NOT p_hash STREQUAL p_prev_hash)
          mitkCleanExternalProject(${p})
        endif()
      endif()
    endif()
    file(WRITE "${p_hash_file}" ${p_hash})
  endif()

  include("${p_file}" OPTIONAL)

  list(APPEND mitk_superbuild_ep_args
       -DMITK_USE_${p}:BOOL=${MITK_USE_${p}}
      )
  get_property(_package GLOBAL PROPERTY MITK_${p}_PACKAGE)
  if(_package)
    list(APPEND mitk_superbuild_ep_args -D${p}_DIR:PATH=${${p}_DIR})
  endif()

  list(APPEND mitk_depends ${${p}_DEPENDS})
endforeach()
if (SWIG_EXECUTABLE)
  list(APPEND mitk_superbuild_ep_args -DSWIG_EXECUTABLE=${SWIG_EXECUTABLE})
endif()

#-----------------------------------------------------------------------------
# Set superbuild boolean args
#-----------------------------------------------------------------------------

set(mitk_cmake_boolean_args
  BUILD_SHARED_LIBS
  WITH_COVERAGE
  BUILD_TESTING
  MITK_BUILD_ALL_PLUGINS
  MITK_BUILD_ALL_APPS
  MITK_BUILD_EXAMPLES
  MITK_USE_Qt6
  MITK_USE_SYSTEM_Boost
  MITK_USE_BLUEBERRY
  MITK_USE_OpenMP
  )

#-----------------------------------------------------------------------------
# Create the final variable containing superbuild boolean args
#-----------------------------------------------------------------------------

set(mitk_superbuild_boolean_args)
foreach(mitk_cmake_arg ${mitk_cmake_boolean_args})
  list(APPEND mitk_superbuild_boolean_args -D${mitk_cmake_arg}:BOOL=${${mitk_cmake_arg}})
endforeach()

if(MITK_BUILD_ALL_PLUGINS)
  list(APPEND mitk_superbuild_boolean_args -DBLUEBERRY_BUILD_ALL_PLUGINS:BOOL=ON)
endif()

#-----------------------------------------------------------------------------
# MITK Utilities
#-----------------------------------------------------------------------------

set(proj MITK-Utilities)
ExternalProject_Add(${proj}
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    ${mitk_depends}
)
#-----------------------------------------------------------------------------
# Additional MITK CXX/C Flags
#-----------------------------------------------------------------------------

set(MITK_ADDITIONAL_C_FLAGS "" CACHE STRING "Additional C Flags for MITK")
set(MITK_ADDITIONAL_C_FLAGS_RELEASE "" CACHE STRING "Additional Release C Flags for MITK")
set(MITK_ADDITIONAL_C_FLAGS_DEBUG "" CACHE STRING "Additional Debug C Flags for MITK")
mark_as_advanced(MITK_ADDITIONAL_C_FLAGS MITK_ADDITIONAL_C_FLAGS_DEBUG MITK_ADDITIONAL_C_FLAGS_RELEASE)

set(MITK_ADDITIONAL_CXX_FLAGS "" CACHE STRING "Additional CXX Flags for MITK")
set(MITK_ADDITIONAL_CXX_FLAGS_RELEASE "" CACHE STRING "Additional Release CXX Flags for MITK")
set(MITK_ADDITIONAL_CXX_FLAGS_DEBUG "" CACHE STRING "Additional Debug CXX Flags for MITK")
mark_as_advanced(MITK_ADDITIONAL_CXX_FLAGS MITK_ADDITIONAL_CXX_FLAGS_DEBUG MITK_ADDITIONAL_CXX_FLAGS_RELEASE)

set(MITK_ADDITIONAL_EXE_LINKER_FLAGS "" CACHE STRING "Additional exe linker flags for MITK")
set(MITK_ADDITIONAL_SHARED_LINKER_FLAGS "" CACHE STRING "Additional shared linker flags for MITK")
set(MITK_ADDITIONAL_MODULE_LINKER_FLAGS "" CACHE STRING "Additional module linker flags for MITK")
mark_as_advanced(MITK_ADDITIONAL_EXE_LINKER_FLAGS MITK_ADDITIONAL_SHARED_LINKER_FLAGS MITK_ADDITIONAL_MODULE_LINKER_FLAGS)

#-----------------------------------------------------------------------------
# MITK Configure
#-----------------------------------------------------------------------------

if(MITK_INITIAL_CACHE_FILE)
  set(mitk_initial_cache_arg -C "${MITK_INITIAL_CACHE_FILE}")
endif()

set(mitk_optional_cache_args )
foreach(type RUNTIME ARCHIVE LIBRARY)
  if(DEFINED CTK_PLUGIN_${type}_OUTPUT_DIRECTORY)
    list(APPEND mitk_optional_cache_args -DCTK_PLUGIN_${type}_OUTPUT_DIRECTORY:PATH=${CTK_PLUGIN_${type}_OUTPUT_DIRECTORY})
  endif()
endforeach()

# Optional python variables
if(MITK_USE_Python3)
  list(APPEND mitk_optional_cache_args
       "-DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}"
       "-DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIRS}"
       "-DPython3_LIBRARY:FILEPATH=${Python3_LIBRARY}"
       "-DPython3_STDLIB:FILEPATH=${Python3_STDLIB}"
       "-DPython3_SITELIB:FILEPATH=${Python3_SITELIB}"
      )
endif()

if(OPENSSL_ROOT_DIR)
  list(APPEND mitk_optional_cache_args
    "-DOPENSSL_ROOT_DIR:PATH=${OPENSSL_ROOT_DIR}"
  )
endif()

if(CMAKE_FRAMEWORK_PATH)
  list(APPEND mitk_optional_cache_args
    "-DCMAKE_FRAMEWORK_PATH:PATH=${CMAKE_FRAMEWORK_PATH}"
  )
endif()

# Optional pass through of Doxygen
if(DOXYGEN_EXECUTABLE)
  list(APPEND mitk_optional_cache_args
       -DDOXYGEN_EXECUTABLE:FILEPATH=${DOXYGEN_EXECUTABLE}
  )
endif()

if(MITK_DOXYGEN_BUILD_ALWAYS)
  list(APPEND mitk_optional_cache_args
    -DMITK_DOXYGEN_BUILD_ALWAYS:BOOL=${MITK_DOXYGEN_BUILD_ALWAYS}
  )
endif()

set(proj MITK-Configure)

ExternalProject_Add(${proj}
  LIST_SEPARATOR ${sep}
  DOWNLOAD_COMMAND ""
  CMAKE_GENERATOR ${gen}
  CMAKE_GENERATOR_PLATFORM ${gen_platform}
  CMAKE_CACHE_ARGS
    # --------------- Build options ----------------
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DMITK_BUILD_CONFIGURATION:STRING=${MITK_BUILD_CONFIGURATION}
    -DMITK_XVFB_TESTING_COMMAND:STRING=${MITK_XVFB_TESTING_COMMAND}
    "-DCMAKE_CONFIGURATION_TYPES:STRING=${CMAKE_CONFIGURATION_TYPES}"
    "-DCMAKE_PREFIX_PATH:PATH=${ep_prefix};${CMAKE_PREFIX_PATH}"
    "-DCMAKE_LIBRARY_PATH:PATH=${CMAKE_LIBRARY_PATH}"
    "-DCMAKE_INCLUDE_PATH:PATH=${CMAKE_INCLUDE_PATH}"
    # --------------- Compile options ----------------
    -DCMAKE_CXX_EXTENSIONS:STRING=${CMAKE_CXX_EXTENSIONS}
    -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} ${MITK_ADDITIONAL_C_FLAGS}"
    "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${MITK_ADDITIONAL_CXX_FLAGS}"
    # debug flags
    "-DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG} ${MITK_ADDITIONAL_CXX_FLAGS_DEBUG}"
    "-DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG} ${MITK_ADDITIONAL_C_FLAGS_DEBUG}"
    # release flags
    "-DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE} ${MITK_ADDITIONAL_CXX_FLAGS_RELEASE}"
    "-DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE} ${MITK_ADDITIONAL_C_FLAGS_RELEASE}"
    # relwithdebinfo
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
    -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
    # link flags
    "-DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS} ${MITK_ADDITIONAL_EXE_LINKER_FLAGS}"
    "-DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS} ${MITK_ADDITIONAL_SHARED_LINKER_FLAGS}"
    "-DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS} ${MITK_ADDITIONAL_MODULE_LINKER_FLAGS}"
    # Output directories
    -DMITK_CMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${MITK_CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    -DMITK_CMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${MITK_CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    -DMITK_CMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${MITK_CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    # ------------- Boolean build options --------------
    ${mitk_superbuild_boolean_args}
    ${mitk_optional_cache_args}
    -DMITK_USE_SUPERBUILD:BOOL=OFF
    -DMITK_PCH:BOOL=${MITK_PCH}
    -DMITK_FAST_TESTING:BOOL=${MITK_FAST_TESTING}
    -DMITK_XVFB_TESTING:BOOL=${MITK_XVFB_TESTING}
    -DCTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
    # ----------------- Miscellaneous ---------------
    -DCMAKE_LIBRARY_PATH:PATH=${CMAKE_LIBRARY_PATH}
    -DCMAKE_INCLUDE_PATH:PATH=${CMAKE_INCLUDE_PATH}
    -DMITK_CTEST_SCRIPT_MODE:STRING=${MITK_CTEST_SCRIPT_MODE}
    -DMITK_SUPERBUILD_BINARY_DIR:PATH=${MITK_BINARY_DIR}
    -DMITK_MODULES_TO_BUILD:INTERNAL=${MITK_MODULES_TO_BUILD}
    -DMITK_WHITELIST:STRING=${MITK_WHITELIST}
    -DMITK_WHITELISTS_EXTERNAL_PATH:STRING=${MITK_WHITELISTS_EXTERNAL_PATH}
    -DMITK_WHITELISTS_INTERNAL_PATH:STRING=${MITK_WHITELISTS_INTERNAL_PATH}
    -DMITK_EXTENSION_DIRS:STRING=${MITK_EXTENSION_DIRS}
    -DMITK_ACCESSBYITK_INTEGRAL_PIXEL_TYPES:STRING=${MITK_ACCESSBYITK_INTEGRAL_PIXEL_TYPES}
    -DMITK_ACCESSBYITK_FLOATING_PIXEL_TYPES:STRING=${MITK_ACCESSBYITK_FLOATING_PIXEL_TYPES}
    -DMITK_ACCESSBYITK_COMPOSITE_PIXEL_TYPES:STRING=${MITK_ACCESSBYITK_COMPOSITE_PIXEL_TYPES}
    -DMITK_ACCESSBYITK_VECTOR_PIXEL_TYPES:STRING=${MITK_ACCESSBYITK_VECTOR_PIXEL_TYPES}
    -DMITK_ACCESSBYITK_DIMENSIONS:STRING=${MITK_ACCESSBYITK_DIMENSIONS}
    -DMITK_CUSTOM_REVISION_DESC:STRING=${MITK_CUSTOM_REVISION_DESC}
    # --------------- External project options ---------------
    -DMITK_DATA_DIR:PATH=${MITK_DATA_DIR}
    -DMITK_EXTERNAL_PROJECT_PREFIX:PATH=${ep_prefix}
    -DCppMicroServices_DIR:PATH=${CppMicroServices_DIR}
    -DDCMTK_CMAKE_DEBUG_POSTFIX:STRING=${DCMTK_CMAKE_DEBUG_POSTFIX}
    -DBoost_ROOT:PATH=${Boost_ROOT}
    -DBOOST_LIBRARYDIR:PATH=${BOOST_LIBRARYDIR}
    -DMITK_USE_Boost_LIBRARIES:STRING=${MITK_USE_Boost_LIBRARIES}
    -DQt6_DIR:PATH=${Qt6_DIR}
    -DMITK_USE_Python3:BOOL=${MITK_USE_Python3}
  CMAKE_ARGS
    ${mitk_initial_cache_arg}
    ${MAC_OSX_ARCHITECTURE_ARGS}
    ${mitk_superbuild_ep_args}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_DIR ${CMAKE_BINARY_DIR}/MITK-build
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    MITK-Utilities
  )

mitkFunctionInstallExternalCMakeProject(${proj})

#-----------------------------------------------------------------------------
# MITK
#-----------------------------------------------------------------------------

if(CMAKE_GENERATOR MATCHES ".*Makefiles.*")
  set(mitk_build_cmd "$(MAKE)")
else()
  set(mitk_build_cmd ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/MITK-build --config ${CMAKE_CFG_INTDIR})
endif()

if(NOT DEFINED SUPERBUILD_EXCLUDE_MITKBUILD_TARGET OR NOT SUPERBUILD_EXCLUDE_MITKBUILD_TARGET)
  set(MITKBUILD_TARGET_ALL_OPTION "ALL")
else()
  set(MITKBUILD_TARGET_ALL_OPTION "")
endif()

add_custom_target(MITK-build ${MITKBUILD_TARGET_ALL_OPTION}
  COMMAND ${mitk_build_cmd}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MITK-build
  DEPENDS MITK-Configure
  )

#-----------------------------------------------------------------------------
# Custom target allowing to drive the build of the MITK project itself
#-----------------------------------------------------------------------------

add_custom_target(MITK
  COMMAND ${mitk_build_cmd}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MITK-build
)

