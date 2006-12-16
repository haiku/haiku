Name: wget
Version: 1.7
Release: 1
Copyright: GPL
Source: ftp://ftp.gnu.org/gnu/wget/wget-%{version}.tar.gz 
Url: http://sunsite.dk/wget/
Provides: webclient
Prereq: /sbin/install-info
BuildRoot: /var/tmp/%{name}-root

Group: Applications/Internet
Group(cs): Aplikace/Internet
Summary: A utility for retrieving files using the HTTP or FTP protocols.
Summary(cs): Nástroj pro stahování souborù pomocí protokolù HTTP nebo FTP.

%description
GNU Wget is a free network utility to retrieve files from the World
Wide Web using HTTP and FTP protocols. It works non-interactively,
thus enabling work in the background, after having logged off.
Wget supports recursive retrieval of HTML pages, as well as FTP sites.
Wget supports proxy servers, which can lighten the network load, speed
up retrieval and provide access behind firewalls.

It works exceedingly well also on slow or unstable connections,
keeping getting the document until it is fully retrieved. Re-getting
files from where it left off works on servers (both HTTP and FTP) that
support it. Matching of wildcards and recursive mirroring of
directories are available when retrieving via FTP.  Both HTTP and FTP
retrievals can be time-stamped, thus Wget can see if the remote file
has changed since last retrieval and automatically retrieve the new
version if it has.

Install wget if you need to retrieve large numbers of files with HTTP or
FTP, or if you need a utility for mirroring web sites or FTP directories.

%description -l cs


%prep
%setup -q

%build
%configure --sysconfdir=/etc
make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall
gzip $RPM_BUILD_ROOT%{_infodir}/*

%post
/sbin/install-info %{_infodir}/wget.info.gz %{_infodir}/dir

%preun
if [ "$1" = 0 ]; then
    /sbin/install-info --delete %{_infodir}/wget.info.gz %{_infodir}/dir
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS MAILING-LIST NEWS README INSTALL doc/ChangeLog doc/sample.wgetrc
%config /etc/wgetrc
%{_bindir}/wget
%{_infodir}/*
/usr/share/locale/*/LC_MESSAGES/*

%changelog
* Wed Jan  3 2001 Jan Prikryl <prikryl@cg.tuwien.ac.at>
- preliminary version for 1.7
- removed all RedHat patches from 1.5.3 for this moment

* Tue Aug  1 2000 Bill Nottingham <notting@redhat.com>
- setlocale for LC_CTYPE too, or else all the translations think their
  characters are unprintable.

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Sun Jun 11 2000 Bill Nottingham <notting@redhat.com>
- build in new environment

* Mon Jun  5 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- FHS compliance

* Thu Feb  3 2000 Bill Nottingham <notting@redhat.com>
- handle compressed man pages

* Thu Aug 26 1999 Jeff Johnson <jbj@redhat.com>
- don't permit chmod 777 on symlinks (#4725).

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 4)

* Fri Dec 18 1998 Bill Nottingham <notting@redhat.com>
- build for 6.0 tree
- add Provides

* Sat Oct 10 1998 Cristian Gafton <gafton@redhat.com>
- strip binaries
- version 1.5.3

* Sat Jun 27 1998 Jeff Johnson <jbj@redhat.com>
- updated to 1.5.2

* Thu Apr 30 1998 Cristian Gafton <gafton@redhat.com>
- modified group to Applications/Networking

* Wed Apr 22 1998 Cristian Gafton <gafton@redhat.com>
- upgraded to 1.5.0
- they removed the man page from the distribution (Duh!) and I added it back
  from 1.4.5. Hey, removing the man page is DUMB!

* Fri Nov 14 1997 Cristian Gafton <gafton@redhat.com>
- first build against glibc
