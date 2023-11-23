# Expected file locations on Windows:
#   All relevant files are gathered from different places (build, Boost libraries, Qt libraries, ZLib) and copied to
#       build\Release\dist, such that build\Release\dist\MP3Diags-unstable.exe will just run
#   Examples of intermediary file locations: build\translations_qm\mp3diags_cs.qm, build\Release\MP3Diags-unstable.exe
#
# Expected file locations on Linux
#   The executable MP3Diags-unstable and the .qm files are right in the build folder


cmake_minimum_required(VERSION 3.12) # What 3.12 does is make cmake figure out a package's location based on an
# environment variable's name and a "_ROOT" suffix, e.g. Boost_ROOT. See https://cmake.org/cmake/help/latest/policy/CMP0074.html
# and see what happens when building on Windows with a smaller min version, like 3.5

set(project_name "MP3Diags-unstable")

project("${project_name}" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#set(CMAKE_CXX_STANDARD 17) # We don't really use anything from 17, and MSVC doesn't compile with it due to removal of auto_ptr and stream changes
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WIN32 is used for both 32 and 64 bits - https://cmake.org/cmake/help/latest/variable/WIN32.html
if(WIN32)
    if(NOT DEFINED ENV{Boost_ROOT})
        message(FATAL_ERROR "You must set Boost_ROOT environment variable")
    endif()
    #include_directories("$ENV{Boost_ROOT}/include")
    if(NOT DEFINED ENV{ZLIB_ROOT})
        message(FATAL_ERROR "You must set ZLIB_ROOT environment variable")
    endif()
    #include_directories("$ENV{ZLIB_ROOT}/include")
    #set(QT_BIN_DIR "$ENV{QT_BIN_DIR}")
    #string(REGEX REPLACE "\\" / QT_BIN_DIR "$ENV{QT_BIN_DIR}")
    string(REGEX REPLACE "\\\\" "/" QT_BIN_DIR "$ENV{QT_BIN_DIR}") #ttt1 This isn't quite right, it's an artefact of
    #  the .cmd file. OTOH Qt5_INCLUDE_DIRS and Qt5_LIBRARY_DIRS are empty (even after running find_package)
    string(REGEX REPLACE "/bin$" "/" QT_ROOT ${QT_BIN_DIR})
endif()


#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Xml Network) # If 5 or 6 are found, QT_VERSION_MAJOR will be defined
#  to either 5 or 6 - https://doc.qt.io/qt-6/cmake-qt5-and-qt6-compatibility.html, and QT_VERSION will be defined as well, which is also needed. Well, currently we don't support 6, so we only accept 5
find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets LinguistTools Xml Network)
find_package(Qt5 REQUIRED COMPONENTS Widgets LinguistTools Xml Network)
find_package(ZLIB REQUIRED)

find_package(Boost REQUIRED COMPONENTS program_options serialization)
include_directories(${Boost_INCLUDE_DIRS})  # These are not needed on Linux # ttt1 maybe do the same for zlib (on Linux, GCC seems to have some sort of built-in zlib: "--with-system-zlib")
link_directories(${Boost_LIBRARY_DIRS})


file(GLOB TS_FILES "src/translations/*.ts")
message(STATUS "Translations: ${TS_FILES}")


#-----------------
# In languages we have both things like "fr" and "fr_FR", as Qt might have one or the other
set(LANGUAGES1 ${TS_FILES})
# The point of using WWW is that list replacement is run repeatedly as long as there are matches, so you cannot stop for an earlier "_"
list(TRANSFORM LANGUAGES1 REPLACE "[^_]*_([a-z][a-z])_([A-Z]+).ts" "\\1WWW\\2")
list(TRANSFORM LANGUAGES1 REPLACE "[^_]*_(.*)\\.ts" "\\1")
list(TRANSFORM LANGUAGES1 REPLACE "WWW" "_")

set(LANGUAGES2 ${LANGUAGES1})
list(TRANSFORM LANGUAGES2 REPLACE "_.*" "")

set(LANGUAGES ${LANGUAGES1} ${LANGUAGES2})
list(REMOVE_DUPLICATES LANGUAGES)

#message(STATUS "LANGUAGES1: ${LANGUAGES1}")
#message(STATUS "LANGUAGES2: ${LANGUAGES2}")
message(STATUS "LANGUAGES: ${LANGUAGES}")
#-----------------

if(WIN32)
    message(STATUS "On Windows, BUILD_TYPE is forced to Release, which matters for debugging library code")
    set(BUILD_TYPE Release) # ttt0 this is wrong, but it doesn't seem possible to do it right, and other areas assume Release anyway
    # https://stackoverflow.com/questions/39282430/how-to-get-current-configuration-release-debug-in-cmake-for-visual-studio
    # https://stackoverflow.com/questions/24460486/cmake-build-type-is-not-being-used-in-cmakelists-txt
    # https://stackoverflow.com/questions/68839452/cmake-determine-the-build-configuration-name
    # Note that there's also a CMAKE_BUILD_TYPE, which is what should normally be passed when calling CMake
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "BUILD_TYPE: ${BUILD_TYPE}")

set(PROJECT_SOURCE_FILES
        src/About.ui
        src/AboutDlgImpl.cpp
        src/AboutDlgImpl.h
        src/AlbumInfoDownloader.ui
        src/AlbumInfoDownloaderDlgImpl.cpp
        src/AlbumInfoDownloaderDlgImpl.h
        src/ApeStream.cpp
        src/ApeStream.h
        src/CbException.cpp
        src/CbException.h
        src/CheckedDir.cpp
        src/CheckedDir.h
        src/ColumnResizer.cpp
        src/ColumnResizer.h
        src/CommonData.cpp
        src/CommonData.h
        src/CommonTypes.cpp
        src/CommonTypes.h
        src/Config.ui
        src/ConfigDlgImpl.cpp
        src/ConfigDlgImpl.h
        src/DataStream.cpp
        src/DataStream.h
        src/Debug.ui
        src/DebugDlgImpl.cpp
        src/DebugDlgImpl.h
        src/DirFilter.ui
        src/DirFilterDlgImpl.cpp
        src/DirFilterDlgImpl.h
        src/DiscogsDownloader.cpp
        src/DiscogsDownloader.h
        src/DoubleList.cpp
        src/DoubleList.h
        src/DoubleListWdg.ui
        src/Export.ui
        src/ExportDlgImpl.cpp
        src/ExportDlgImpl.h
        src/ExternalTool.ui
        src/ExternalToolDlgImpl.cpp
        src/ExternalToolDlgImpl.h
        src/FileEnum.cpp
        src/FileEnum.h
        src/FileRenamer.ui
        src/FileRenamerDlgImpl.cpp
        src/FileRenamerDlgImpl.h
        src/FilesModel.cpp
        src/FilesModel.h
        src/fstream_unicode.cpp
        src/fstream_unicode.h
        src/FullSizeImgDlg.cpp
        src/FullSizeImgDlg.h
        src/Helpers.cpp
        src/Helpers.h
        src/Id3Transf.cpp
        src/Id3Transf.h
        src/Id3V230Stream.cpp
        src/Id3V230Stream.h
        src/Id3V240Stream.cpp
        src/Id3V240Stream.h
        src/Id3V2Stream.cpp
        src/Id3V2Stream.h
        src/ImageInfoPanel.ui
        src/ImageInfoPanelWdgImpl.cpp
        src/ImageInfoPanelWdgImpl.h
        src/LogModel.cpp
        src/LogModel.h
        src/LyricsStream.cpp
        src/LyricsStream.h
        src/main.cpp
        src/MainForm.ui
        src/MainFormDlgImpl.cpp
        src/MainFormDlgImpl.h
        src/Mp3Manip.cpp
        src/Mp3Manip.h
        src/Mp3TransformThread.cpp
        src/Mp3TransformThread.h
        src/MpegFrame.cpp
        src/MpegFrame.h
        src/MpegStream.cpp
        src/MpegStream.h
        src/MultiLineTvDelegate.cpp
        src/MultiLineTvDelegate.h
        src/MusicBrainzDownloader.cpp
        src/MusicBrainzDownloader.h
        src/NoteFilter.ui
        src/NoteFilterDlgImpl.cpp
        src/NoteFilterDlgImpl.h
        src/Notes.cpp
        src/Notes.h
        src/NotesModel.cpp
        src/NotesModel.h
        src/OsFile.cpp
        src/OsFile.h
        src/Palette.ui
        src/PaletteDlgImpl.cpp
        src/PaletteDlgImpl.h
        src/Patterns.ui
        src/QHttp
        src/QHttpRequestHeader
        src/QHttpResponseHeader
        src/RenamerPatternsDlgImpl.cpp
        src/RenamerPatternsDlgImpl.h
        src/Scan.ui
        src/ScanDlgImpl.cpp
        src/ScanDlgImpl.h
        src/SerSupport.cpp
        src/SerSupport.h
        src/SessionEditor.ui
        src/SessionEditorDlgImpl.cpp
        src/SessionEditorDlgImpl.h
        src/Sessions.ui
        src/SessionsDlgImpl.cpp
        src/SessionsDlgImpl.h
        src/SimpleSaxHandler.h
        src/SongInfoParser.cpp
        src/SongInfoParser.h
        src/StoredSettings.cpp
        src/StoredSettings.h
        src/StreamsModel.cpp
        src/StreamsModel.h
        src/StructuralTransformation.cpp
        src/StructuralTransformation.h
        src/TagEditor.ui
        src/TagEditorDlgImpl.cpp
        src/TagEditorDlgImpl.h
        src/TagEdtPatternsDlgImpl.cpp
        src/TagEdtPatternsDlgImpl.h
        src/TagReadPanel.cpp
        src/TagReadPanel.h
        src/TagWriter.cpp
        src/TagWriter.h
        src/ThreadRunner.ui
        src/ThreadRunnerDlgImpl.cpp
        src/ThreadRunnerDlgImpl.h
        src/Transformation.cpp
        src/Transformation.h
        src/Translation.cpp
        src/Translation.h
        src/UniqueNotesModel.cpp
        src/UniqueNotesModel.h
        src/Version.cpp
        src/Version.h
        src/Widgets.cpp
        src/Widgets.h
        ${TS_FILES}
)

set(PROJECT_RESOURCE_FILES
        src/Mp3Diags.qrc
)

qt5_add_resources(PROJECT_RESOURCES ${PROJECT_RESOURCE_FILES})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${project_name}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCE_FILES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${project_name} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    #qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(WIN32)
        set(qm_output_location "translations_qm")
        message(STATUS "Creating dir ${qm_output_location}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${qm_output_location}")
    else()
        set(qm_output_location ".")
    endif()

    set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${qm_output_location}")
    #qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES}) # !!! We don't want qt5_create_translation, which has a tendency to delete the .ts files due
    # to them "being generated from sources". Instead, we update them manually as needed, via lupdate-qt5 and treat them here as sources
    #qt5_add_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    qt5_add_translation(QM_FILES ${TS_FILES})

    if(ANDROID)
        add_library(${project_name} SHARED
            ${PROJECT_SOURCE_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${project_name}
            ${PROJECT_SOURCE_FILES}
            ${PROJECT_RESOURCES}
            ${QM_FILES}
        )
    endif()

endif()

target_link_libraries(${project_name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
    ZLIB::ZLIB
    ${Boost_LIBRARIES})

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${project_name})
endif()

set_target_properties(${project_name} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${project_name}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${project_name})
endif()

if(WIN32) # ttt0 review these  #ttt0 see about MSVC rather than OS
    # We want to use the Unicode Windows API, dynamically link against Boost and suppress deprecation / security warnings.
#    add_definitions(-D_UNICODE -DUNICODE -DBOOST_ALL_DYN_LINK -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_UNICODE -DUNICODE -DBOOST_ALL_DYN_LINK -D_CRT_SECURE_NO_WARNINGS) # ttt1 fix warnings, not hide them

    # Do not treat wchar_t as a built-in type (for compatibility with Qt).
#    set_target_properties(${project_name} PROPERTIES COMPILE_FLAGS "/Zc:wchar_t-")

    # For "GetProcessMemoryInfo()".
#    target_link_libraries(${project_name} PRIVATE psapi.lib)

#    add_custom_command(TARGET "${project_name}" POST_BUILD
#        #COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_base> ${PROJECT_BINARY_DIR}/test_base.exe
#        COMMAND ${CMAKE_COMMAND} -E copy C:\Mp3Diags\MP3Diags-unstable-1.5.02\build\Release\dist\translations\qt_help_zh_CN.qm C:\Mp3Diags\MP3Diags-unstable-1.5.02\build\Release\dist\styles
#        #COMMENT "Created ${PROJECT_BINARY_DIR}/test_base.exe"
#    )

    #add_custom_command(TARGET "${project_name}" POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy C:/Mp3Diags/MP3Diags-unstable-1.5.02/build/Release/dist/translations/qt_help_zh_CN.qm C:/Mp3Diags/MP3Diags-unstable-1.5.02/build/Release/dist/styles
    #)

    add_custom_command(TARGET "${project_name}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${qm_output_location} ${BUILD_TYPE}/dist  #ttt0 Replace the need for qm_output_location with a pattern-based file copy
        COMMAND ${CMAKE_COMMAND} -E copy "${BUILD_TYPE}/${project_name}.exe" ${BUILD_TYPE}/dist
    )
    #install(TARGETS "${project_name}" DESTINATION ${BUILD_TYPE}/dist) #ttt1 see if this can be used to copy the exe

    file(GLOB BOOST_SER_DLL "${Boost_LIBRARY_DIRS}/boost_serialization*-mt-x64-*.dll")
    message(STATUS "BOOST_SER_DLL: ${BOOST_SER_DLL}")
    file(COPY "${BOOST_SER_DLL}" DESTINATION ${BUILD_TYPE}/dist)

    file(GLOB BOOST_PROG_OPT_DLL "${Boost_LIBRARY_DIRS}/boost_program_options*-mt-x64-*.dll")
    message(STATUS "BOOST_PROG_OPT_DLL: ${BOOST_PROG_OPT_DLL}")
    #add_custom_command(TARGET "${project_name}" POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy "${BOOST_PROG_OPT_DLL}" ${BUILD_TYPE}/dist
    #)
    file(COPY "${BOOST_PROG_OPT_DLL}" DESTINATION ${BUILD_TYPE}/dist)

    #message(STATUS "languages=${LANGUAGES}")
    foreach(language IN LISTS LANGUAGES)
        #message(STATUS "language=${language}")
        #file(GLOB langFiles "${QT_ROOT}/translations/*_${language}\\.qm")
        #message(STATUS "searching: ${QT_ROOT}translations/*_${language}.qm")
        file(GLOB langFiles "${QT_ROOT}translations/*_${language}.qm")
        #message(STATUS "langFiles: ${langFiles}")
        foreach(f IN LISTS langFiles)
            file(COPY "${f}" DESTINATION "${BUILD_TYPE}/dist/translations")
        endforeach()
    endforeach()
# boost_program_options-vc143-mt-x64-1_83.dll
# C:/local/boost_1_83_0/lib64-msvc-14.3
# Boost_VERSION: 108300

    message(STATUS "CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_BINDIR}")
    message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "LANGUAGES: ${LANGUAGES}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

    message(STATUS "QT_VERSION_MAJOR: ${QT_VERSION_MAJOR}")
    message(STATUS "QT_ROOT: ${QT_ROOT}")

    # These are all empty
    # message(STATUS "Qt5_INCLUDE_DIRS: ${Qt5_INCLUDE_DIRS}")
    # message(STATUS "Qt5_LIBRARY_DIRS: ${Qt5_LIBRARY_DIRS}")
    # message(STATUS "QT5_INCLUDE_DIRS: ${QT5_INCLUDE_DIRS}")
    # message(STATUS "QT5_LIBRARY_DIRS: ${QT5_LIBRARY_DIRS}")
    #
    # message(STATUS "Qt_INCLUDE_DIRS: ${Qt_INCLUDE_DIRS}")
    # message(STATUS "Qt_LIBRARY_DIRS: ${Qt_LIBRARY_DIRS}")
    # message(STATUS "QT_INCLUDE_DIRS: ${QT_INCLUDE_DIRS}")
    # message(STATUS "QT_LIBRARY_DIRS: ${QT_LIBRARY_DIRS}")

    message(STATUS "QT_BIN_DIR: ${QT_BIN_DIR}")
    #message(STATUS "QT_BIN_DIR: ENV{QT_BIN_DIR}")

    message(STATUS "ZLIB_INCLUDE_DIRS: ${ZLIB_INCLUDE_DIRS}")
    #message(STATUS "ZLIB_LIBRARY_DIRS: ${ZLIB_LIBRARY_DIRS}")

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

    message(STATUS "Boost_VERSION_MAJOR: ${Boost_VERSION_MAJOR}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
else()
#    This is supposed to strip debug info in the Release target, but it's not portable and doesn't seem to work anyway
#    if(CMAKE_BUILD_TYPE EQUAL Release)
#        set_target_properties(TARGET_NAME PROPERTIES LINK_FLAGS_RELEASE -s)
#    endif()
    add_custom_command(TARGET "${project_name}" POST_BUILD
            COMMENT "Stripping debug info ..."
            #COMMAND sleep 10
            COMMAND strip ${project_name}  # ttt1 Not too portable
            )
endif()
