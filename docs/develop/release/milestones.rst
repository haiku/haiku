Critical Milestones
===================

Community Concensus
-------------------

* Determine a release is needed
* Gain support in the community
* Raise release proposal on mailing list (haiku-development)
    * Elect Release Coordinator 
    * Plan timeline (which should be made up of milestones below)
    * Allow one week RFC time for comments

Scramle. Enhancement Deadline
-----------------------------

**Time:** ~ 1 week

* Roughly one week for people to finish up their enhancements for *(RELEASE)*
* Should be announced on the mailing lists
* Final ICU upgrades, webkit upgrades, etc.
* Plan for release logo used in installer (always fun, lots of bikeshed)
* Be working on release notes!

Branch
------

**Time:** ~ 1 week

* **Sysadmin** Branch *(RELEASE)*, haiku, buildtools
* **Sysadmin** Add *(RELEASE)* to CI/CD (Concourse)
* **Sysadmin** generate toolchain container for *(RELEASE)*
* **Sysadmin** begin Test Candidate builds for target architectures (x86_gcc2h, x86_64)
* **Sysadmin** Test Candidate (TC) builds should be clearly labeled and made available
* **Sysadmin** and **Release Coordinator** work in unison to merge bugfix patches to stable branches
    * **Sysadmin** focuses on merging build fixes, etc
    * **Release Coordinator** focuses on reviewing bug fixes
    * Communication is key!

Testing
-------

**Time:** ~ 2 weekes

* Agressively market Test Candidate builds for *(RELEASE)*
* Bugs opened on trac under *new* version. Squash bugs
* **Release Coordinator** should be downright obnoxious about people testing TC images!

Finalization
------------

**Time:** ~ 1 week

* Everything going well?  Produce Release Candidate images
* RC images all want to be *(RELEASE)*
* Any RC image can be renamed *(RELEASE)*
* Ensure release notes are almost done.
* MOAR Testing!

Distribution
------------

**Time:** ~ 1 week

* You now have the release images in hand! (RCX is secretly the release)
    * Keep it to yourself and don't tell people
* Generate Torrents, seed.  Get a few other people to seed.
* Place onto wasabi s3 under releases in final layout (be consistent!)
* Move to releases onto IPFS, pin and use pinning services
* Give mirrors time to... mirror via rsync
* Update website references.
    * Double check listed mirrors have release
    * Comment out any mirrors which don't have it (a few missing is fine)
    * Put release notes on proper place on website
* Release!
