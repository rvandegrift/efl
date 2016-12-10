%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}
%{!?_rel:%{expand:%%global _rel 0.enl%{?dist}}}
%define _missing_doc_files_terminate_build 0

%if %(systemctl --version | head -1 | cut -d' ' -f2) >= 209
%{expand:%%global have_systemd 1}
%endif

%{expand:%%global ac_enable_systemd --%{?have_systemd:en}%{!?have_systemd:dis}able-systemd}

Summary: Enlightenment Foundation Libraries
Name: efl
Version: 1.18.4
Release: %{_rel}
License: LGPLv2.1 GPLv2.1 BSD
Group: System Environment/Libraries
URL: http://www.enlightenment.org/
Source: http://download.enlightenment.org/releases/%{name}-%{version}.tar.gz
Packager: %{?_packager:%{_packager}}%{!?_packager:The Enlightenment Project <enlightenment-devel@lists.sourceforge.net>}
Vendor: %{?_vendorinfo:%{_vendorinfo}}%{!?_vendorinfo:The Enlightenment Project (http://www.enlightenment.org/)}
Distribution: %{?_distribution:%{_distribution}}%{!?_distribution:%{_vendor}}
BuildRequires: libjpeg-devel, zlib-devel, giflib-devel
BuildRequires: fribidi-devel, mesa-libGL-devel
BuildRequires: libX11-devel, libXinerama-devel, libXrender-devel, libXScrnSaver-devel
Provides: eo = %{version}-%{release}
Obsoletes: eo < %{version}-%{release}
Provides: eina = %{version}-%{release}
Obsoletes: eina < %{version}-%{release}
Provides: eet = %{version}-%{release}
Obsoletes: eet < %{version}-%{release}
Provides: embryo = %{version}-%{release}
Obsoletes: embryo < %{version}-%{release}
Provides: evas = %{version}-%{release}
Obsoletes: evas < %{version}-%{release}
Provides: eio = %{version}-%{release}
Obsoletes: eio < %{version}-%{release}
Provides: ecore = %{version}-%{release}
Obsoletes: ecore < %{version}-%{release}
Provides: edje = %{version}-%{release}
Obsoletes: edje < %{version}-%{release}
Provides: elementary = %{version}-%{release}
Obsoletes: elementary < %{version}-%{release}
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
The Enlightenment Foundation Libraries are a collection of libraries
and tools upon which sophisticated graphical applications can be
built.  Included are a data structure library (Eina), a C-based object
engine (EO), a data storage library (EET), an object canvas (Evas),
and more.

%package devel
Summary: EFL headers, static libraries, documentation and test programs
Group: System Environment/Libraries
Requires: %{name} = %{version}
Provides: eo-devel = %{version}-%{release}
Obsoletes: eo-devel < %{version}-%{release}
Provides: eina-devel = %{version}-%{release}
Obsoletes: eina-devel < %{version}-%{release}
Provides: eet-devel = %{version}-%{release}
Obsoletes: eet-devel < %{version}-%{release}
Provides: embryo-devel = %{version}-%{release}
Obsoletes: embryo-devel < %{version}-%{release}
Provides: evas-devel = %{version}-%{release}
Obsoletes: evas-devel < %{version}-%{release}
Provides: eio-devel = %{version}-%{release}
Obsoletes: eio-devel < %{version}-%{release}
Provides: ecore-devel = %{version}-%{release}
Obsoletes: ecore-devel < %{version}-%{release}
Provides: edje-devel = %{version}-%{release}
Obsoletes: edje-devel < %{version}-%{release}
Provides: elementary-devel = %{version}-%{release}
Obsoletes: elementary-devel < %{version}-%{release}

%description devel
Headers, static libraries, test programs and documentation for EFL


%prep
%setup -q


%build
%{configure} --prefix=%{_prefix} %{ac_enable_systemd} CFLAGS="-O0 -ggdb3"
### use this if you have build problems
#./configure --prefix=%{_prefix} %{ac_enable_systemd} CFLAGS="-O0 -ggdb3"
%{__make} %{?_smp_mflags} %{?mflags}


%install
%{__make} %{?mflags_install} -j1 DESTDIR=$RPM_BUILD_ROOT install
find $RPM_BUILD_ROOT%{_prefix} -name '*.la' -print0 | xargs -0 rm -f

%{find_lang} %{name}


%clean
test "x$RPM_BUILD_ROOT" != "x/" && rm -rf $RPM_BUILD_ROOT


%post
/sbin/ldconfig


%postun
/sbin/ldconfig


%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS README NEWS COPYING licenses/COPYING.BSD licenses/COPYING.LGPL licenses/COPYING.GPL licenses/COPYING.FTL
%{_bindir}/*
%{_libdir}/*.so*
%{_libdir}/e*/
%{_datadir}/applications/*.desktop
%{_datadir}/dbus*/services/*
%{_datadir}/ecore*/
%{_datadir}/edje/
%{_datadir}/eeze/
%{_datadir}/efreet/
%{_datadir}/elementary/
%{_datadir}/elua/
%{_datadir}/embryo/
%{_datadir}/emotion/
%{_datadir}/eo/
%{_datadir}/ethumb*/
%{_datadir}/evas/
%{_datadir}/icons/*
%{_datadir}/gdb/auto-load%{_libdir}/lib*.py
%{_datadir}/mime/packages/*.xml
%if %{?have_systemd:1}0
%{_prefix}/lib/systemd/*/*.service
%endif

%files devel
%defattr(-, root, root)
%{_includedir}/*
%{_libdir}/cmake/*
%{_libdir}/pkgconfig/*
%{_datadir}/embryo/
%{_datadir}/eolian/
%{_datadir}/evas/


%changelog
