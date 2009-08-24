# -------------------------------------------------
# Project created by QtCreator 2009-05-24T08:17:27
# -------------------------------------------------
QT += svg \
    xml \
    network
TARGET = Mp3DiagsWindows
TEMPLATE = app
SOURCES += Version.cpp \
    UniqueNotesModel.cpp \
    Transformation.cpp \
    ThreadRunnerDlgImpl.cpp \
    TagWriter.cpp \
    TagReadPanel.cpp \
    TagEdtPatternsDlgImpl.cpp \
    TagEditorDlgImpl.cpp \
    StructuralTransformation.cpp \
    StreamsModel.cpp \
    StoredSettings.cpp \
    SongInfoParser.cpp \
    SessionsDlgImpl.cpp \
    SessionEditorDlgImpl.cpp \
    SerSupport.cpp \
    ScanDlgImpl.cpp \
    RenamerPatternsDlgImpl.cpp \
    PaletteDlgImpl.cpp \
    OsFile.cpp \
    NotesModel.cpp \
    Notes.cpp \
    NoteFilterDlgImpl.cpp \
    NormalizeDlgImpl.cpp \
    MusicBrainzDownloader.cpp \
    MultiLineTvDelegate.cpp \
    MpegStream.cpp \
    MpegFrame.cpp \
    Mp3TransformThread.cpp \
    Mp3Manip.cpp \
    MainFormDlgImpl.cpp \
    main.cpp \
    LyricsStream.cpp \
    LogModel.cpp \
    ImageInfoPanelWdgImpl.cpp \
    Id3V240Stream.cpp \
    Id3V230Stream.cpp \
    Id3V2Stream.cpp \
    Id3Transf.cpp \
    Helpers.cpp \
    FilesModel.cpp \
    FileRenamerDlgImpl.cpp \
    FileEnum.cpp \
    DoubleList.cpp \
    DiscogsDownloader.cpp \
    DirFilterDlgImpl.cpp \
    DebugDlgImpl.cpp \
    DataStream.cpp \
    ConfigDlgImpl.cpp \
    CommonTypes.cpp \
    CommonData.cpp \
    ColumnResizer.cpp \
    CheckedDir.cpp \
    ApeStream.cpp \
    AlbumInfoDownloaderDlgImpl.cpp \
    AboutDlgImpl.cpp \
    Widgets.cpp \
    fstream_unicode.cpp
HEADERS += Widgets.h \
    UniqueNotesModel.h \
    Transformation.h \
    ThreadRunnerDlgImpl.h \
    TagWriter.h \
    TagReadPanel.h \
    TagEdtPatternsDlgImpl.h \
    TagEditorDlgImpl.h \
    StructuralTransformation.h \
    StreamsModel.h \
    StoredSettings.h \
    SongInfoParser.h \
    SimpleSaxHandler.h \
    SessionsDlgImpl.h \
    SessionEditorDlgImpl.h \
    SerSupport.h \
    ScanDlgImpl.h \
    RenamerPatternsDlgImpl.h \
    PaletteDlgImpl.h \
    OsFile.h \
    NotesModel.h \
    Notes.h \
    NoteFilterDlgImpl.h \
    NormalizeDlgImpl.h \
    MusicBrainzDownloader.h \
    MultiLineTvDelegate.h \
    MpegStream.h \
    MpegFrame.h \
    Mp3TransformThread.h \
    Mp3Manip.h \
    MainFormDlgImpl.h \
    LyricsStream.h \
    LogModel.h \
    ImageInfoPanelWdgImpl.h \
    Id3V240Stream.h \
    Id3V230Stream.h \
    Id3V2Stream.h \
    Id3Transf.h \
    Helpers.h \
    FilesModel.h \
    FileRenamerDlgImpl.h \
    FileEnum.h \
    DoubleList.h \
    DiscogsDownloader.h \
    DirFilterDlgImpl.h \
    DebugDlgImpl.h \
    DataStream.h \
    ConfigDlgImpl.h \
    CommonTypes.h \
    CommonData.h \
    ColumnResizer.h \
    CheckedDir.h \
    ApeStream.h \
    AlbumInfoDownloaderDlgImpl.h \
    AboutDlgImpl.h \
    fstream_unicode.h
FORMS += TagEditor.ui \
    Sessions.ui \
    SessionEditor.ui \
    Scan.ui \
    Patterns.ui \
    Palette.ui \
    NoteFilter.ui \
    Normalize.ui \
    MainForm.ui \
    ImageInfoPanel.ui \
    FileRenamer.ui \
    DoubleListWdg.ui \
    DirFilter.ui \
    Debug.ui \
    Config.ui \
    AlbumInfoDownloader.ui \
    About.ui \
    ThreadRunner.ui
RESOURCES += Mp3Diags.qrc

INCLUDEPATH += C:/boost_1_39_0 C:/zlib123-dll/include

#LIBS += -L/path/to -lpsapi
LIBS += -LC:/boost_1_39_0/bin.v2/libs/serialization/build/gcc-mingw-3.4.5/release/threading-multi -lboost_serialization-mgw34-mt-1_39
LIBS += -LC:/zlib123-dll/lib -lzdll
LIBS += -lpsapi
