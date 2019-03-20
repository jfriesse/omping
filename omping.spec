Name: omping
Version: 0.0.4
Release: 2%{?dist}
Summary: Utility to test IP multicast functionality
Group: Applications/Internet
License: ISC
URL: https://github.com/jfriesse/omping
Source0: https://github.com/jfriesse/%{name}/releases/download/%{version}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Omping (Open Multicast Ping) is tool to test IP multicast functionality
primarily in local network.

%prep
%setup -q

%build
%set_build_flags
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR="%{buildroot}" PREFIX="%{_prefix}" install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc AUTHORS
%license COPYING
%{_bindir}/%{name}
%{_mandir}/man8/*

%changelog
* Wed Mar 20 2019 Jan Friesse <jfriesse@redhat.com> - 0.0.4-2
- Use license and set_build_flags macro

* Mon Jun 22 2011 Jan Friesse <jfriesse@redhat.com> - 0.0.4-1
- Update to version 0.0.4

* Mon May 02 2011 Jan Friesse <jfriesse@redhat.com> - 0.0.3-1
- Update to version 0.0.3

* Wed Nov 24 2010 Jan Friesse <jfriesse@redhat.com> - 0.0.1-2
- Change hard coded prefix path to macro

* Fri Nov 19 2010 Jan Friesse <jfriesse@redhat.com> - 0.0.1-1
- Initial package for Fedora
