cmake_minimum_required (VERSION 2.8.12)

project("NanoGUI")

#if(POLICY CMP0048)
#  cmake_policy(SET CMP0048 NEW)
#endif()

if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/src")
  message(FATAL_ERROR "The NanoGUI dependency repositories (GLFW, etc.) are missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by calling \"git submodule update --init --recursive\"")
endif()

if (WIN32)
  set(NANOGUI_USE_GLAD_DEFAULT ON)
else()
  set(NANOGUI_USE_GLAD_DEFAULT OFF)
endif()

option(NANOGUI_BUILD_EXAMPLE "Build NanoGUI example application?" OFF)
option(NANOGUI_BUILD_SHARED  "Build NanoGUI as a shared library?" OFF)
option(NANOGUI_BUILD_PYTHON  "Build a Python plugin for NanoGUI?" OFF)
option(NANOGUI_USE_GLAD      "Build a Python plugin for NanoGUI?" ${NANOGUI_USE_GLAD_DEFAULT})
option(NANOGUI_INSTALL       "Install NanoGUI on `make install`?" ON)

set(NANOGUI_PYTHON_VERSION "" CACHE STRING "Python version to use for compiling the Python plugin")

# Required libraries, flags, and include files for compiling and linking against nanogui (all targets)
set(NANOGUI_EXTRA_LIBS "")
set(NANOGUI_EXTRA_DEFS "")
set(NANOGUI_EXTRA_INCS "")

# Platform-dependent files for libnanogui
set(LIBNANOGUI_EXTRA_SOURCE "")
set(LIBNANOGUI_PYTHON_EXTRA_SOURCE "")

if(APPLE AND NANOGUI_BUILD_SHARED)
  set(CMAKE_MACOSX_RPATH ON)
endif()

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

macro(CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs("int main(int argc, char **argv) { return 0; }" ${_RESULT})
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "")
endmacro()

# Compile GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
set(GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)
set(BUILD_SHARED_LIBS ${NANOGUI_BUILD_SHARED} CACHE BOOL " " FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw" "ext_build/glfw")
# Two targets have now been defined: `glfw_objects`, which will be merged into
# NanoGUI at the end, and `glfw`.  The `glfw` target is the library itself
# (e.g., libglfw.so), but can be skipped as we do not need to link against it
# (because we merge `glfw_objects` into NanoGUI).  Skipping is required for
# XCode, but preferable for all build systems (reduces build artifacts).
set_target_properties(glfw PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# Compile Cereal
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/cereal" "ext_build/cereal")

# Python support: add NANOGUI_PYTHON flag to all targets
if (NANOGUI_BUILD_PYTHON)
  list(APPEND NANOGUI_EXTRA_DEFS -DNANOGUI_PYTHON)
endif()

# Shared library mode: add dllimport/dllexport flags to all symbols
if (NANOGUI_BUILD_SHARED)
  list(APPEND NANOGUI_EXTRA_DEFS -DNANOGUI_SHARED -DNVG_SHARED -DGLAD_GLAPI_EXPORT)
endif()

if (MSVC)
  # Disable annoying MSVC warnings (all targets)
  add_definitions(/D "_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

    # Disable Eigen vectorization for Windows 32 bit builds (issues with unaligned access segfaults)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNANOGUI_EIGEN_DONT_ALIGN /DEIGEN_DONT_ALIGN")
  endif()
endif()

# Compile with compiler warnings turned on
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|Intel)$")
  CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" HAS_CPP11_FLAG)

  if (HAS_CPP14_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  elseif (HAS_CPP11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    message(FATAL_ERROR "Unsupported compiler -- pybind11 requires C++11 support!")
  endif()
endif()

# Various optimizations for shared library release builds
if (NANOGUI_BUILD_SHARED)
  if (U_CMAKE_BUILD_TYPE MATCHES REL AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    # Set the default symbol visibility to hidden
    if (NOT CMAKE_CXX_FLAGS MATCHES "-fvisibility")
      set(CMAKE_CXX_FLAGS "-fvisibility=hidden ${CMAKE_CXX_FLAGS}")
    endif()

    # Enable link time optimization
    if (NOT CMAKE_CXX_FLAGS MATCHES "-flto")
      if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(LTO_CXX_FLAGS "-flto=thin")
        set(LTO_LINKER_FLAGS "-flto=thin")
        if (NOT APPLE AND U_CMAKE_BUILD_TYPE MATCHES MINSIZEREL)
            # Clang Gold plugin does not support -Os
            set(LTO_CXX_FLAGS "${LTO_CXX_FLAGS} -O3")
        endif()
      else()
        set(LTO_CXX_FLAGS "-flto -fno-fat-lto-objects")
        set(LTO_LINKER_FLAGS "-flto")
      endif()

      CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LTO ${LTO_CXX_FLAGS} ${LTO_LINKER_FLAGS})

      if (HAS_LTO)
        message(STATUS "NanoGUI: LTO support enabled.")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_LINKER_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LTO_LINKER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LTO_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LTO_CXX_FLAGS}")
      else()
        message(STATUS "NanoGUI: LTO not supported by the compiler.")
      endif()
    endif()
  elseif(MSVC)
    set(Configurations RELEASE RELWITHDEBINFO MINSIZEREL)
    set(LinkTypes EXE SHARED MODULE STATIC)
    foreach(Configuration ${Configurations})
      set("CMAKE_CXX_FLAGS_${Configuration}" "${CMAKE_CXX_FLAGS_${Configuration}} /GL")
      foreach(LinkType ${LinkTypes})
        set("CMAKE_${LinkType}_LINKER_FLAGS_${Configuration}" "${CMAKE_${LinkType}_LINKER_FLAGS_${Configuration}} /LTCG")
      endforeach()
    endforeach()
    message(STATUS "NanoGUI: LTO support enabled.")
  endif()
endif()

# Always use libc++ on Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if(HAS_LIBCPPABI)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi")
      message(STATUS "NanoGUI: using libc++ and libc++abi.")
    else()
      message(STATUS "NanoGUI: using libc++.")
    endif()
  else()
    message(STATUS "NanoGUI: NOT using libc++.")
  endif()
endif()

if (NANOGUI_USE_GLAD)
  # Build and include GLAD on Windows
  list(APPEND LIBNANOGUI_EXTRA_SOURCE
     "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c"
	 "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/glad/glad.h"
	 "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/KHR/khrplatform.h")
  if (MSVC)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c"
      PROPERTIES COMPILE_FLAGS "/wd4055 ")
  endif()
  include_directories(ext/glad/include)
  list(APPEND NANOGUI_EXTRA_DEFS -DNANOGUI_GLAD)
  list(APPEND NANOGUI_EXTRA_INCS "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include")
endif()

list(APPEND NANOGUI_EXTRA_INCS
  "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src"
)

if (NOT NANOGUI_EIGEN_INCLUDE_DIR)
  set(NANOGUI_EIGEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/eigen")
  list(APPEND NANOGUI_EXTRA_INCS "${NANOGUI_EIGEN_INCLUDE_DIR}")
endif()

# Required core libraries on various platforms
if (WIN32)
  list(APPEND NANOGUI_EXTRA_LIBS opengl32)
elseif (APPLE)
  find_library(cocoa_library Cocoa)
  find_library(opengl_library OpenGL)
  find_library(corevideo_library CoreVideo)
  find_library(iokit_library IOKit)
  list(APPEND NANOGUI_EXTRA_LIBS ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  list(APPEND LIBNANOGUI_EXTRA_SOURCE src/darwin.mm)
elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
  list(APPEND NANOGUI_EXTRA_LIBS GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread )
  if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND NANOGUI_EXTRA_LIBS rt)
  endif()
  if(CMAKE_SYSTEM MATCHES "Linux")
    list(APPEND NANOGUI_EXTRA_LIBS dl)
  endif()
endif()

include_directories(${NANOGUI_EIGEN_INCLUDE_DIR} ext/glfw/include ext/nanovg/src ext/cereal/include include ${CMAKE_CURRENT_BINARY_DIR})

# Run simple C converter to put font files into the data segment
# Globalize resource files
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ttf")

# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=nanogui_resources.cpp
  -DOUTPUT_H=nanogui_resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake")

# Run bin2c on resource files
add_custom_command(
  OUTPUT nanogui_resources.cpp nanogui_resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Set library type
if (NANOGUI_BUILD_SHARED)
  set(NANOGUI_LIBRARY_TYPE "SHARED")
else()
  set(NANOGUI_LIBRARY_TYPE "STATIC")
endif()

if (APPLE OR CMAKE_SYSTEM MATCHES "Linux")
  # Include coroutine support for running the mainloop in detached mode
  add_definitions(-DCORO_SJLJ)
  include_directories(ext/coro)
  list(APPEND LIBNANOGUI_PYTHON_EXTRA_SOURCE ext/coro/coro.c)
endif()

if (APPLE)
  # Use automatic reference counting for Objective-C portions
  add_compile_options(-fobjc-arc)
endif()

add_definitions(${NANOGUI_EXTRA_DEFS})

# Compile main NanoGUI library
add_library(nanogui-obj OBJECT
  # Merge NanoVG into the NanoGUI library
  ext/nanovg/src/nanovg.c
  # Merge GLAD into the NanoGUI library (only if needed)
  ${LIBNANOGUI_EXTRA_SOURCE}
  # Fonts etc.
  nanogui_resources.cpp
  include/nanogui/glutil.h src/glutil.cpp
  include/nanogui/common.h src/common.cpp
  include/nanogui/widget.h src/widget.cpp
  include/nanogui/theme.h src/theme.cpp
  include/nanogui/layout.h src/layout.cpp
  include/nanogui/screen.h src/screen.cpp
  include/nanogui/label.h src/label.cpp
  include/nanogui/window.h src/window.cpp
  include/nanogui/popup.h src/popup.cpp
  include/nanogui/checkbox.h src/checkbox.cpp
  include/nanogui/button.h src/button.cpp
  include/nanogui/popupbutton.h src/popupbutton.cpp
  include/nanogui/combobox.h src/combobox.cpp
  include/nanogui/progressbar.h src/progressbar.cpp
  include/nanogui/slider.h src/slider.cpp
  include/nanogui/messagedialog.h src/messagedialog.cpp
  include/nanogui/textbox.h src/textbox.cpp
  include/nanogui/imagepanel.h src/imagepanel.cpp
  include/nanogui/imageview.h src/imageview.cpp
  include/nanogui/vscrollpanel.h src/vscrollpanel.cpp
  include/nanogui/colorwheel.h src/colorwheel.cpp
  include/nanogui/colorpicker.h src/colorpicker.cpp
  include/nanogui/Plot.h
  include/nanogui/DataDisplay.h src/DataDisplay.cpp
  include/nanogui/pieChart.h src/pieChart.cpp
  include/nanogui/controlRodDisplay.h src/controlRodDisplay.cpp
  include/nanogui/ReactivityDisplay.h src/ReactivityDisplay.cpp
  include/nanogui/BarGraph.h
  include/nanogui/graph.h src/graph.cpp
  include/nanogui/stackedwidget.h src/stackedwidget.cpp
  include/nanogui/tabheader.h src/tabheader.cpp
  include/nanogui/tabwidget.h src/tabwidget.cpp
  include/nanogui/fileDialog.h src/fileDialog.cpp
  include/nanogui/formhelper.h
  include/nanogui/toolbutton.h
  include/nanogui/opengl.h
  include/nanogui/nanogui.h
  include/nanogui/serializer/core.h
  include/nanogui/serializer/opengl.h
  include/nanogui/serializer/sparse.h
  include/ScriptCommand.h src/ScriptCommand.cpp
  src/serializer.cpp
)

# XCode has a serious bug where the XCode project produces an invalid target
# that will not get linked if it consists only of objects from object libraries,
# it will not generate any products (executables, libraries). The only work
# around is to add a dummy source file to the library definition. This is an
# XCode, not a CMake bug. See: https://itk.org/Bug/view.php?id=14044
if (CMAKE_GENERATOR STREQUAL Xcode)
  set(XCODE_DUMMY ${CMAKE_CURRENT_BINARY_DIR}/xcode_dummy.cpp)
  file(WRITE ${XCODE_DUMMY} "")
  add_library(nanogui ${NANOGUI_LIBRARY_TYPE}
    ${XCODE_DUMMY}
    $<TARGET_OBJECTS:nanogui-obj>
    $<TARGET_OBJECTS:glfw_objects>
  )
else()
  add_library(nanogui ${NANOGUI_LIBRARY_TYPE}
    $<TARGET_OBJECTS:nanogui-obj>
    $<TARGET_OBJECTS:glfw_objects>
  )
endif()

# Compile/link flags for NanoGUI
set_property(TARGET nanogui-obj APPEND PROPERTY COMPILE_DEFINITIONS "NANOGUI_BUILD;NVG_BUILD")

if (NANOGUI_USE_GLAD AND NANOGUI_BUILD_SHARED)
  set_property(TARGET nanogui APPEND PROPERTY COMPILE_DEFINITIONS
    "GLAD_GLAPI_EXPORT;GLAD_GLAPI_EXPORT_BUILD")
endif()

if (NANOGUI_BUILD_SHARED)
  set_property(TARGET nanogui APPEND PROPERTY COMPILE_DEFINITIONS "_GLFW_BUILD_DLL;NVG_SHARED")
endif()

if (NANOGUI_INSTALL)
  install(
    TARGETS nanogui
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  install(
    DIRECTORY include/nanogui DESTINATION include
    FILES_MATCHING PATTERN "*.h"
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG AND NANOGUI_BUILD_SHARED)
    # Link-time code generation (only for shared library in release mode)
    set_property(TARGET nanogui APPEND_STRING PROPERTY COMPILE_FLAGS "-fvisibility=hidden ")

    # Check for Link Time Optimization support
    CHECK_CXX_COMPILER_FLAG("-flto" HAS_LTO_FLAG)
    if (HAS_LTO_FLAG)
      set_property(TARGET nanogui APPEND_STRING PROPERTY COMPILE_FLAGS "-flto ")
    endif()
  endif()
elseif (MSVC AND NANOGUI_BUILD_SHARED)
  # Link-time code generation (only for shared library in release mode)
  set_property(TARGET nanogui APPEND PROPERTY COMPILE_OPTIONS
    "$<$<CONFIG:Release>:/GL>" "$<$<CONFIG:MinSizeRel>:/GL>" "$<$<CONFIG:RelWithDebInfo>:/GL>")
  set_property(TARGET nanogui APPEND_STRING PROPERTY LINK_FLAGS_RELEASE "/LTCG ")
  set_property(TARGET nanogui APPEND_STRING PROPERTY LINK_FLAGS_MINSIZEREL "/LTCG ")
  set_property(TARGET nanogui APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO "/LTCG ")
endif()

if (NANOGUI_BUILD_SHARED)
  # When GLFW is merged into the NanoGUI library, this flag must be specified
  set_property(TARGET nanogui APPEND PROPERTY COMPILE_DEFINITIONS "_GLFW_BUILD_DLL")
endif()

if (NANOGUI_BUILD_SHARED AND NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
  if (APPLE)
    # Strip .dylib library on OSX
    add_custom_command(TARGET nanogui POST_BUILD COMMAND strip -u -r ${CMAKE_CURRENT_BINARY_DIR}/libnanogui.dylib)
  elseif(UNIX)
    # Strip .so library on Linux
    add_custom_command(TARGET nanogui POST_BUILD COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/libnanogui.so)
  endif()
endif()

# Quench warnings while compiling NanoVG
if (CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS -Wno-unused-result)
elseif(MSVC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS "/wd4005 /wd4456 /wd4457")
endif()

if (NANOGUI_BUILD_SHARED)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_DEFINITIONS "NVG_BUILD;NVG_SHARED")
else()
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_DEFINITIONS "NVG_BUILD")
endif()

# Build simulator
add_executable(SimulatorGUI src/SimulatorGUI.cpp src/ScriptCommand.cpp  src/Simulator.cpp ext/nanovg/src/nanovg.c include/Icon.h include/Simulator.h include/ControlRod.h resource1.h logo_256px.ico SimulatorGUI1.rc include/PeriodicalMode.h include/Settings.h include/SerialClass.h src/SerialClass.cpp)
target_link_libraries(SimulatorGUI nanogui ${NANOGUI_EXTRA_LIBS})
file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
if (NANOGUI_INSTALL)
    install(
      TARGETS SimulatorGUI
      RUNTIME DESTINATION bin
    )
 endif()

if (NANOGUI_BUILD_PYTHON)
  # Detect Python
  set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6 3.7)
  find_package(PythonLibs ${NANOGUI_PYTHON_VERSION})
  if (NOT PYTHONLIBS_FOUND)
    # Python not found -- disable the plugin
    set(NANOGUI_BUILD_PYTHON OFF CACHE BOOL "Build a Python plugin for NanoGUI?" FORCE)
  endif()
endif()

if (NANOGUI_BUILD_PYTHON)
  # Need PIC code in libnanogui even when compiled as a static library
  set_target_properties(nanogui PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(glfw_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

  include_directories("ext/pybind11/include" ${PYTHON_INCLUDE_DIR})
  add_library(nanogui_python SHARED python/python.cpp python/python.h python/py_doc.h
                                    ${LIBNANOGUI_PYTHON_EXTRA_SOURCE})
  set_target_properties(nanogui_python PROPERTIES OUTPUT_NAME "nanogui")
  target_link_libraries(nanogui_python nanogui ${NANOGUI_EXTRA_LIBS})

  # Quench GCC-related warnings
  if (CMAKE_COMPILER_IS_GNUCC)
    set_source_files_properties(python/python.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-variable)
  endif()

  set_target_properties(nanogui_python PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)
  set_target_properties(nanogui_python PROPERTIES PREFIX "")

  if (WIN32)
    # Set output path
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "Release/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "Debug/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "Release/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "Debug/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/python")

    # Link against the Python shared library
    target_link_libraries(nanogui-python ${PYTHON_LIBRARY})

    if (MSVC)
      # Optimize for size, /bigobj is needed for due to the heavy template metaprogramming in pybind11
      set_property(TARGET nanogui-python-obj APPEND PROPERTY COMPILE_OPTIONS
        "/bigobj" "$<$<CONFIG:Release>:/Os>" "$<$<CONFIG:MinSizeRel>:/Os>"
        "$<$<CONFIG:RelWithDebInfo>:/Os>")
    endif()
  elseif(UNIX)
    # .SO file extension on Linux/Mac OS
    set_target_properties(nanogui_python PROPERTIES SUFFIX ".so")

    # Optimize for a small binary size
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      set_target_properties(nanogui_python PROPERTIES COMPILE_FLAGS "-Os -fvisibility=hidden ")
    endif()

    # Strip unnecessary sections of the binary on Linux/Mac OS
    if(APPLE)
      set_target_properties(nanogui_python PROPERTIES MACOSX_RPATH ".")
      set_target_properties(nanogui_python PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")

      if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
        add_custom_command(TARGET nanogui_python POST_BUILD COMMAND strip -u -r ${CMAKE_CURRENT_BINARY_DIR}/python/nanogui.so)
      endif()
    else()
      if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
        add_custom_command(TARGET nanogui_python POST_BUILD COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/python/nanogui.so)
      endif()
    endif()
  endif()

  if (NANOGUI_INSTALL)
    install(
      TARGETS nanogui_python
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
    )
  endif()
endif()

get_directory_property(NANOGUI_HAS_PARENT PARENT_DIRECTORY)
if(NANOGUI_HAS_PARENT)
  # This project is included from somewhere else. Export NANOGUI_EXTRA_LIBS variable
  set(NANOGUI_EXTRA_LIBS ${NANOGUI_EXTRA_LIBS} PARENT_SCOPE)
  set(NANOGUI_EXTRA_DEFS ${NANOGUI_EXTRA_DEFS} PARENT_SCOPE)
  set(NANOGUI_EXTRA_INCS ${NANOGUI_EXTRA_INCS} PARENT_SCOPE)
else()
  # Create documentation for python plugin (optional target for developers)

  string(REPLACE " " ";" MKDOC_CXX_FLAGS_LIST ${CMAKE_CXX_FLAGS})
  get_property(MKDOC_INCLUDE_DIRECTORIES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  get_property(MKDOC_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)

  foreach (value ${MKDOC_INCLUDE_DIRECTORIES})
    list(APPEND MKDOC_CXX_FLAGS_LIST -I${value})
  endforeach()

  foreach (value ${MKDOC_COMPILE_DEFINITIONS})
    list(APPEND MKDOC_CXX_FLAGS_LIST -D${value})
  endforeach()

  add_custom_target(mkdoc COMMAND
    python3 ${PROJECT_SOURCE_DIR}/ext/pybind11/tools/mkdoc.py
      ${MKDOC_CXX_FLAGS_LIST}
      ${PROJECT_SOURCE_DIR}/include/nanogui/*.h
      > ${CMAKE_CURRENT_SOURCE_DIR}/python/py_doc.h)

endif()

# vim: set et ts=2 sw=2 ft=cmake nospell:
