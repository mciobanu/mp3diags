# =============== Usage ===============
#
# -------------- Windows --------------
#
# The only unusual thing is adjusting environment variables in CMakePresets.json. There are some values there, but
#   they might need to be changed to match your current installation. Make sure you don't use a subfolder:
#       Boost_ROOT - Contains files "boostcpp.jam" or "boost-build.jam"
#       ZLIB_ROOT - Contains folders "include" and "lib"
#       Qt5_ROOT - Contains folders "include", "lib", "mkspecs", "translations", ...
# Then the easiest is to open the project in Visual Studio. Use the menu File / Open / CMake
#
# To build from the command line:
#   open a Visual Studio command prompt (called something like "x64 Native Tools Command Prompt for VS 2022")
#   run this (or perhaps make adjustments for other architectures or configurations):
#       cmake -S . -B out\build\x64-release --preset x64-release && cmake --build out\build\x64-release --config x64-release
#   If all went OK, the output is located in out\build\x64-release, including required dependencies
#
# --------------- Linux ---------------
#
# After installing the dependencies (cmake, g++, dev libraries for Qt5, Boost Program Options, Boost Serialization, ZLib), run one of these:
#   CPU_COUNT=12 cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release && cmake --build build/release -j $CPU_COUNT
#   CPU_COUNT=12 cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/debug -j $CPU_COUNT
# (Change the value of CPU_COUNT to whatever you want to use.)
#
#   The executable and the .qm files (used for translation) are in the build folder


cmake_minimum_required(VERSION 3.12) # What 3.12 does is make cmake figure out a package's location based on an
# environment variable's name and a "_ROOT" suffix, e.g. Boost_ROOT. See https://cmake.org/cmake/help/latest/policy/CMP0074.html
# and see what happens when building on Windows with a smaller min version, like 3.5

# ttt1 Normally there should be a CMakeLists.txt in the project root and another in src, included in the former

if(WIN32)
    set(project_name "MP3DiagsWindows-unstable")
else()
    set(project_name "MP3Diags-unstable")
endif()

project("${project_name}" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#set(CMAKE_CXX_STANDARD 17) # We don't really use anything from 17, and MSVC doesn't compile with it due to removal of auto_ptr and stream changes
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WIN32 is used for both 32 and 64 bits - https://cmake.org/cmake/help/latest/variable/WIN32.html
if(WIN32)
    if(NOT DEFINED ENV{Boost_ROOT}) #  !!! Boost_ROOT vs BOOST_ROOT: As find_package() is for Boost, it probably makes sense to keep Pascal case. See also: https://github.com/conan-io/conan/issues/4597
        message(FATAL_ERROR "You must set the Boost_ROOT environment variable")
    endif()
    #include_directories("$ENV{Boost_ROOT}/include")
    if(NOT DEFINED ENV{ZLIB_ROOT})
        message(FATAL_ERROR "You must set the ZLIB_ROOT environment variable")
    endif()
    if(NOT DEFINED ENV{Qt5_ROOT})
        message(FATAL_ERROR "You must set the Qt5_ROOT environment variable")
    endif()
    string(REGEX REPLACE "\\\\" "/" Qt5_ROOT "$ENV{Qt5_ROOT}") # extract environment variable to a proper variable to use later to call windeployqt
endif()


# https://doc.qt.io/qt-6/cmake-qt5-and-qt6-compatibility.html
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

# For the executable's icon
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/MP3DiagsIcon.rc)
endif()

#-----------------

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

if(WIN32)
#        set(qm_output_location "translations_qm")
#        message(STATUS "Creating dir ${qm_output_location}")
#        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${qm_output_location}")
else()
    set(qm_output_location ".")
endif()

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${qm_output_location}")
#qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES}) # !!! We don't want qt5_create_translation, which has a tendency to delete the .ts files due
# to them "being generated from sources". Instead, we update them manually as needed, via lupdate-qt5 and treat them here as sources
#qt5_add_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
qt5_add_translation(QM_FILES ${TS_FILES})

add_executable(${project_name}
    ${PROJECT_SOURCE_FILES}
    ${PROJECT_RESOURCES}
    ${QM_FILES}
    ${WIN32_RESOURCES} # this not being defined on Linux seems OK
)

target_link_libraries(${project_name} PRIVATE
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    ZLIB::ZLIB
    ${Boost_LIBRARIES})


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
#if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${project_name})
#endif()

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

    if ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
        set(DLL_NAME_EXTRA "-gd")
        message(STATUS "Debug build")
    else()
        set(DLL_NAME_EXTRA "")
        message(STATUS "Release build")
    endif()
    file(GLOB BOOST_SER_DLL "${Boost_LIBRARY_DIRS}/boost_serialization*-mt${DLL_NAME_EXTRA}-x64-*.dll")
    message(STATUS "BOOST_SER_DLL: ${BOOST_SER_DLL}")
    file(COPY "${BOOST_SER_DLL}" DESTINATION .)

    file(GLOB BOOST_PROG_OPT_DLL "${Boost_LIBRARY_DIRS}/boost_program_options*-mt${DLL_NAME_EXTRA}-x64-*.dll")
    message(STATUS "BOOST_PROG_OPT_DLL: ${BOOST_PROG_OPT_DLL}")
    #add_custom_command(TARGET "${project_name}" POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy "${BOOST_PROG_OPT_DLL}" ${BUILD_TYPE}/dist
    #)
    file(COPY "${BOOST_PROG_OPT_DLL}" DESTINATION .)

    SET(LICENSES boost gplv2 gplv3 "lgpl-2.1" lgplv3 zlib)
    message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
    foreach(license IN LISTS LICENSES)
        #configure_file("license.${license}.txt" ${BUILD_TYPE}/dist/zz.${license}.txt COPYONLY)
        if(EXISTS "license.${license}.txt") #ttt0 Stop renaming licenses in MakeArchives.sh, so this test won't be needed
            file(COPY "license.${license}.txt" DESTINATION .)
            file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/license.${license}.txt" "${CMAKE_CURRENT_BINARY_DIR}/${license}.txt")
        else()
            file(COPY "src/licences/${license}.txt" DESTINATION .)
        endif()
    endforeach()

    file(COPY changelog.txt DESTINATION .)
    file(COPY doc/favicon.ico DESTINATION .)

    file(GLOB CLI_RUNNER "MP3DiagsCLI*.cmd")
    file(COPY "${CLI_RUNNER}" DESTINATION .)

    #message(STATUS "languages=${LANGUAGES}")

    # Tells Qt to build the redistributable files. This involves combining .qm files, and copying DLLs and
    # vc_redist.x64.exe, which contains Visual C++'s dependencies
    add_custom_command(TARGET "${project_name}" POST_BUILD
            #COMMENT "******************** windeployqt ${RUNTIME_OUTPUT_DIRECTORY}\n"
            COMMAND "${Qt5_ROOT}/bin/windeployqt" "./${project_name}.exe"
        )

    string(REPLACE "/" "\\" WND_TRANSL_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations")
    # windeployqt copies to ${WND_TRANSL_DIR} all the translations it knows about; we keep the ones that we want and
    # then delete the rest; (and "copy" is not a good word, as it combines multiple files to create a single one,
    # e.g. qt_fr.qm, qtbase_fr.qm, qtmultimedia_fr.qm, and others, are combined into a larger qt_fr.qm)

    foreach(language IN LISTS LANGUAGES)
        # Not all languages exist, as we try both "fr_FR" and just "fr", and that's fine
        add_custom_command(TARGET "${project_name}" POST_BUILD
                #COMMENT "******************** Copying translation for ${language} ...\n"
                COMMAND echo "trying ${language} in ${WND_TRANSL_DIR}\\qt_${language}.qm"
                #COMMAND cmd /c "if exist \"${WND_TRANSL_DIR}\\qt_${language}.qm\" copy \"${WND_TRANSL_DIR}\\qt_${language}.qm\" \"${WND_TRANSL_DIR}\\..\""
                #COMMAND cmd /c "if exist ${WND_TRANSL_DIR}\\qt_${language}.qm copy ${WND_TRANSL_DIR}\\qt_${language}.qm ${WND_TRANSL_DIR}\\.."
                #COMMAND echo "if exist \"${WND_TRANSL_DIR}\\qt_${language}.qm\" copy \"${WND_TRANSL_DIR}\\qt_${language}.qm\" \"${WND_TRANSL_DIR}\\..\""

                #COMMAND echo ""
                COMMAND cmd /c if exist "${WND_TRANSL_DIR}\\qt_${language}.qm" copy "${WND_TRANSL_DIR}\\qt_${language}.qm" "${WND_TRANSL_DIR}\\.."
                VERBATIM
                )
    endforeach()

    add_custom_command(TARGET "${project_name}" POST_BUILD
            #COMMENT "******************** Removing ${WND_TRANSL_DIR} ...\n"
            COMMAND rmdir /S /Q "${WND_TRANSL_DIR}"
            )

    message(STATUS "CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_BINDIR}")
    message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "LANGUAGES: ${LANGUAGES}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

    message(STATUS "QT_VERSION_MAJOR: ${QT_VERSION_MAJOR}")
    message(STATUS "Qt5_ROOT: ${Qt5_ROOT}")

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

    #message(STATUS "QT_BIN_DIR: ENV{QT_BIN_DIR}")

    message(STATUS "ZLIB_INCLUDE_DIRS: ${ZLIB_INCLUDE_DIRS}")
    #message(STATUS "ZLIB_LIBRARY_DIRS: ${ZLIB_LIBRARY_DIRS}")

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

    message(STATUS "Boost_VERSION_MAJOR: ${Boost_VERSION_MAJOR}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
else()
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        # This is supposed to strip debug info in the Release target, but it's not portable and doesn't seem to work anyway
        # set_target_properties(TARGET_NAME PROPERTIES LINK_FLAGS_RELEASE -s)

        add_custom_command(TARGET "${project_name}" POST_BUILD
                COMMENT "Stripping debug info ..."
                #COMMAND sleep 10
                COMMAND strip ${project_name}  # ttt1 Not too portable
                )
    endif()
endif()
