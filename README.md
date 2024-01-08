MP3 Diags
---------

MP3 Diags finds problems in MP3 files and helps the user fix many of them. It looks at both the audio part (VBR info,
quality, normalization) and the tags containing track information (ID3). It has a tag editor, which can download album
information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track
information can also be extracted from a file's name. Another component is the file renamer, which can rename files
based on the fields in their ID3V2 tag (artist, track number, album, genre, etc.).

For more detailed information visit https://mp3diags.sf.net/unstable and https://mp3diags.blogspot.com/

Installation
------------

In most cases it's easier to use pre-built binaries, which are available for
Windows and for several other operating systems, including major Linux distributions. 

Still, building from sources shouldn't be a huge deal.

The details for the official release can be found in the
[main download page](https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html), 
while the build instructions for the code in the git repository can be seen at
[htmlpreview.github.io](https://htmlpreview.github.io/?https://github.com/mciobanu/mp3diags/blob/master/doc/html/010_getting_the_program.html).

**Note**: The Windows build is currently done on Windows 10. 
No build was attempted on Windows 11 yet, but `CMake` is expected to work.
