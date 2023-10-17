MP3 Diags
---------

MP3 Diags finds problems in MP3 files and helps the user fix many of them. It looks at both the audio part (VBR info, 
quality, normalization) and the tags containing track information (ID3). It has a tag editor, which can download album 
information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track 
information can also be extracted from a file's name. Another component is the file renamer, which can rename files 
based on the fields in their ID3V2 tag (artist, track number, album, genre, etc.).

For more detailed information visit https://mp3diags.sf.net/ and https://mp3diags.blogspot.com/

Installation
------------

In most cases it's easier to use pre-built binaries, which are available for
Windows and for several major Linux distributions from the main download page 
or from those distributions:
    https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html

For official releases, build instructions are outdated, but can be used as a starting point. They are at:
  * https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceWindows, for Windows 
  * https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceGeneric, for Linux / others

Support for `CMake` was added, but it's not thoroughly tested, and not yet part of any official release. Instructions are at
[htmlpreview.github.io](https://htmlpreview.github.io/?https://github.com/mciobanu/mp3diags/blob/master/doc/html/010_getting_the_program.html).
but they will be moved back to SourceForge once a new official release is created.

For Windows 10 and 11, `CMake` is probably the better option to use, as the old build approach needs adjustments.
