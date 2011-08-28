Summary: Tool for finding and fixing problems in MP3 files; includes a tagger
%define version 0.99.0.1
%define branch test

%define pkgName MP3Diags
# pkgName should be mp3diags, MP3Diags, or whatever else
# !!! note that you can't simply comment a "define", as macros get expanded inside comments

%define srcBaseName MP3Diags%{branch}
# ttt1 perhaps have a binName and a dskName and use some file renaming and sed to control the name of the binary, desktop file, and icons (probably the same as the desktop file)

Name: %{pkgName}%{branch}
Version: %{version}
Release: 1
#Conflicts: MP3Diags >= 0.8.0.0
#Provides: MP3Diags
Group: Applications/Multimedia
Source: %{srcBaseName}-%{version}.tar.gz
URL: http://mp3diags.sourceforge.net/
License: http://www.gnu.org/licenses/gpl-2.0.html


BuildRoot: %{_tmppath}/%{name}-%{version}-build
Packager: Ciobi


%if 0%{?suse_version} > 0000
Requires: libqt4-x11
BuildRequires: zlib-devel boost-devel libqt4-devel
BuildRequires: update-desktop-files
%endif

%if 0%{?fedora} || 0%{?fedora_version}
Requires: qt-x11
BuildRequires: qt-devel zlib-devel boost-devel boost-devel-static gcc-c++
%endif

# this breaks the build for mandriva 2009.1: parseExpressionBoolean returns -1
%if 0%{?mandriva_version} >= 2009
#%if 0%{?mdkversion} >= 200900
BuildRequires:  kdelibs4-devel
BuildRequires:  boost-devel boost-static-devel
BuildRequires:  zlib-devel
Requires:       qt4-common
%endif
# related but probably something else: https://bugzilla.novell.com/show_bug.cgi?id=459337  or  https://bugzilla.redhat.com/show_bug.cgi?id=456103





%description
Finds problems in MP3 files and helps the user to fix many of them. Looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3.)

Has a tag editor, which can download album information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track information can also be extracted from a file's name.

Another component is the file renamer, which can rename files based on the fields in their ID3V2 tag (artist, track number, album, genre, ...)



%prep
%setup -q -n %{srcBaseName}-%{version}



%build

./AdjustMt.sh STATIC_SER

%if 0%{?suse_version}
qmake
%endif

%if 0%{?mandriva_version} >= 2009
qmake
%endif

%if 0%{?fedora} || 0%{?fedora_version}
qmake-qt4
%endif

make
strip $RPM_BUILD_DIR/%{srcBaseName}-%{version}/bin/%{srcBaseName}

%install
# ttt1 perhaps look at http://doc.trolltech.com/4.3/qmake-variable-reference.html#installs and use INSTALLS += ...
echo BUILD ROOT - %{buildroot}%{_bindir}

mkdir -p %{buildroot}%{_bindir} ; install -p -m755 bin/%{srcBaseName} %{buildroot}%{_bindir}

#mkdir -p %{buildroot}%{_datadir}/applications ; desktop-file-install --dir %{buildroot}%{_datadir}/applications desktop/%{srcBaseName}.desktop
mkdir -p %{buildroot}%{_datadir}/applications ; install -p -m644 desktop/%{srcBaseName}.desktop %{buildroot}%{_datadir}/applications/%{srcBaseName}.desktop

mkdir -p %{buildroot}%{_datadir}/icons/hicolor/16x16/apps ; install -p -m644 desktop/%{srcBaseName}16.png %{buildroot}%{_datadir}/icons/hicolor/16x16/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/22x22/apps ; install -p -m644 desktop/%{srcBaseName}22.png %{buildroot}%{_datadir}/icons/hicolor/22x22/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/24x24/apps ; install -p -m644 desktop/%{srcBaseName}24.png %{buildroot}%{_datadir}/icons/hicolor/24x24/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/32x32/apps ; install -p -m644 desktop/%{srcBaseName}32.png %{buildroot}%{_datadir}/icons/hicolor/32x32/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/36x36/apps ; install -p -m644 desktop/%{srcBaseName}36.png %{buildroot}%{_datadir}/icons/hicolor/36x36/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/40x40/apps ; install -p -m644 desktop/%{srcBaseName}40.png %{buildroot}%{_datadir}/icons/hicolor/40x40/apps/%{srcBaseName}.png
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/48x48/apps ; install -p -m644 desktop/%{srcBaseName}48.png %{buildroot}%{_datadir}/icons/hicolor/48x48/apps/%{srcBaseName}.png



%if 0%{?suse_version} > 0000
%suse_update_desktop_file -n %{srcBaseName}
#echo ================ SUSE ================ SUSE ================
%endif
#error with suse_update_desktop_file -in MP3Diags , perhaps try suse_update_desktop_file -n -i MP3Diags


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/*
%dir %{_datadir}/icons/hicolor/*/*
%{_bindir}/%{srcBaseName}
%{_datadir}/applications/%{srcBaseName}.desktop
%{_datadir}/icons/hicolor/*/apps/%{srcBaseName}.png

#?datadir (=/usr/share)
#/usr/share/applications



%changelog
