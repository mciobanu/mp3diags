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
Windows and for several other operating systems, including major Linux distributions. More details can be found in the
[main download page](https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html).


When building from sources, the build instructions for official releases are outdated, but can be used as a starting point. They are:
* [Building on Windows](https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceWindows) (see below
  about using `CMake`, though, as it works better, at least on Windows 10, where it works out of the box)
* [Building on Linux / others](https://mp3diags.sourceforge.net/unstable/010_getting_the_program.html#sourceGeneric)

Support for `CMake` was added in October 2023, but it's not thoroughly tested, and not yet part of an official release,
so it's only available when cloning the repository at [GitHub](https://github.com/mciobanu/mp3diags/tree/master)
or [SourceForge](https://sourceforge.net/p/mp3diags/code-git/ci/master/tree/).
Build instructions can be seen at
[htmlpreview.github.io](https://htmlpreview.github.io/?https://github.com/mciobanu/mp3diags/blob/master/doc/html/010_getting_the_program.html)
for now. They will be moved back to SourceForge once a new official release is created.

**Important note:** If building on Windows or if using `CMake`, get the latest code from git (either
`BuildMp3Diags.hta` or `CMakeLists.txt`), as there have been some fixes to the build files after `MP3Diags.tar.gz`
and `MP3Diags-1.4.01.tar.gz` were built.

[comment]: <> (ttt9 remove in next version)

**Note**: No build was attempted on Windows 11 yet, but `CMake` is expected to work.
