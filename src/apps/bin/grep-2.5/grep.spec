%define beta %nil
%define rel 1
Summary: The GNU versions of grep pattern matching utilities.
Name: grep
Version: 2.5
%if "%{beta}" != ""
Release: 0.%{beta}.%{rel}
%else
Release: %{rel}
%endif
License: GPL
Group: Applications/Text
Source: ftp://ftp.gnu.org/pub/gnu/grep/grep-%{version}%{beta}.tar.bz2
Prefix: %{_prefix}
Prereq: /sbin/install-info
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires: pcre
Buildrequires: pcre-devel

%description
The GNU versions of commonly used grep utilities.  Grep searches
through textual input for lines which contain a match to a specified
pattern and then prints the matching lines.  GNU's grep utilities
include grep, egrep and fgrep.

You should install grep on your system, because it is a very useful
utility for searching through text.

%prep
%setup -q -n %{name}-%{version}%{beta}

%build
[ ! -e configure ] && ./autogen.sh
%configure --prefix=/usr --without-included-regex
make

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall LDFLAGS=-s prefix=${RPM_BUILD_ROOT}%{_prefix} exec_prefix=${RPM_BUILD_ROOT}
%ifos Linux
mkdir -p $RPM_BUILD_ROOT/bin
mv $RPM_BUILD_ROOT%{_prefix}/bin/* $RPM_BUILD_ROOT/bin
rm -rf $RPM_BUILD_ROOT%{_prefix}/bin
%endif
gzip -9f $RPM_BUILD_ROOT%{_infodir}/grep*

%find_lang %name

%clean
rm -rf ${RPM_BUILD_ROOT}

%post
[ -e %{_infodir}/grep.info.gz ] && /sbin/install-info --quiet --info-dir=%{_infodir} %{_infodir}/grep.info.gz || :

%preun
if [ $1 = 0 ]; then
	[ -e %{_infodir}/grep.info.gz ] && /sbin/install-info --quiet --info-dir=%{_infodir} --delete %{_infodir}/grep.info.gz
fi

%files -f %{name}.lang
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS THANKS TODO NEWS README ChangeLog

%ifos Linux
/bin/*
%else
%{_prefix}/bin/*
%endif
%{_infodir}/*.info.gz
%{_mandir}/*/*

%changelog
* Wed Mar 13 2002 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-1
- 2.5 final

* Wed Jan 23 2002 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-0.g.1
- 2.5g

* Wed Jan 09 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Mon Nov 19 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-0.f.4
- Update CVS to reduce bloat

* Thu Nov  8 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-0.f.3
- Don't fail %%post with --excludedocs

* Wed Sep 26 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-0.f.2
- Fix up echo A |grep '[A-Z0-9]' in locales other than C

* Tue Sep 25 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5-0.f.1
- 2.5f, fixes #53603

* Wed Jul 18 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.4.2-7
- Fix up the i18n patch - it used to break "grep '[]a]'" (#49003)
- revert to 2.4.2 (latest official release) for now

* Mon May 28 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5e-4
- Fix "echo Linux forever |grep -D skip Linux"

* Mon May 21 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5e-3
- Add new -D, --devices option
- Fix a bug with "directories" being uninitialized

* Sun May 13 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5e-2
- Fix up the --color option to behave like the one from ls (--color=auto)
  Sooner or later, some people will alias grep="grep --color" and wonder why
  their scripts break.
- Update docs accordingly
- Get rid of the annoying blinking in grep --color

* Sun May 13 2001 Bernhard Rosenkraenzer <bero@redhat.com> 2.5e-1
- 2.5e

* Tue Feb 27 2001 Trond Eivind Glomsrød <teg@redhat.com>
- use %%{_tmppath}
- langify

* Sun Aug 20 2000 Jakub Jelinek <jakub@redhat.com>
- i18n character ranges patch from Ulrich Drepper

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Mon Jun 19 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- FHSify

* Tue Mar 21 2000 Florian La Roche <Florian.LaRoche@redhat.com>
- update to 2.4.2
- fix download URL

* Thu Feb 03 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- gzip info pages (Bug #9035)

* Wed Feb 02 2000 Cristian Gafton <gafton@redhat.com>
- fix description

* Wed Dec 22 1999 Jeff Johnson <jbj@redhat.com>
- update to 2.4.

* Wed Oct 20 1999 Bill Nottingham <notting@redhat.com>
- prereq install-info

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 2)

* Mon Mar 08 1999 Preston Brown <pbrown@redhat.com>
- upgraded to grep 2.3, added install-info %post/%preun for info

* Wed Feb 24 1999 Preston Brown <pbrown@redhat.com>
- Injected new description and group.

* Sat May 09 1998 Prospector System <bugs@redhat.com>
- translations modified for de, fr, tr

* Fri May 01 1998 Cristian Gafton <gafton@redhat.com>
- updated to 2.2

* Thu Oct 16 1997 Donnie Barnes <djb@redhat.com>
- updated from 2.0 to 2.1
- spec file cleanups
- added BuildRoot

* Mon Jun 02 1997 Erik Troan <ewt@redhat.com>
- built against glibc
