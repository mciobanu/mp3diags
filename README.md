MP3 Diags
---------

MP3 Diags finds problems in MP3 files and helps the user fix many of them. It looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3). It has a tag editor, which can download album information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track information can also be extracted from a file's name. Another component is the file renamer, which can rename files based on the fields in their ID3V2 tag (artist, track number, album, genre, etc.).

For more detailed information visit http://mp3diags.sf.net/ and https://mp3diags.blogspot.com/

Installation
------------

In most cases it's easier to use pre-built binaries, which are available for
Windows and for several major Linux distributions from the main download page:
    http://mp3diags.sourceforge.net/unstable/010_getting_the_program.html

Build instructions for Windows are at:
    http://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceWindows

Build instructions for Linux / others are at:
    http://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceGeneric

Basically, you should run BuildMp3Diags.hta on Windows and Install.sh elsewhere.

Note that even if there is a CMakeLists.txt, CMake isn't officially supported.
If it works for you – fine. If it doesn't – you're on your own.
