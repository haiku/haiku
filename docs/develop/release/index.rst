Release engineering
===================

To forge a successful stable release of the Haiku operating system, several important tasks must be accomplished. These steps are time tested as a best roadmap to draft a successful release of Haiku.

Important first steps
---------------------

* Review blockers for the next release in [Trac](https://dev.haiku-os.org)
* A minimum of 25% of the individuals in the [contributors group](https://review.haiku-os.org/admin/groups/23fa29f291e2dd5d41452202147d038f020fc8db,members) should reach concensus of the need for a stable release
    * Over time this may change to a time-based stable release schedule.
* Community nomination of a Release Coordinator
    * Should be someone from the contributors group
    * Should have visibility of most aspects of Haiku
    * Should have good coordination and communication skills
    * Generally occurs via the mailing list haiku-development
    * Timeline proposals are proposed via the haiku-development mailing list

Forming a timeline
------------------

An important aspect of drafting a release is forming a timeline.  The Release Coordinator's role is to drive Haiku towards this release date.

* Final date for enhancements in (RELEASE)
* Branch buildtools for (RELEASE)
* Branch haiku for (RELEASE)
* Setup CI/CD pipelines for (RELEASE)
* Generate first test candidates (TC0, TC1, etc), encourage extreme testing.
* Begin accepting bugfixes in branches via code review 
* Final translations synchronization
* Generate first release candidates (RC0, RC1, etc), encourage testing.
* Profit

* R1/Beta 2's timeline from branch to release was roughly 35 days
* R1/Beta 3's timeline from branch to release was roughly 50 days.

Release dates can slide, it's ok.
We just try to slide pragmatically (+1 week because of X,Y,Z)

.. toctree::

   milestones
