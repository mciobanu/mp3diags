Summary: Tool for finding and fixing problems in MP3 files; includes a tagger
%define version 0.99.0.1
License: http://www.gnu.org/licenses/gpl-2.0.html
Group: Applications/Multimedia
Name: MP3Diags
#Prefix: /usr
#Provides: MP3Diags
Release: 1
Source: MP3Diags-%{version}.tar.gz
URL: http://mp3diags
Version: %{version}
BuildRoot: %{_tmppath}/%{name}-%{version}-build

Packager: Ciobi

#BuildRequires: libqt4-devel
#BuildRequires: boost-devel
# ??? ttt0

%if 0%{?suse_version} > 0000
Requires: libqt4-x11
BuildRequires: zlib-devel boost-devel libqt4-devel
BuildRequires: update-desktop-files
%endif

%if 0%{?fedora} || 0%{?fedora_version}
Requires: qt-x11
#BuildRequires: qt-devel qt-config
BuildRequires: qt-devel zlib-devel boost-devel gcc-c++
%endif

%description
Finds problems in MP3 files and helps the user to fix many of them. Looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3.)

Has a tag editor, which can download album information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track information can also be extracted from a file's name.

Another component is the file renamer, which can rename files based on the fields in their ID3V2 tag (artist, track number, album, genre, ...)



%prep
%setup -q

#ttt0 echo ... > $RPM_BUILD_DIR/MP3Diags-%{version}/src

%build

%if 0%{?suse_version}
qmake
%endif

%if 0%{?fedora} || 0%{?fedora_version}
qmake-qt4
%endif

make
strip $RPM_BUILD_DIR/MP3Diags-%{version}/bin/MP3Diags

%install
echo mkdir $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_bindir}
cp $RPM_BUILD_DIR/MP3Diags-%{version}/bin/MP3Diags $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags.desktop $RPM_BUILD_ROOT%{_datadir}/applications
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags16.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags22.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags24.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags32.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags36.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags40.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps/MP3Diags.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags48.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/MP3Diags.png


#mkdir -p $RPM_BUILD_ROOT%{_bindir}
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/bin/MP3Diags $RPM_BUILD_ROOT%{_bindir}

#mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags.desktop $RPM_BUILD_ROOT%{_datadir}/applications

#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags16.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags22.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags24.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags32.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags36.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags40.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps/MP3Diags.png
#%dir $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
#cp $RPM_BUILD_DIR/MP3Diags-%{version}/desktop/MP3Diags48.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/MP3Diags.png



%if 0%{?suse_version} > 0000
%suse_update_desktop_file -n MP3Diags
#echo ================ SUSE ================ SUSE ================
%endif
#error with suse_update_desktop_file -in MP3Diags , perhaps try suse_update_desktop_file -n -i MP3Diags


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/16x16
%dir %{_datadir}/icons/hicolor/16x16/apps
%dir %{_datadir}/icons/hicolor/22x22
%dir %{_datadir}/icons/hicolor/22x22/apps
%dir %{_datadir}/icons/hicolor/24x24
%dir %{_datadir}/icons/hicolor/24x24/apps
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/36x36
%dir %{_datadir}/icons/hicolor/36x36/apps
%dir %{_datadir}/icons/hicolor/40x40
%dir %{_datadir}/icons/hicolor/40x40/apps
%dir %{_datadir}/icons/hicolor/48x48
%dir %{_datadir}/icons/hicolor/48x48/apps
%{_bindir}/MP3Diags
%{_datadir}/applications/MP3Diags.desktop
%{_datadir}/icons/hicolor/16x16/apps/MP3Diags.png
%{_datadir}/icons/hicolor/22x22/apps/MP3Diags.png
%{_datadir}/icons/hicolor/24x24/apps/MP3Diags.png
%{_datadir}/icons/hicolor/32x32/apps/MP3Diags.png
%{_datadir}/icons/hicolor/36x36/apps/MP3Diags.png
%{_datadir}/icons/hicolor/40x40/apps/MP3Diags.png
%{_datadir}/icons/hicolor/48x48/apps/MP3Diags.png

#?datadir (=/usr/share)
#/usr/share/applications



%changelog
