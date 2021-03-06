Summary: Network Performance Testing Tool
Name: netperf
Version: 2.4.3
Release: 1
Copyright: Unknown
Group: System Environment/Base
URL: http://www.netperf.org/
Packager: Martin A. Brown
Source: ftp://ftp.netperf.org/netperf/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Prefix: /usr

%description
Many different network benchmarking tools are collected in this package,
maintained by Rick Jones of HP.

%prep
%setup

%build
#%patch0 -p1
./configure \
  --prefix=%{_prefix} \
  --mandir=%{_mandir} \
  --infodir=%{_infodir}
make

%install
test "$RPM_BUILD_ROOT" = "/"    || rm -rf $RPM_BUILD_ROOT
make DESTDIR=${RPM_BUILD_ROOT} install

# -- .svn directory only needed by developers; blowing it away
#    in our BUILD/ directory, so that we do not package it
#
rm -rf doc/examples/.svn

%clean
test "$RPM_BUILD_ROOT" = "/"    || rm -rf $RPM_BUILD_ROOT

# %post

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog INSTALL COPYING
%doc README.* Release_Notes
%doc doc/examples
%{_mandir}/man1/*
%{_infodir}/*
%{_bindir}/netperf
%{_bindir}/netserver


%changelog
* Sat Jun 17 2006 Martin A. Brown <martin@linux-ip.net>
- initial contributed specfile for netperf package (v2.4.2)
