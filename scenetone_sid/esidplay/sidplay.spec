Name: sidplay
Summary: music player and C64 SID chip emulator
Version: 1.36.10
%define version %{PACKAGE_VERSION}
Release: 1
Group: Applications/Sound
URL: http://www.geocities.com/SiliconValley/Lakes/5147/
Copyright: GPL (freely redistributable)
Source: sidplay-%{version}.tgz
Source2: HV_SIDS.FAQ
Source3: STIL.FAQ
Patch1: sidplay-1.36.2-bigendian.patch
Patch2: sidplay-1.36.2-x86.patch
BuildRoot: /tmp/sidplay-build

%description
This is a music player and SID chip emulator. With it you can listen to
more than 6000 musics from old and new C64 programs. The majority of
available musics is in the High Voltage SID Collection at:
http://www.dhp.com/~shark/c64music/

%package -n libsidplay
Summary: Shared library
Group: Libraries

%description -n libsidplay
Some people seem to like having everything shared.

%package -n libsidplay-devel
Summary: Header files and static library
Group: Libraries

%description -n libsidplay-devel
The header files and static library are required to compile applications
that use libsidplay.

%prep
%setup

cd console
cp -f audio/linux_audiodrv.h audiodrv.h
cp -f audio/linux_audiodrv.cpp audiodrv.cpp
cd ..

%ifarch i386
%patch2 -p1
%endif

%ifarch sparc
%patch1 -p1 -b .nobigendian
%endif

%ifarch ppc
%patch1 -p1 -b .nobigendian
%endif

%ifarch m68k
%patch1 -p1 -b .nobigendian
%endif

%build
cd libsidplay
make depend
make libsidplay.a libsidplay.so
cd ../console
make depend
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
cd console
make DESTPREFIX="$RPM_BUILD_ROOT/usr" install
cd ../libsidplay
make DESTPREFIX="$RPM_BUILD_ROOT/usr" install

%post -n libsidplay
ldconfig

%postun -n libsidplay
rm -f /usr/lib/libsidplay.so.1
rm -f /usr/lib/libsidplay.so.%{version}
ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc COPYING COPYRIGHT README.1st HV_SIDS.FAQ STIL.FAQ
/usr/bin/sidplay
/usr/bin/sid2wav

%files -n libsidplay
/usr/lib/libsidplay.so
/usr/lib/libsidplay.so.1
/usr/lib/libsidplay.so.%{version}

%files -n libsidplay-devel
/usr/include/sidplay
/usr/lib/libsidplay.a
