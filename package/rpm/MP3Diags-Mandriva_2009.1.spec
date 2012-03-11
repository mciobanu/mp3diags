Summary: Tool for finding and fixing problems in MP3 files; includes a tagger
%define version 0.99.0.1
%define branch test
License: http://www.gnu.org/licenses/gpl-2.0.html
Group: Applications/Multimedia
%define pkgName MP3Diags
%define translName mp3diags%{branch}

Name: MP3Diags%{branch}
#Prefix: /usr
#Provides: MP3Diags
Release: 1
Source: MP3Diags%{branch}-%{version}.tar.gz
URL: http://mp3diags.sourceforge.net/
Version: %{version}
BuildRoot: %{_tmppath}/%{name}-%{version}-build

Packager: Ciobi

#BuildRequires: libqt4-devel
#BuildRequires: boost-devel
# ??? ttt0


# this breaks the build for mandriva 2009.1: parseExpressionBoolean returns -1
#%if 0%{?mandriva_version} >= 2009
#%if 0%{?mdkversion} >= 200900
BuildRequires:  kdelibs4-devel
#BuildRequires:  libboost1.37.0-devel
#BuildRequires:  libboost-devel libboost-static-devel-1.38.0
BuildRequires:  boost-devel boost-static-devel
BuildRequires:  zlib-devel
Requires:       qt4-common
#%endif
# related but probably something else: https://bugzilla.novell.com/show_bug.cgi?id=459337  or  https://bugzilla.redhat.com/show_bug.cgi?id=456103



%description
Finds problems in MP3 files and helps the user to fix many of them. Looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3.)

Has a tag editor, which can download album information (including cover art) from MusicBrainz and Discogs, as well as paste data from the clipboard. Track information can also be extracted from a file's name.

Another component is the file renamer, which can rename files based on the fields in their ID3V2 tag (artist, track number, album, genre, ...)



%prep
%setup -q



%build

./AdjustMt.sh STATIC_SER


#%if 0%{?mandriva_version} > 2006
#export PATH=/usr/lib/qt4/bin:$PATH
#export QTDIR=%{_prefix}/lib/qt4/
#%endif


#%if 0%{?mandriva_version} >= 2009
#export PATH=/usr/lib/qt4/bin:$PATH
#export QTDIR=%{_prefix}/lib/qt4/
#ls /usr/lib/qt4/bin
#/usr/lib/qt4/bin/qmake
qmake
#%endif
lrelease src/src.pro


make
strip $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/bin/MP3Diags%{branch}

%install
# ttt1 perhaps look at http://doc.trolltech.com/4.3/qmake-variable-reference.html#installs and use INSTALLS += ...
echo mkdir $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_bindir}
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/bin/MP3Diags%{branch} $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags%{branch}.desktop $RPM_BUILD_ROOT%{_datadir}/applications
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags16%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags22%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/22x22/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags24%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags32%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags36%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/36x36/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags40%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/40x40/apps/MP3Diags%{branch}.png
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
cp $RPM_BUILD_DIR/MP3Diags%{branch}-%{version}/desktop/MP3Diags48%{branch}.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/MP3Diags%{branch}.png

mkdir -p %{buildroot}/usr/share/%{translName}/translations ; install -p -m644 src/translations/*.qm %{buildroot}/usr/share/%{translName}/translations

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
%dir /usr/share/%{translName}/translations
%{_bindir}/MP3Diags%{branch}
%{_datadir}/applications/MP3Diags%{branch}.desktop
%{_datadir}/icons/hicolor/16x16/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/22x22/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/24x24/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/32x32/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/36x36/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/40x40/apps/MP3Diags%{branch}.png
%{_datadir}/icons/hicolor/48x48/apps/MP3Diags%{branch}.png
/usr/share/%{translName}/translations/*.qm

#?datadir (=/usr/share)
#/usr/share/applications



%changelog
