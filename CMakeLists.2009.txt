cmake_minimum_required(VERSION 2.6.0)

# Avoid to include the CMakeLists.txt files themselves to the project to get rid
# of the custom build step in Visual Studio that regenerates the project.
set(CMAKE_SUPPRESS_REGENERATION TRUE)

# Use relative instead of absolute paths in generated project files.
set(CMAKE_USE_RELATIVE_PATHS TRUE)

# Do not use whitespaces in the project name.
set(name "MP3Diags")
project(${name})

# Search for Qt in QTDIR first, then in C:\Qt\** (on Windows only).
list(APPEND qt_glob_paths $ENV{QTDIR})

if(WIN32)
    file(GLOB dirs "C:/Qt/*")
    foreach(dir ${dirs})
        if(IS_DIRECTORY ${dir})
            list(APPEND qt_glob_paths ${dir})
        endif()
    endforeach()

    # Search paths with higher version numbers first.
    list(SORT qt_glob_paths)
    list(REVERSE qt_glob_paths)
endif()

set(QT_SEARCH_PATHS ${qt_glob_paths} CACHE PATH "Paths where the Qt library is searched in.")
set(CMAKE_PREFIX_PATH ${QT_SEARCH_PATHS})

# Try to find Qt, which on success provides QT_USE_FILE, which in turn provides
# QT_LIBRARIES.
find_package(Qt4 REQUIRED)

file(GLOB_RECURSE sources "src/*.cpp")
file(GLOB_RECURSE sources_h "src/*.h")
file(GLOB_RECURSE sources_ui "src/*.ui")
file(GLOB_RECURSE sources_qrc "src/*.qrc")

# For some more info see e.g. http://qtnode.net/wiki/Qt_with_cmake
qt4_wrap_cpp(sources_moc ${sources_h})
qt4_wrap_ui(sources_uic ${sources_ui})
qt4_add_resources(source_rcc ${sources_qrc})

# Enable the required Qt libraries.
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

# CMake places the ui_*.h files here.
include_directories(${CMAKE_BINARY_DIR})

# Try to find Boost, probably requires BOOST_ROOT to be set.
set(BOOST_SEARCH_PATHS "$ENV{BOOST_ROOT}" CACHE PATH "Paths where the Boost library is searched in.")
set(ENV{BOOST_ROOT} ${BOOST_SEARCH_PATHS})

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_executable(${name} ${sources} ${sources_h} ${sources_moc} ${sources_uic} ${sources_qrc} ${source_rcc})
target_link_libraries(${name} ${QT_LIBRARIES} ${Boost_LIBRARIES})

if(WIN32)
    # We want to use the Unicode Windows API, dynamically link against Boost and suppress deprecation / security warnings.
    add_definitions(-D_UNICODE -DUNICODE -DBOOST_ALL_DYN_LINK -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

    # Do not treat wchar_t as a built-in type (for compatibility with Qt).
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS "/Zc:wchar_t-") 

    # For "GetProcessMemoryInfo()".
    target_link_libraries(${name} psapi.lib)
endif()

# Hide a few variables from the CMake GUI.
set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "")
set(QT_QMAKE_EXECUTABLE ${QT_QMAKE_EXECUTABLE} CACHE INTERNAL "")
set(Boost_LIB_DIAGNOSTIC_DEFINITIONS ${Boost_LIB_DIAGNOSTIC_DEFINITIONS} CACHE INTERNAL "")
