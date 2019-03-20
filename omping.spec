Name: omping
Version: 0.0.4
Release: 2%{?dist}
Summary: Utility to test IP multicast functionality
License: ISC
URL: https://github.com/jfriesse/omping
Source0: https://github.com/jfriesse/%{name}/releases/download/%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc

%description
Omping (Open Multicast Ping) is tool to test IP multicast functionality
primarily in local network.

%prep
%setup -q

%build
%set_build_flags
make %{?_smp_mflags}

%install
make DESTDIR="%{buildroot}" PREFIX="%{_prefix}" install

%files
%doc AUTHORS
%license COPYING
%{_bindir}/%{name}
%{_mandir}/man8/*

%changelog
* Wed Mar 20 2019 Jan Friesse <jfriesse@redhat.com> - 0.0.4-2
- Use license and set_build_flags macro
- Remove deprecated Group and Buildroot options
- Remove deprecated clean section
- Remove unneeded defattr

* Mon Jun 22 2011 Jan Friesse <jfriesse@redhat.com> - 0.0.4-1
- Update to version 0.0.4

* Mon May 02 2011 Jan Friesse <jfriesse@redhat.com> - 0.0.3-1
- Update to version 0.0.3

* Wed Nov 24 2010 Jan Friesse <jfriesse@redhat.com> - 0.0.1-2
- Change hard coded prefix path to macro

* Fri Nov 19 2010 Jan Friesse <jfriesse@redhat.com> - 0.0.1-1
- Initial package for Fedora
