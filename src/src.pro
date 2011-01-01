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
 NormalizeDlgImpl.cpp \
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
  FullSizeImgDlg.cpp
TEMPLATE = app
CONFIG += warn_on \
	  thread \
          qt \
 debug_and_release
TARGET = MP3Diags
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
NormalizeDlgImpl.h \
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
 FullSizeImgDlg.h
FORMS += About.ui \
AlbumInfoDownloader.ui \
Config.ui \
Debug.ui \
DirFilter.ui \
DoubleListWdg.ui \
FileRenamer.ui \
ImageInfoPanel.ui \
MainForm.ui \
Normalize.ui \
NoteFilter.ui \
Palette.ui \
Patterns.ui \
Scan.ui \
SessionEditor.ui \
Sessions.ui \
TagEditor.ui \
ThreadRunner.ui \
 Export.ui



#CONFIG += console


#DEFINES += DISABLE_CHECK_FOR_UPDATES


QMAKE_CXXFLAGS_DEBUG += -DGENERATE_TOC_zz

LIBS += -lz \
  -lboost_serialization-mt

