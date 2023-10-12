cmake_minimum_required(VERSION 3.5)

project(mp3diags-cmake VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# file:///r/ciobi/cpp/mp3diags-cmake/build/build-mp3diags-cmake-Desktop-Debug/CMakeFiles/mp3diags-cmake.dir/build.make had at the
# end /usr/lib64/libQt5Widgets.so.5.15.11 but no /usr/lib64/libQt5Xml.so.5.15.11, but that was fixed by adding to target_link_libraries,
# which fixed the includes as well

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Xml Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Xml Network)
find_package(ZLIB REQUIRED)

find_package(Boost REQUIRED COMPONENTS program_options serialization)
include_directories(${Boost_INCLUDE_DIRS})  # ttt0 maybe do the same for zlib (on Linux, GCC seems to have some sort of built-in zlib: "--with-system-zlib")
link_directories(${Boost_LIBRARY_DIRS})


set(TS_FILES
        src/translations/mp3diags_cs.ts
        src/translations/mp3diags_de_DE.ts
        src/translations/mp3diags_fr_FR.ts
)

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
    qt_add_executable(mp3diags-cmake
        MANUAL_FINALIZATION
        ${PROJECT_SOURCE_FILES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET mp3diags-cmake APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(mp3diags-cmake SHARED
            ${PROJECT_SOURCE_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(mp3diags-cmake
            ${PROJECT_SOURCE_FILES}
            ${PROJECT_RESOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(mp3diags-cmake PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Network ZLIB::ZLIB ${Boost_LIBRARIES})
#target_link_libraries(mp3diags-cmake ${Boost_LIBRARIES})
#

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.mp3diags-cmake)
endif()
set_target_properties(mp3diags-cmake PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS mp3diags-cmake
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(mp3diags-cmake)
endif()
