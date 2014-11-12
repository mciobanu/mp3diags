SOURCES +=  \
 Helpers.cpp \
 main.cpp \
 DiscogsDownloader.cpp \
 DoubleList.cpp \
 FileEnum.cpp \
 FileRenamerDlgImpl.cpp \
 FilesModel.cpp \
 Id3Transf.cpp \
 Id3V230Stream.cpp \
 Id3V240Stream.cpp \
 Id3V2Stream.cpp \
 ImageInfoPanelWdgImpl.cpp \
 LogModel.cpp \
 LyricsStream.cpp \
 MainFormDlgImpl.cpp \
 Mp3Manip.cpp \
 Mp3TransformThread.cpp \
 MpegFrame.cpp \
 MpegStream.cpp \
 MultiLineTvDelegate.cpp \
 MusicBrainzDownloader.cpp \
 ExternalToolDlgImpl.cpp \
 NoteFilterDlgImpl.cpp \
 Notes.cpp \
 NotesModel.cpp \
 OsFile.cpp \
 PaletteDlgImpl.cpp \
 RenamerPatternsDlgImpl.cpp \
 ScanDlgImpl.cpp \
 SessionEditorDlgImpl.cpp \
 SessionsDlgImpl.cpp \
 SongInfoParser.cpp \
 StoredSettings.cpp \
 StreamsModel.cpp \
 StructuralTransformation.cpp \
 TagEditorDlgImpl.cpp \
 TagEdtPatternsDlgImpl.cpp \
 TagReadPanel.cpp \
 TagWriter.cpp \
 ThreadRunnerDlgImpl.cpp \
 Transformation.cpp \
 UniqueNotesModel.cpp \
 Widgets.cpp \
 AboutDlgImpl.cpp \
 AlbumInfoDownloaderDlgImpl.cpp \
 ApeStream.cpp \
 CheckedDir.cpp \
 ColumnResizer.cpp \
 CommonData.cpp \
 CommonTypes.cpp \
 ConfigDlgImpl.cpp \
 DataStream.cpp \
 DebugDlgImpl.cpp \
 DirFilterDlgImpl.cpp \
 Version.cpp \
 fstream_unicode.cpp \
 ExportDlgImpl.cpp \
 SerSupport.cpp \
  FullSizeImgDlg.cpp \
    Translation.cpp \
    CbException.cpp
TEMPLATE = app
CONFIG += warn_on \
	  thread \
          qt \
 debug_and_release
TARGET = MP3Diags-unstable
DESTDIR = ../bin

QT += xml \
network

RESOURCES += Mp3Diags.qrc

HEADERS += AboutDlgImpl.h \
AlbumInfoDownloaderDlgImpl.h \
ApeStream.h \
CheckedDir.h \
ColumnResizer.h \
CommonData.h \
CommonTypes.h \
ConfigDlgImpl.h \
DataStream.h \
DebugDlgImpl.h \
DirFilterDlgImpl.h \
DiscogsDownloader.h \
DoubleList.h \
FileEnum.h \
FileRenamerDlgImpl.h \
FilesModel.h \
Helpers.h \
Id3Transf.h \
Id3V230Stream.h \
Id3V240Stream.h \
Id3V2Stream.h \
ImageInfoPanelWdgImpl.h \
LogModel.h \
LyricsStream.h \
MainFormDlgImpl.h \
Mp3Manip.h \
Mp3TransformThread.h \
MpegFrame.h \
MpegStream.h \
MultiLineTvDelegate.h \
MusicBrainzDownloader.h \
ExternalToolDlgImpl.h \
NoteFilterDlgImpl.h \
Notes.h \
NotesModel.h \
OsFile.h \
PaletteDlgImpl.h \
RenamerPatternsDlgImpl.h \
ScanDlgImpl.h \
SerSupport.h \
SessionEditorDlgImpl.h \
SessionsDlgImpl.h \
SimpleSaxHandler.h \
SongInfoParser.h \
StoredSettings.h \
StreamsModel.h \
StructuralTransformation.h \
TagEditorDlgImpl.h \
TagEdtPatternsDlgImpl.h \
TagReadPanel.h \
TagWriter.h \
ThreadRunnerDlgImpl.h \
Transformation.h \
UniqueNotesModel.h \
Widgets.h \
 fstream_unicode.h \
 ExportDlgImpl.h \
 FullSizeImgDlg.h \
 Version.h \
    Translation.h \
    CbException.h
FORMS += About.ui \
AlbumInfoDownloader.ui \
Config.ui \
Debug.ui \
DirFilter.ui \
DoubleListWdg.ui \
FileRenamer.ui \
ImageInfoPanel.ui \
MainForm.ui \
ExternalTool.ui \
NoteFilter.ui \
Palette.ui \
Patterns.ui \
Scan.ui \
SessionEditor.ui \
Sessions.ui \
TagEditor.ui \
ThreadRunner.ui \
 Export.ui

UI_DIR = ui-forms

#CONFIG += console

# !!! One thing to keep in mind is that when using BuildMp3Diags.hta the file src.pro shouldn't be changed. At the first build attempt a copy src.pro1 is created in the "package" dir, and that is where changes for the actual location of the libs


#DEFINES += DISABLE_CHECK_FOR_UPDATES
#DEFINES += OS2

# GENERATE_TOC and GAPLESS_SUPPORT are not independent (though perhaps they should be), so GAPLESS_SUPPORT is ignored if GENERATE_TOC is set
# GAPLESS_SUPPORT also need LAME library
#QMAKE_CXXFLAGS_DEBUG += -DGENERATE_TOC
#QMAKE_CXXFLAGS_DEBUG += -DGAPLESS_SUPPORT
#QMAKE_CXXFLAGS += -DGAPLESS_SUPPORT

LIBS += -lz \
  -lboost_serialization-mt \
  -lboost_program_options-mt

#LIBS += -lmp3lame

# Notes for Windows gapless support:
#   - It's hard to build Lame and prebuilt libs are not official and don't work:
#       - they don't have the apparently newer hip_decode_init(), but only the older lame_decode_init(), which seems to have been deprecated in 2004
#       - there were different library names, like lame_enc.dll
#       - it is supposedly possible to build an .a and then convert it to a .dll
#       - older installed MinGW cannot compile LAME; there is an addon MSYS / RXVT, which adds "make" and a few other things and thus allows building, but it's probably hard to get for the older version which came with the Qt used for building
#       - DLLs built with MSVC allow the MinGW built exe to start, but trying to use LAME causes a crash
#       - DLLs built with MSVC also cause the MSVC-built exe to crash or not start at all (claiming that some entry point is missing from the zlib DLL)
#       - building from outside MSVC requires changes in the Makefile, as some deprecated options aren't understood; something like this worked after getting rid of "/opt:NOWIN98"
#               nmake -f Makefile.MSVC ASM=qt CFG=qt dll
#           ("qt" didn't mean anything except that it forced some setting to not be used)
#           not sure if the things this created were fine; only tested the DLL and it crashed; when building from within Visual Studio also tried the static libs
#
#   - What seemed to do the trick was building LAME libs from within Visual Studio, then link 2 libs statically to the MSVC-build MP3 Diags:
#       - there's a vc_solution dir; import the project from there; it will complain a bit but actually works
#       - build the libs in RELEASE (there are some optimized Release builds, didn't try them; one needs a separate assembler)
#
#LIBS += C:\\Users\\ciobi\\temp\\lame-3.99.5-03\\lame-3.99.5\\output\\Release\\libmp3lame-static.lib
#LIBS += C:\\Users\\ciobi\\temp\\lame-3.99.5-03\\lame-3.99.5\\output\\Release\\libmpghip-static.lib
#
# A dir for LAME's include should be created and inside it there should be a "lame" dir and inside it a copy of "lame.h", then the parent of "lame" should be added to the path. The reason is that the #include is for "<lame/lame.h>"
#INCLUDEPATH += C:\\Users\\ciobi\\temp\\lame-include
#


TRANSLATIONS = translations/mp3diags_cs.ts \
    translations/mp3diags_de_DE.ts \
    translations/mp3diags_fr_FR.ts



