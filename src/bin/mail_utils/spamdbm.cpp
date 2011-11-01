/******************************************************************************
 * $Id: spamdbm.cpp 30630 2009-05-05 01:31:01Z bga $
 *
 * This is a BeOS program for classifying e-mail messages as spam (unwanted
 * junk mail) or as genuine mail using a Bayesian statistical approach.  There
 * is also a Mail Daemon Replacement add-on to filter mail using the
 * classification statistics collected earlier.
 *
 * See also http://www.paulgraham.com/spam.html for a good writeup and
 * http://www.tuxedo.org/~esr/bogofilter/ for another implementation.
 * And more recently, Gary Robinson's write up of his improved algorithm
 * at http://radio.weblogs.com/0101454/stories/2002/09/16/spamDetection.html
 * which gives a better spread in spam ratios and slightly fewer
 * misclassifications.
 *
 * Note that this uses the AGMS vacation coding style, not the OpenTracker one.
 * That means no tabs, indents are two spaces, m_ is the prefix for member
 * variables, g_ is the prefix for global names, C style comments, constants
 * are in all capital letters and most other things are mixed case, it's word
 * wrapped to fit in 79 characters per line to make proofreading on paper
 * easier, and functions are listed in reverse dependency order so that forward
 * declarations (function prototypes with no code) aren't needed.
 *
 * The Original Design:
 * There is a spam database (just a file listing words and number of times they
 * were used in spam and non-spam messages) that a BeMailDaemon input filter
 * will use when scanning email.  It will mark the mail with the spam
 * probability (an attribute, optionally a mail header field) and optionally do
 * something if the probability exceeds a user defined level (delete message,
 * change subject, file in a different folder).  Or should that be a different
 * filter?  Outside the mail system, the probability can be used in queries to
 * find spam.
 *
 * A second user application will be used to update the database.  Besides
 * showing you the current list of words, you can drag and drop files to mark
 * them as spam or non-spam (a balanced binary tree is used internally to make
 * word storage fast).  It will add a second attribute to the files to show how
 * they have been classified by the user (and won't update the database if you
 * accidentally try to classify a file again).  Besides drag and drop, there
 * will be a command line interface and a message passing interface.  BeMail
 * (or other programs) will then communicate via messages to tell it when the
 * user marks a message as spam or not (via having separate delete spam /
 * delete genuine mail buttons and a menu item or two).
 *
 * Plus lots of details, like the rename swap method to update the database
 * file (so programs with the old file open aren't affected).  A nice tab text
 * format so you can open the database in a spreadsheet.  Startup and shutdown
 * control of the updater from BeMail.  Automatic creation of the indices
 * needed by the filter.  MIME types for the database file.  Icons for the app.
 * System settings to enable tracker to display the new attributes when viewing
 * e-mail (and maybe news articles if someone ever gets around to an NNTP as
 * files reader).  Documentation.  Recursive directory traversal for the
 * command line or directory drag and drop.  Options for the updater to warn or
 * ignore non-email files.  Etc.
 *
 * The Actual Implementation:
 * The spam database updates and the test for spam have been combined into one
 * program which runs as a server.  That way there won't be as long a delay
 * when the e-mail system wants to check for spam, because the database is
 * already loaded by the server and in memory.  The MDR mail filter add-on
 * simply sends scripting commands to the server (and starts it up if it isn't
 * already running).  The filter takes care of marking the messages when it
 * gets the rating back from the server, and then the rest of the mail system
 * rule chain can delete the message or otherwise manipulate it.
 *
 * Revision History (now manually updated due to SVN's philosophy)
 * $Log: spamdbm.cpp,v $
 * ------------------------------------------------------------------------
 * r15195 | agmsmith | 2005-11-27 21:07:55 -0500 (Sun, 27 Nov 2005) | 4 lines
 * Just a few minutes after checking in, I mentioned it to Japanese expert Koki
 * and he suggested also including the Japanese comma.  So before I forget to
 * do it...
 *
 * ------------------------------------------------------------------------
 * r15194 | agmsmith | 2005-11-27 20:37:13 -0500 (Sun, 27 Nov 2005) | 5 lines
 * Truncate overly long URLs to the maximum word length.  Convert Japanese
 * periods to spaces so that more "words" are found.  Fix UTF-8 comparison
 * problems with tolower() incorrectly converting characters with the high bit
 * set.
 *
 * r15098 | agmsmith | 2005-11-23 23:17:00 -0500 (Wed, 23 Nov 2005) | 5 lines
 * Added better tokenization so that HTML is parsed and things like tags
 * between letters of a word no longer hide that word.  After testing, the
 * result seems to be a tighter spread of ratings when done in full text plus
 * header mode.
 *
 * Revision 1.10  2005/11/24 02:08:39  agmsmith
 * Fixed up prefix codes, Z for things that are inside other things.
 *
 * Revision 1.9  2005/11/21 03:28:03  agmsmith
 * Added a function for extracting URLs.
 *
 * Revision 1.8  2005/11/09 03:36:18  agmsmith
 * Removed noframes detection (doesn't show up in e-mails).  Now use
 * just H for headers and Z for HTML tag junk.
 *
 * Revision 1.7  2005/10/24 00:00:08  agmsmith
 * Adding HTML tag removal, which also affected the search function so it
 * could search for single part things like &nbsp;.
 *
 * Revision 1.6  2005/10/17 01:55:08  agmsmith
 * Remove HTML comments and a few other similar things.
 *
 * Revision 1.5  2005/10/16 18:35:36  agmsmith
 * Under construction - looking into HTML not being in UTF-8.
 *
 * Revision 1.4  2005/10/11 01:51:21  agmsmith
 * Starting on the tokenising passes.  Still need to test asian truncation.
 *
 * Revision 1.3  2005/10/06 11:54:07  agmsmith
 * Not much.
 *
 * Revision 1.2  2005/09/12 01:49:37  agmsmith
 * Enable case folding for the whole file tokenizer.
 *
 * r13961 | agmsmith | 2005-08-13 22:25:28 -0400 (Sat, 13 Aug 2005) | 2 lines
 * Source code changes so that mboxtobemail now compiles and is in the build
 * system.
 *
 * r13959 | agmsmith | 2005-08-13 22:05:27 -0400 (Sat, 13 Aug 2005) | 2 lines
 * Rename the directory before doing anything else, otherwise svn dies badly.
 *
 * r13952 | agmsmith | 2005-08-13 15:31:42 -0400 (Sat, 13 Aug 2005) | 3 lines
 * Added the resources and file type associations, changed the application
 * signature and otherwise made the spam detection system work properly again.
 *
 * r13951 | agmsmith | 2005-08-13 11:40:01 -0400 (Sat, 13 Aug 2005) | 2 lines
 * Had to do the file rename as a separate operation due to SVN limitations.
 *
 * r13950 | agmsmith | 2005-08-13 11:38:44 -0400 (Sat, 13 Aug 2005) | 3 lines
 * Oops, "spamdb" is already used for a Unix package.  And spamdatabase is
 * already reserved by a domain name squatter.  Use "spamdbm" instead.
 *
 * r13949 | agmsmith | 2005-08-13 11:17:52 -0400 (Sat, 13 Aug 2005) | 3 lines
 * Renamed spamfilter to be the more meaningful spamdb (spam database) and
 * moved it into its own source directory in preparation for adding resources.
 *
 * r13628 | agmsmith | 2005-07-10 20:11:29 -0400 (Sun, 10 Jul 2005) | 3 lines
 * Updated keyword expansion to use SVN keywords.  Also seeing if svn is
 * working well enough for me to update files from BeOS R5.
 *
 * r11909 | axeld | 2005-03-18 19:09:19 -0500 (Fri, 18 Mar 2005) | 2 lines
 * Moved bin/ directory out of apps/.
 *
 * r11769 | bonefish | 2005-03-17 03:30:54 -0500 (Thu, 17 Mar 2005) | 1 line
 * Move trunk into respective module.
 *
 * r10362 | nwhitehorn | 2004-12-06 20:14:05 -0500 (Mon, 06 Dec 2004) | 2 lines
 * Fixed the spam filter so it works correctly now.
 *
 * r9934 | nwhitehorn | 2004-11-11 21:55:05 -0500 (Thu, 11 Nov 2004) | 2 lines
 * Added AGMS's excellent spam detection software.  Still some weirdness with
 * the configuration interface from E-mail prefs.
 *
 * Revision 1.2  2004/12/07 01:14:05  nwhitehorn
 * Fixed the spam filter so it works correctly now.
 *
 * Revision 1.87  2004/09/20 15:57:26  nwhitehorn
 * Mostly updated the tree to Be/Haiku style identifier naming conventions.  I
 * have a few more things to work out, mostly in mail_util.h, and then I'm
 * proceeding to jamify the build system.  Then we go into Haiku CVS.
 *
 * Revision 1.86  2003/07/26 16:47:46  agmsmith
 * Bug - wasn't allowing double classification if the user had turned on
 * the option to ignore the previous classification.
 *
 * Revision 1.85  2003/07/08 14:52:57  agmsmith
 * Fix bug with classification choices dialog box coming up with weird
 * sizes due to RefsReceived message coming in before ReadyToRun had
 * finished setting up the default sizes of the controls.
 *
 * Revision 1.84  2003/07/04 19:59:29  agmsmith
 * Now with a GUI option to let you declassify messages (set them back
 * to uncertain, rather than spam or genuine).  Required a BAlert
 * replacement since BAlerts can't do four buttons.
 *
 * Revision 1.83  2003/07/03 20:40:36  agmsmith
 * Added Uncertain option for declassifying messages.
 *
 * Revision 1.82  2003/06/16 14:57:13  agmsmith
 * Detect spam which uses mislabeled text attachments, going by the file name
 * extension.
 *
 * Revision 1.81  2003/04/08 20:27:04  agmsmith
 * AGMSBayesianSpamServer now shuts down immediately and returns true if
 * it is asked to quit by the registrar.
 *
 * Revision 1.80  2003/04/07 19:20:27  agmsmith
 * Ooops, int64 doesn't exist, use long long instead.
 *
 * Revision 1.79  2003/04/07 19:05:22  agmsmith
 * Now with Allen Brunson's atoll for PPC (you need the %Ld, but that
 * becomes %lld on other systems).
 *
 * Revision 1.78  2003/04/04 22:43:53  agmsmith
 * Fixed up atoll PPC processor hack so it would actually work, was just
 * returning zero which meant that it wouldn't load in the database file
 * (read the size as zero).
 *
 * Revision 1.77  2003/01/22 03:19:48  agmsmith
 * Don't convert words to lower case, the case is important for spam.
 * Particularly sentences which start with exciting words, which you
 * normally won't use at the start of a sentence (and thus capitalize).
 *
 * Revision 1.76  2002/12/18 02:29:22  agmsmith
 * Add space for the Uncertain display in Tracker.
 *
 * Revision 1.75  2002/12/18 01:54:37  agmsmith
 * Added uncertain sound effect.
 *
 * Revision 1.74  2002/12/13 23:53:12  agmsmith
 * Minimize the window before opening it so that it doesn't flash on the
 * screen in server mode.  Also load the database when the window is
 * displayed so that the user can see the words.
 *
 * Revision 1.73  2002/12/13 20:55:57  agmsmith
 * Documentation.
 *
 * Revision 1.72  2002/12/13 20:26:11  agmsmith
 * Fixed bug with adding messages in strings to database (was limited to
 * messages at most 1K long).  Also changed default server mode to true
 * since that's what people use most.
 *
 * Revision 1.71  2002/12/11 22:37:30  agmsmith
 * Added commands to train on spam and genuine e-mail messages passed
 * in string arguments rather then via external files.
 *
 * Revision 1.70  2002/12/10 22:12:41  agmsmith
 * Adding a message to the database now uses a BPositionIO rather than a
 * file and file name (for future string rather than file additions).  Also
 * now re-evaluate a file after reclassifying it so that the user can see
 * the new ratio.  Also remove the [Spam 99.9%] subject prefix when doing
 * a re-evaluation or classification (the number would be wrong).
 *
 * Revision 1.69  2002/12/10 01:46:04  agmsmith
 * Added the Chi-Squared scoring method.
 *
 * Revision 1.68  2002/11/29 22:08:25  agmsmith
 * Change default purge age to 2000 so that hitting the purge button
 * doesn't erase stuff from the new sample database.
 *
 * Revision 1.67  2002/11/25 20:39:39  agmsmith
 * Don't need to massage the MIME type since the mail library now does
 * the lower case conversion and converts TEXT to text/plain too.
 *
 * Revision 1.66  2002/11/20 22:57:12  nwhitehorn
 * PPC Compatibility Fixes
 *
 * Revision 1.65  2002/11/10 18:43:55  agmsmith
 * Added a time delay to some quitting operations so that scripting commands
 * from a second client (like a second e-mail account) will make the program
 * abort the quit operation.
 *
 * Revision 1.64  2002/11/05 18:05:16  agmsmith
 * Looked at Nathan's PPC changes (thanks!), modified style a bit.
 *
 * Revision 1.63  2002/11/04 03:30:22  nwhitehorn
 * Now works (or compiles at least) on PowerPC.  I'll get around to testing it
 * later.
 *
 * Revision 1.62  2002/11/04 01:03:33  agmsmith
 * Fixed warnings so it compiles under the bemaildaemon system.
 *
 * Revision 1.61  2002/11/03 23:00:37  agmsmith
 * Added to the bemaildaemon project on SourceForge.  Hmmmm, seems to switch to
 * a new version if I commit and specify a message, but doesn't accept the
 * message and puts up the text editor.  Must be a bug where cvs eats the first
 * option after "commit".
 *
 * Revision 1.60.1.1  2002/10/22 14:29:27  agmsmith
 * Needed to recompile with the original Libmail.so from Beta/1 since
 * the current library uses a different constructor, and thus wouldn't
 * run when used with the old library.
 *
 * Revision 1.60  2002/10/21 16:41:27  agmsmith
 * Return a special error code when no words are found in a message,
 * so that messages without text/plain parts can be recognized as
 * spam by the mail filter.
 *
 * Revision 1.59  2002/10/20 21:29:47  agmsmith
 * Watch out for MIME types of "text", treat as text/plain.
 *
 * Revision 1.58  2002/10/20 18:29:07  agmsmith
 * *** empty log message ***
 *
 * Revision 1.57  2002/10/20 18:25:02  agmsmith
 * Fix case sensitivity in MIME type tests, and fix text/any test.
 *
 * Revision 1.56  2002/10/19 17:00:10  agmsmith
 * Added the pop-up menu for the tokenize modes.
 *
 * Revision 1.55  2002/10/19 14:54:06  agmsmith
 * Fudge MIME type of body text components so that they get
 * treated as text.
 *
 * Revision 1.54  2002/10/19 00:56:37  agmsmith
 * The parsing of e-mail messages seems to be working now, just need
 * to add some user interface stuff for the tokenizing mode.
 *
 * Revision 1.53  2002/10/18 23:37:56  agmsmith
 * More mail kit usage, can now decode headers, but more to do.
 *
 * Revision 1.52  2002/10/16 23:52:33  agmsmith
 * Getting ready to add more tokenizing modes, exploring Mail Kit to break
 * apart messages into components (and decode BASE64 and other encodings).
 *
 * Revision 1.51  2002/10/11 20:05:31  agmsmith
 * Added installation of sound effect names, which the filter will use.
 *
 * Revision 1.50  2002/10/02 16:50:02  agmsmith
 * Forgot to add credits to the algorithm inventors.
 *
 * Revision 1.49  2002/10/01 00:39:29  agmsmith
 * Added drag and drop to evaluate files or to add them to the list.
 *
 * Revision 1.48  2002/09/30 19:44:17  agmsmith
 * Switched to Gary Robinson's method, removed max spam/genuine word.
 *
 * Revision 1.47  2002/09/23 17:08:55  agmsmith
 * Add an attribute with the spam ratio to files which have been evaluated.
 *
 * Revision 1.46  2002/09/23 02:50:32  agmsmith
 * Fiddling with display width of e-mail attributes.
 *
 * Revision 1.45  2002/09/23 01:13:56  agmsmith
 * Oops, bug in string evaluation scripting.
 *
 * Revision 1.44  2002/09/22 21:00:55  agmsmith
 * Added EvaluateString so that the BeMail add-on can pass the info without
 * having to create a temporary file.
 *
 * Revision 1.43  2002/09/20 19:56:02  agmsmith
 * Added about box and button for estimating the spam ratio of a file.
 *
 * Revision 1.42  2002/09/20 01:22:26  agmsmith
 * More testing, decide that an extreme ratio bias point of 0.5 is good.
 *
 * Revision 1.41  2002/09/19 21:17:12  agmsmith
 * Changed a few names and proofread the program.
 *
 * Revision 1.40  2002/09/19 14:27:17  agmsmith
 * Rearranged execution of commands, moving them to a separate looper
 * rather than the BApplication, so that thousands of files could be
 * processed without worrying about the message queue filling up.
 *
 * Revision 1.39  2002/09/18 18:47:16  agmsmith
 * Stop flickering when the view is partially obscured, update cached
 * values in all situations except when app is busy.
 *
 * Revision 1.38  2002/09/18 18:08:11  agmsmith
 * Add a function for evaluating the spam ratio of a message.
 *
 * Revision 1.37  2002/09/16 01:30:16  agmsmith
 * Added Get Oldest command.
 *
 * Revision 1.36  2002/09/16 00:47:52  agmsmith
 * Change the display to counter-weigh the spam ratio by the number of
 * messages.
 *
 * Revision 1.35  2002/09/15 20:49:35  agmsmith
 * Scrolling improved, buttons, keys and mouse wheel added.
 *
 * Revision 1.34  2002/09/15 03:46:10  agmsmith
 * Up and down buttons under construction.
 *
 * Revision 1.33  2002/09/15 02:09:21  agmsmith
 * Took out scroll bar.
 *
 * Revision 1.32  2002/09/15 02:05:30  agmsmith
 * Trying to add a scroll bar, but it isn't very useful.
 *
 * Revision 1.31  2002/09/14 23:06:28  agmsmith
 * Now has live updates of the list of words.
 *
 * Revision 1.30  2002/09/14 19:53:11  agmsmith
 * Now with a better display of the words.
 *
 * Revision 1.29  2002/09/13 21:33:54  agmsmith
 * Now draws the words in the word display view, but still primitive.
 *
 * Revision 1.28  2002/09/13 19:28:02  agmsmith
 * Added display of most genuine and most spamiest, fixed up cursor.
 *
 * Revision 1.27  2002/09/13 03:08:42  agmsmith
 * Show current word and message counts, and a busy cursor.
 *
 * Revision 1.26  2002/09/13 00:00:08  agmsmith
 * Fixed up some deadlock problems, now using asynchronous message replies.
 *
 * Revision 1.25  2002/09/12 17:56:58  agmsmith
 * Keep track of words which are spamiest and genuinest.
 *
 * Revision 1.24  2002/09/12 01:57:10  agmsmith
 * Added server mode.
 *
 * Revision 1.23  2002/09/11 23:30:45  agmsmith
 * Added Purge button and ignore classification checkbox.
 *
 * Revision 1.22  2002/09/11 21:23:13  agmsmith
 * Added bulk update choice, purge button, moved to a BView container
 * for all the controls (so background colour could be set, and Pulse
 * works normally for it too).
 *
 * Revision 1.21  2002/09/10 22:52:49  agmsmith
 * You can now change the database name in the GUI.
 *
 * Revision 1.20  2002/09/09 14:20:42  agmsmith
 * Now can have multiple backups, and implemented refs received.
 *
 * Revision 1.19  2002/09/07 19:14:56  agmsmith
 * Added standard GUI measurement code.
 *
 * Revision 1.18  2002/09/06 21:03:03  agmsmith
 * Rearranging code to avoid forward references when adding a window class.
 *
 * Revision 1.17  2002/09/06 02:54:00  agmsmith
 * Added the ability to purge old words from the database.
 *
 * Revision 1.16  2002/09/05 00:46:03  agmsmith
 * Now adds spam to the database!
 *
 * Revision 1.15  2002/09/04 20:32:15  agmsmith
 * Read ahead a couple of letters to decode quoted-printable better.
 *
 * Revision 1.14  2002/09/04 03:10:03  agmsmith
 * Can now tokenize (break into words) a text file.
 *
 * Revision 1.13  2002/09/03 21:50:54  agmsmith
 * Count database command, set up MIME type for the database file.
 *
 * Revision 1.12  2002/09/03 19:55:54  agmsmith
 * Added loading and saving the database.
 *
 * Revision 1.11  2002/09/02 03:35:33  agmsmith
 * Create indices and set up attribute associations with the e-mail MIME type.
 *
 * Revision 1.10  2002/09/01 15:52:49  agmsmith
 * Can now delete the database.
 *
 * Revision 1.9  2002/08/31 21:55:32  agmsmith
 * Yet more scripting.
 *
 * Revision 1.8  2002/08/31 21:41:37  agmsmith
 * Under construction, with example code to decode a B_REPLY.
 *
 * Revision 1.7  2002/08/30 19:29:06  agmsmith
 * Combined loading and saving settings into one function.
 *
 * Revision 1.6  2002/08/30 02:01:10  agmsmith
 * Working on loading and saving settings.
 *
 * Revision 1.5  2002/08/29 23:17:42  agmsmith
 * More scripting.
 *
 * Revision 1.4  2002/08/28 00:40:52  agmsmith
 * Scripting now seems to work, at least the messages flow properly.
 *
 * Revision 1.3  2002/08/25 21:51:44  agmsmith
 * Getting the about text formatting right.
 *
 * Revision 1.2  2002/08/25 21:28:20  agmsmith
 * Trying out the BeOS scripting system as a way of implementing the program.
 *
 * Revision 1.1  2002/08/24 02:27:51  agmsmith
 * Initial revision
 */

/* Standard C Library. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Standard C++ library. */

#include <iostream>

/* STL (Standard Template Library) headers. */

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

using namespace std;

/* BeOS (Be Operating System) headers. */

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Button.h>
#include <CheckBox.h>
#include <Cursor.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <fs_info.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Picture.h>
#include <PictureButton.h>
#include <Point.h>
#include <Polygon.h>
#include <PopUpMenu.h>
#include <PropertyInfo.h>
#include <RadioButton.h>
#include <Resources.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

/* Included from the Mail Daemon Replacement project (MDR) include/public
directory, available from http://sourceforge.net/projects/bemaildaemon/ */

#include <MailMessage.h>
#include <MailAttachment.h>


/******************************************************************************
 * Global variables, and not-so-variable things too.  Grouped by functionality.
 */

static float g_MarginBetweenControls; /* Space of a letter "M" between them. */
static float g_LineOfTextHeight;      /* Height of text the current font. */
static float g_StringViewHeight;      /* Height of a string view text box. */
static float g_ButtonHeight;          /* How many pixels tall buttons are. */
static float g_CheckBoxHeight;        /* Same for check boxes. */
static float g_RadioButtonHeight;     /* Also for radio buttons. */
static float g_PopUpMenuHeight;       /* Again for pop-up menus. */
static float g_TextBoxHeight;         /* Ditto for editable text controls. */

static const char *g_ABSAppSignature =
  "application/x-vnd.agmsmith.spamdbm";

static const char *g_ABSDatabaseFileMIMEType =
  "text/x-vnd.agmsmith.spam_probability_database";

static const char *g_DefaultDatabaseFileName =
  "SpamDBM Database";

static const char *g_DatabaseRecognitionString =
  "Spam Database File";

static const char *g_AttributeNameClassification = "MAIL:classification";
static const char *g_AttributeNameSpamRatio = "MAIL:ratio_spam";
static const char *g_BeepGenuine = "SpamFilter-Genuine";
static const char *g_BeepSpam = "SpamFilter-Spam";
static const char *g_BeepUncertain = "SpamFilter-Uncertain";
static const char *g_ClassifiedSpam = "Spam";
static const char *g_ClassifiedGenuine = "Genuine";
static const char *g_DataName = "data";
static const char *g_ResultName = "result";

static const char *g_SettingsDirectoryName = "Mail";
static const char *g_SettingsFileName = "SpamDBM Settings";
static const uint32 g_SettingsWhatCode = 'SDBM';
static const char *g_BackupSuffix = ".backup %d";
static const int g_MaxBackups = 10; /* Numbered from 0 to g_MaxBackups - 1. */
static const size_t g_MaxWordLength = 50; /* Words longer than this aren't. */
static const int g_MaxInterestingWords = 150; /* Top N words are examined. */
static const double g_RobinsonS = 0.45; /* Default weight for no data. */
static const double g_RobinsonX = 0.5; /* Halfway point for no data. */

static bool g_CommandLineMode;
  /* TRUE if the program was started from the command line (and thus should
  exit after processing the command), FALSE if it is running with a graphical
  user interface. */

static bool g_ServerMode;
  /* When TRUE the program runs in server mode - error messages don't result in
  pop-up dialog boxes, but you can still see them in stderr.  Also the window
  is minimized, if it exists. */

static int g_QuitCountdown = -1;
  /* Set to the number of pulse timing events (about one every half second) to
  count down before the program quits.  Negative means stop counting.  Zero
  means quit at the next pulse event.  This is used to keep the program alive
  for a short while after someone requests that it quit, in case more scripting
  commands come in, which will stop the countdown.  Needed to handle the case
  where there are multiple e-mail accounts all requesting spam identification,
  and one finishes first and tells the server to quit.  It also checks to see
  that there is no more work to do before trying to quit. */

static volatile bool g_AppReadyToRunCompleted = false;
  /* The BApplication starts processing messages before ReadyToRun finishes,
  which can lead to initialisation problems (button heights not determined).
  So wait for this to turn TRUE in code that might run early, like
  RefsReceived. */

static class CommanderLooper *g_CommanderLooperPntr = NULL;
static BMessenger *g_CommanderMessenger = NULL;
  /* Some globals for use with the looper which processes external commands
  (arguments received, file references received), needed for avoiding deadlocks
  which would happen if the BApplication sent a scripting message to itself. */

static BCursor *g_BusyCursor = NULL;
  /* The busy cursor, will be loaded from the resource file during application
  startup. */

typedef enum PropertyNumbersEnum
{
  PN_DATABASE_FILE = 0,
  PN_SPAM,
  PN_SPAM_STRING,
  PN_GENUINE,
  PN_GENUINE_STRING,
  PN_UNCERTAIN,
  PN_IGNORE_PREVIOUS_CLASSIFICATION,
  PN_SERVER_MODE,
  PN_FLUSH,
  PN_PURGE_AGE,
  PN_PURGE_POPULARITY,
  PN_PURGE,
  PN_OLDEST,
  PN_EVALUATE,
  PN_EVALUATE_STRING,
  PN_RESET_TO_DEFAULTS,
  PN_INSTALL_THINGS,
  PN_TOKENIZE_MODE,
  PN_SCORING_MODE,
  PN_MAX
} PropertyNumbers;

static const char * g_PropertyNames [PN_MAX] =
{
  "DatabaseFile",
  "Spam",
  "SpamString",
  "Genuine",
  "GenuineString",
  "Uncertain",
  "IgnorePreviousClassification",
  "ServerMode",
  "Flush",
  "PurgeAge",
  "PurgePopularity",
  "Purge",
  "Oldest",
  "Evaluate",
  "EvaluateString",
  "ResetToDefaults",
  "InstallThings",
  "TokenizeMode",
  "ScoringMode"
};

/* This array lists the scripting commands we can handle, in a format that the
scripting system can understand too. */

static struct property_info g_ScriptingPropertyList [] =
{
  /* *name; commands[10]; specifiers[10]; *usage; extra_data; ... */
  {g_PropertyNames[PN_DATABASE_FILE], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Get the pathname of the current database file.  "
    "The default name is something like B_USER_SETTINGS_DIRECTORY / "
    "Mail / SpamDBM Database", PN_DATABASE_FILE,
    {}, {}, {}},
  {g_PropertyNames[PN_DATABASE_FILE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Change the pathname of the database file to "
    "use.  It will automatically be converted to an absolute path name, "
    "so make sure the parent directories exist before setting it.  If it "
    "doesn't exist, you'll have to use the create command next.",
    PN_DATABASE_FILE, {}, {}, {}},
  {g_PropertyNames[PN_DATABASE_FILE], {B_CREATE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Creates a new empty database, will replace "
    "the existing database file too.", PN_DATABASE_FILE, {}, {}, {}},
  {g_PropertyNames[PN_DATABASE_FILE], {B_DELETE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Deletes the database file and all backup copies "
    "of that file too.  Really only of use for uninstallers.",
    PN_DATABASE_FILE, {}, {}, {}},
  {g_PropertyNames[PN_DATABASE_FILE], {B_COUNT_PROPERTIES, 0},
    {B_DIRECT_SPECIFIER, 0}, "Returns the number of words in the database.",
    PN_DATABASE_FILE, {}, {}, {}},
  {g_PropertyNames[PN_SPAM], {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
    "Adds the spam in the given file (specify full pathname to be safe) to "
    "the database.  The words in the files will be added to the list of words "
    "in the database that identify spam messages.  The files processed will "
    "also have the attribute MAIL:classification added with a value of "
    "\"Spam\" or \"Genuine\" as specified.  They also have their spam ratio "
    "attribute updated, as if you had also used the Evaluate command on "
    "them.  If they already have the MAIL:classification "
    "attribute and it matches the new classification then they won't get "
    "processed (and if it is different, they will get removed from the "
    "statistics for the old class and added to the statistics for the new "
    "one).  You can turn off that behaviour with the "
    "IgnorePreviousClassification property.  The command line version lets "
    "you specify more than one pathname.", PN_SPAM, {}, {}, {}},
  {g_PropertyNames[PN_SPAM], {B_COUNT_PROPERTIES, 0}, {B_DIRECT_SPECIFIER, 0},
    "Returns the number of spam messages in the database.", PN_SPAM,
    {}, {}, {}},
  {g_PropertyNames[PN_SPAM_STRING], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Adds the spam in the given string (assumed to "
    "be the text of a whole e-mail message, not just a file name) to the "
    "database.", PN_SPAM_STRING, {}, {}, {}},
  {g_PropertyNames[PN_GENUINE], {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
    "Similar to adding spam except that the message file is added to the "
    "genuine statistics.", PN_GENUINE, {}, {}, {}},
  {g_PropertyNames[PN_GENUINE], {B_COUNT_PROPERTIES, 0},
    {B_DIRECT_SPECIFIER, 0}, "Returns the number of genuine messages in the "
    "database.", PN_GENUINE, {}, {}, {}},
  {g_PropertyNames[PN_GENUINE_STRING], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Adds the genuine message in the given string "
    "(assumed to be the text of a whole e-mail message, not just a file name) "
    "to the database.", PN_GENUINE_STRING, {}, {}, {}},
  {g_PropertyNames[PN_UNCERTAIN], {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
    "Similar to adding spam except that the message file is removed from the "
    "database, undoing the previous classification.  Obviously, it needs to "
    "have been classified previously (using the file attributes) so it can "
    "tell if it is removing spam or genuine words.", PN_UNCERTAIN, {}, {}, {}},
  {g_PropertyNames[PN_IGNORE_PREVIOUS_CLASSIFICATION], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "If set to true then the previous classification "
    "(which was saved as an attribute of the e-mail message file) will be "
    "ignored, so that you can add the message to the database again.  If set "
    "to false (the normal case), the attribute will be examined, and if the "
    "message has already been classified as what you claim it is, nothing "
    "will be done.  If it was misclassified, then the message will be removed "
    "from the statistics for the old class and added to the stats for the "
    "new classification you have requested.",
    PN_IGNORE_PREVIOUS_CLASSIFICATION, {}, {}, {}},
  {g_PropertyNames[PN_IGNORE_PREVIOUS_CLASSIFICATION], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Find out the current setting of the flag for "
    "ignoring the previously recorded classification.",
    PN_IGNORE_PREVIOUS_CLASSIFICATION, {}, {}, {}},
  {g_PropertyNames[PN_SERVER_MODE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "If set to true then error messages get printed "
    "to the standard error stream rather than showing up in an alert box.  "
    "It also starts up with the window minimized.", PN_SERVER_MODE,
    {}, {}, {}},
  {g_PropertyNames[PN_SERVER_MODE], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Find out the setting of the server mode flag.",
    PN_SERVER_MODE, {}, {}, {}},
  {g_PropertyNames[PN_FLUSH], {B_EXECUTE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Writes out the database file to disk, if it has "
    "been updated in memory but hasn't been saved to disk.  It will "
    "automatically get written when the program exits, so this command is "
    "mostly useful for server mode.", PN_FLUSH, {}, {}, {}},
  {g_PropertyNames[PN_PURGE_AGE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Sets the old age limit.  Words which haven't "
      "been updated since this many message additions to the database may be "
      "deleted when you do a purge.  A good value is 1000, meaning that if a "
      "word hasn't appeared in the last 1000 spam/genuine messages, it will "
      "be forgotten.  Zero will purge all words, 1 will purge words not in "
      "the last message added to the database, 2 will purge words not in the "
      "last two messages added, and so on.  This is mostly useful for "
      "removing those one time words which are often hunks of binary garbage, "
      "not real words.  This acts in combination with the popularity limit; "
      "both conditions have to be valid before the word gets deleted.",
      PN_PURGE_AGE, {}, {}, {}},
  {g_PropertyNames[PN_PURGE_AGE], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Gets the old age limit.", PN_PURGE_AGE,
    {}, {}, {}},
  {g_PropertyNames[PN_PURGE_POPULARITY], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Sets the popularity limit.  Words which aren't "
    "this popular may be deleted when you do a purge.  A good value is 5, "
    "which means that the word is safe from purging if it has been seen in 6 "
    "or more e-mail messages.  If it's only in 5 or less, then it may get "
    "purged.  The extreme is zero, where only words that haven't been seen "
    "in any message are deleted (usually means no words).  This acts in "
    "combination with the old age limit; both conditions have to be valid "
    "before the word gets deleted.", PN_PURGE_POPULARITY, {}, {}, {}},
  {g_PropertyNames[PN_PURGE_POPULARITY], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Gets the purge popularity limit.",
    PN_PURGE_POPULARITY, {}, {}, {}},
  {g_PropertyNames[PN_PURGE], {B_EXECUTE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Purges the old obsolete words from the "
    "database, if they are old enough according to the age limit and also "
    "unpopular enough according to the popularity limit.", PN_PURGE,
    {}, {}, {}},
  {g_PropertyNames[PN_OLDEST], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Gets the age of the oldest message in the "
    "database.  It's relative to the beginning of time, so you need to do "
    "(total messages - age - 1) to see how many messages ago it was added.",
    PN_OLDEST, {}, {}, {}},
  {g_PropertyNames[PN_EVALUATE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Evaluates a given file (by path name) to see "
    "if it is spam or not.  Returns the ratio of spam probability vs genuine "
    "probability, 0.0 meaning completely genuine, 1.0 for completely spam.  "
    "Normally you should safely be able to consider it as spam if it is over "
    "0.56 for the Robinson scoring method.  For the ChiSquared method, the "
    "numbers are near 0 for genuine, near 1 for spam, and anywhere in the "
    "middle means it can't decide.  The program attaches a MAIL:ratio_spam "
    "attribute with the ratio as its "
    "float32 value to the file.  Also returns the top few interesting words "
    "in \"words\" and the associated per-word probability ratios in "
    "\"ratios\".", PN_EVALUATE, {}, {}, {}},
  {g_PropertyNames[PN_EVALUATE_STRING], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Like Evaluate, but rather than a file name, "
    "the string argument contains the entire text of the message to be "
    "evaluated.", PN_EVALUATE_STRING, {}, {}, {}},
  {g_PropertyNames[PN_RESET_TO_DEFAULTS], {B_EXECUTE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Resets all the configuration options to the "
    "default values, including the database name.", PN_RESET_TO_DEFAULTS,
    {}, {}, {}},
  {g_PropertyNames[PN_INSTALL_THINGS], {B_EXECUTE_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Creates indices for the MAIL:classification and "
    "MAIL:ratio_spam attributes on all volumes which support BeOS queries, "
    "identifies them to the system as e-mail related attributes (modifies "
    "the text/x-email MIME type), and sets up the new MIME type "
    "(text/x-vnd.agmsmith.spam_probability_database) for the database file.  "
    "Also registers names for the sound effects used by the separate filter "
    "program (use the installsound BeOS program or the Sounds preferences "
    "program to associate sound files with the names).", PN_INSTALL_THINGS,
    {}, {}, {}},
  {g_PropertyNames[PN_TOKENIZE_MODE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Sets the method used for breaking up the "
    "message into words.  Use \"Whole\" for the whole file (also use it for "
    "non-email files).  The file isn't broken into parts; the whole thing is "
    "converted into words, headers and attachments are just more raw data.  "
    "Well, not quite raw data since it converts quoted-printable codes "
    "(equals sign followed by hex digits or end of line) to the equivalent "
    "single characters.  \"PlainText\" breaks the file into MIME components "
    "and only looks at the ones which are of MIME type text/plain.  "
    "\"AnyText\" will look for words in all text/* things, including "
    "text/html attachments.  \"AllParts\" will decode all message components "
    "and look for words in them, including binary attachments.  "
    "\"JustHeader\" will only look for words in the message header.  "
    "\"AllPartsAndHeader\", \"PlainTextAndHeader\" and \"AnyTextAndHeader\" "
    "will also include the words from the message headers.", PN_TOKENIZE_MODE,
    {}, {}, {}},
  {g_PropertyNames[PN_TOKENIZE_MODE], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Gets the method used for breaking up the "
    "message into words.", PN_TOKENIZE_MODE, {}, {}, {}},
  {g_PropertyNames[PN_SCORING_MODE], {B_SET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Sets the method used for combining the "
    "probabilities of individual words into an overall score.  "
    "\"Robinson\" mode will use Gary Robinson's nth root of the product "
    "method.  It gives a nice range of values between 0 and 1 so you can "
    "see shades of spaminess.  The cutoff point between spam and genuine "
    "varies depending on your database of words (0.56 was one point in "
    "some experiments).  \"ChiSquared\" mode will use chi-squared "
    "statistics to evaluate the difference in probabilities that the lists "
    "of word ratios are random.  The result is very close to 0 for genuine "
    "and very close to 1 for spam, and near the middle if it is uncertain.",
    PN_SCORING_MODE, {}, {}, {}},
  {g_PropertyNames[PN_SCORING_MODE], {B_GET_PROPERTY, 0},
    {B_DIRECT_SPECIFIER, 0}, "Gets the method used for combining the "
    "individual word ratios into an overall score.", PN_SCORING_MODE,
    {}, {}, {}},
  {0, {0}, {0}, 0, 0, {}, {}, {}} /* End of list of property commands. */
};


/* The various scoring modes as text and enums.  See PN_SCORING_MODE. */

typedef enum ScoringModeEnum
{
  SM_ROBINSON = 0,
  SM_CHISQUARED,
  SM_MAX
} ScoringModes;

static const char * g_ScoringModeNames [SM_MAX] =
{
  "Robinson",
  "ChiSquared"
};


/* The various tokenizing modes as text and enums.  See PN_TOKENIZE_MODE. */

typedef enum TokenizeModeEnum
{
  TM_WHOLE = 0,
  TM_PLAIN_TEXT,
  TM_PLAIN_TEXT_HEADER,
  TM_ANY_TEXT,
  TM_ANY_TEXT_HEADER,
  TM_ALL_PARTS,
  TM_ALL_PARTS_HEADER,
  TM_JUST_HEADER,
  TM_MAX
} TokenizeModes;

static const char * g_TokenizeModeNames [TM_MAX] =
{
  "All",
  "Plain text",
  "Plain text and header",
  "Any text",
  "Any text and header",
  "All parts",
  "All parts and header",
  "Just header"
};


/* Possible message classifications. */

typedef enum ClassificationTypesEnum
{
  CL_GENUINE = 0,
  CL_SPAM,
  CL_UNCERTAIN,
  CL_MAX
} ClassificationTypes;

static const char * g_ClassificationTypeNames [CL_MAX] =
{
  g_ClassifiedGenuine,
  g_ClassifiedSpam,
  "Uncertain"
};


/* Some polygon graphics for the scroll arrows. */

static BPoint g_UpLinePoints [] =
{
  BPoint (8, 2 * (1)),
  BPoint (14, 2 * (6)),
  BPoint (10, 2 * (6)),
  BPoint (10, 2 * (13)),
  BPoint (6, 2 * (13)),
  BPoint (6, 2 * (6)),
  BPoint (2, 2 * (6))
};

static BPoint g_DownLinePoints [] =
{
  BPoint (8, 2 * (14-1)),
  BPoint (14, 2 * (14-6)),
  BPoint (10, 2 * (14-6)),
  BPoint (10, 2 * (14-13)),
  BPoint (6, 2 * (14-13)),
  BPoint (6, 2 * (14-6)),
  BPoint (2, 2 * (14-6))
};

static BPoint g_UpPagePoints [] =
{
  BPoint (8, 2 * (1)),
  BPoint (13, 2 * (6)),
  BPoint (10, 2 * (6)),
  BPoint (14, 2 * (10)),
  BPoint (10, 2 * (10)),
  BPoint (10, 2 * (13)),
  BPoint (6, 2 * (13)),
  BPoint (6, 2 * (10)),
  BPoint (2, 2 * (10)),
  BPoint (6, 2 * (6)),
  BPoint (3, 2 * (6))
};

static BPoint g_DownPagePoints [] =
{
  BPoint (8, 2 * (14-1)),
  BPoint (13, 2 * (14-6)),
  BPoint (10, 2 * (14-6)),
  BPoint (14, 2 * (14-10)),
  BPoint (10, 2 * (14-10)),
  BPoint (10, 2 * (14-13)),
  BPoint (6, 2 * (14-13)),
  BPoint (6, 2 * (14-10)),
  BPoint (2, 2 * (14-10)),
  BPoint (6, 2 * (14-6)),
  BPoint (3, 2 * (14-6))
};


/* An array of flags to identify characters which are considered to be spaces.
If character code X has g_SpaceCharacters[X] set to true then it is a
space-like character.  Character codes 128 and above are always non-space since
they are UTF-8 characters.  Initialised in the ABSApp constructor. */

static bool g_SpaceCharacters [128];



/******************************************************************************
 * Each word in the spam database gets one of these structures.  The database
 * has a string (the word) as the key and this structure as the value
 * (statistics for that word).
 */

typedef struct StatisticsStruct
{
  uint32 age;
    /* Sequence number for the time when this word was last updated in the
    database, so that we can remove old words (haven't been seen in recent
    spam).  It's zero for the first file ever added (spam or genuine) to the
    database, 1 for all words added or updated by the second file, etc.  If a
    later file updates an existing word, it gets the age of the later file. */

  uint32 genuineCount;
    /* Number of genuine messages that have this word. */

  uint32 spamCount;
    /* A count of the number of spam e-mail messages which contain the word. */

} StatisticsRecord, *StatisticsPointer;

typedef map<string, StatisticsRecord> StatisticsMap;
  /* Define this type which will be used for our main data storage facility, so
  we can more conveniently specify things that are derived from it, like
  iterators. */



/******************************************************************************
 * An alert box asking how the user wants to mark messages.  There are buttons
 * for each classification category, and a checkbox to mark all remaining N
 * messages the same way.  And a cancel button.  To use it, first create the
 * ClassificationChoicesWindow, specifying the input arguments.  Then call the
 * Go method which will show the window, stuff the user's answer into your
 * output arguments (class set to CL_MAX if the user cancels), and destroy the
 * window.  Implemented because BAlert only allows 3 buttons, max!
 */

class ClassificationChoicesWindow : public BWindow
{
public:
  /* Constructor and destructor. */
  ClassificationChoicesWindow (BRect FrameRect,
    const char *FileName, int NumberOfFiles);

  /* BeOS virtual functions. */
  virtual void MessageReceived (BMessage *MessagePntr);

  /* Our methods. */
  void Go (bool *BulkModeSelectedPntr,
    ClassificationTypes *ChoosenClassificationPntr);

  /* Various message codes for various buttons etc. */
  static const uint32 MSG_CLASS_BUTTONS = 'ClB0';
  static const uint32 MSG_CANCEL_BUTTON = 'Cncl';
  static const uint32 MSG_BULK_CHECKBOX = 'BlkK';

private:
  /* Member variables. */
  bool *m_BulkModeSelectedPntr;
  ClassificationTypes *m_ChoosenClassificationPntr;
};

class ClassificationChoicesView : public BView
{
public:
  /* Constructor and destructor. */
  ClassificationChoicesView (BRect FrameRect,
    const char *FileName, int NumberOfFiles);

  /* BeOS virtual functions. */
  virtual void AttachedToWindow ();
  virtual void GetPreferredSize (float *width, float *height);

private:
  /* Member variables. */
  const char *m_FileName;
  int         m_NumberOfFiles;
  float       m_PreferredBottomY;
};



/******************************************************************************
 * Due to deadlock problems with the BApplication posting scripting messages to
 * itself, we need to add a second Looper.  Its job is to just to convert
 * command line arguments and arguments from the Tracker (refs received) into a
 * series of scripting commands sent to the main BApplication.  It also prints
 * out the replies received (to stdout for command line replies).  An instance
 * of this class will be created and run by the main() function, and shut down
 * by it too.
 */

class CommanderLooper : public BLooper
{
public:
  CommanderLooper ();
  ~CommanderLooper ();
  virtual void MessageReceived (BMessage *MessagePntr);

  void CommandArguments (int argc, char **argv);
  void CommandReferences (BMessage *MessagePntr,
    bool BulkMode = false,
    ClassificationTypes BulkClassification = CL_GENUINE);
  bool IsBusy ();

private:
  void ProcessArgs (BMessage *MessagePntr);
  void ProcessRefs (BMessage *MessagePntr);

  static const uint32 MSG_COMMAND_ARGUMENTS = 'CArg';
  static const uint32 MSG_COMMAND_FILE_REFS = 'CRef';

  bool m_IsBusy;
};



/******************************************************************************
 * This view contains the various buttons and other controls for setting
 * configuration options and displaying the state of the database (but not the
 * actual list of words).  It will appear in the top half of the
 * DatabaseWindow.
 */

class ControlsView : public BView
{
public:
  /* Constructor and destructor. */
  ControlsView (BRect NewBounds);
  ~ControlsView ();

  /* BeOS virtual functions. */
  virtual void AttachedToWindow ();
  virtual void FrameResized (float Width, float Height);
  virtual void MessageReceived (BMessage *MessagePntr);
  virtual void Pulse ();

private:
  /* Various message codes for various buttons etc. */
  static const uint32 MSG_BROWSE_BUTTON = 'Brws';
  static const uint32 MSG_DATABASE_NAME = 'DbNm';
  static const uint32 MSG_ESTIMATE_BUTTON = 'Estm';
  static const uint32 MSG_ESTIMATE_FILE_REFS = 'ERef';
  static const uint32 MSG_IGNORE_CLASSIFICATION = 'IPCl';
  static const uint32 MSG_PURGE_AGE = 'PuAg';
  static const uint32 MSG_PURGE_BUTTON = 'Purg';
  static const uint32 MSG_PURGE_POPULARITY = 'PuPo';
  static const uint32 MSG_SERVER_MODE = 'SrvM';

  /* Our member functions. */
  void BrowseForDatabaseFile ();
  void BrowseForFileToEstimate ();
  void PollServerForChanges ();

  /* Member variables. */
  BButton        *m_AboutButtonPntr;
  BButton        *m_AddExampleButtonPntr;
  BButton        *m_BrowseButtonPntr;
  BFilePanel     *m_BrowseFilePanelPntr;
  BButton        *m_CreateDatabaseButtonPntr;
  char            m_DatabaseFileNameCachedValue [PATH_MAX];
  BTextControl   *m_DatabaseFileNameTextboxPntr;
  bool            m_DatabaseLoadDone;
  BButton        *m_EstimateSpamButtonPntr;
  BFilePanel     *m_EstimateSpamFilePanelPntr;
  uint32          m_GenuineCountCachedValue;
  BTextControl   *m_GenuineCountTextboxPntr;
  bool            m_IgnorePreviousClassCachedValue;
  BCheckBox      *m_IgnorePreviousClassCheckboxPntr;
  BButton        *m_InstallThingsButtonPntr;
  uint32          m_PurgeAgeCachedValue;
  BTextControl   *m_PurgeAgeTextboxPntr;
  BButton        *m_PurgeButtonPntr;
  uint32          m_PurgePopularityCachedValue;
  BTextControl   *m_PurgePopularityTextboxPntr;
  BButton        *m_ResetToDefaultsButtonPntr;
  ScoringModes    m_ScoringModeCachedValue;
  BMenuBar       *m_ScoringModeMenuBarPntr;
  BPopUpMenu     *m_ScoringModePopUpMenuPntr;
  bool            m_ServerModeCachedValue;
  BCheckBox      *m_ServerModeCheckboxPntr;
  uint32          m_SpamCountCachedValue;
  BTextControl   *m_SpamCountTextboxPntr;
  bigtime_t       m_TimeOfLastPoll;
  TokenizeModes   m_TokenizeModeCachedValue;
  BMenuBar       *m_TokenizeModeMenuBarPntr;
  BPopUpMenu     *m_TokenizeModePopUpMenuPntr;
  uint32          m_WordCountCachedValue;
  BTextControl   *m_WordCountTextboxPntr;
};


/* Various message codes for various buttons etc. */
static const uint32 MSG_LINE_DOWN = 'LnDn';
static const uint32 MSG_LINE_UP = 'LnUp';
static const uint32 MSG_PAGE_DOWN = 'PgDn';
static const uint32 MSG_PAGE_UP = 'PgUp';

/******************************************************************************
 * This view contains the list of words.  It displays as many as can fit in the
 * view rectangle, starting at a specified word (so it can simulate scrolling).
 * Usually it will appear in the bottom half of the DatabaseWindow.
 */

class WordsView : public BView
{
public:
  /* Constructor and destructor. */
  WordsView (BRect NewBounds);

  /* BeOS virtual functions. */
  virtual void AttachedToWindow ();
  virtual void Draw (BRect UpdateRect);
  virtual void KeyDown (const char *BufferPntr, int32 NumBytes);
  virtual void MakeFocus (bool Focused);
  virtual void MessageReceived (BMessage *MessagePntr);
  virtual void MouseDown (BPoint point);
  virtual void Pulse ();

private:
  /* Our member functions. */
  void MoveTextUpOrDown (uint32 MovementType);
  void RefsDroppedHere (BMessage *MessagePntr);

  /* Member variables. */
  BPictureButton *m_ArrowLineDownPntr;
  BPictureButton *m_ArrowLineUpPntr;
  BPictureButton *m_ArrowPageDownPntr;
  BPictureButton *m_ArrowPageUpPntr;
    /* Various buttons for controlling scrolling, since we can't use a scroll
    bar.  To make them less obvious, their background view colour needs to be
    changed whenever the main view's colour changes. */

  float m_AscentHeight;
    /* The ascent height for the font used to draw words.  Height from the top
    of the highest letter to the base line (which is near the middle bottom of
    the letters, the line where you would align your writing of the text by
    hand, all letters have part above, some also have descenders below this
    line). */

  rgb_color m_BackgroundColour;
    /* The current background colour.  Changes when the focus changes. */

  uint32 m_CachedTotalGenuineMessages;
  uint32 m_CachedTotalSpamMessages;
  uint32 m_CachedWordCount;
    /* These are cached copies of the similar values in the BApplication.  They
    reflect what's currently displayed.  If they are different than the values
    from the BApplication then the polling loop will try to redraw the display.
    They get set to the values actually used during drawing when drawing is
    successful. */

  char m_FirstDisplayedWord [g_MaxWordLength + 1];
    /* The scrolling display starts at this word.  Since we can't use index
    numbers (word[12345] for example), we use the word itself.  The scroll
    buttons set this to the next or previous word in the database.  Typing by
    the user when the view has the focus will also change this starting word.
    */

  rgb_color m_FocusedColour;
    /* The colour to use for focused mode (typing by the user is received by
    our view). */

  bigtime_t m_LastTimeAKeyWasPressed;
    /* Records the time when a key was last pressed.  Used for determining when
    the user has stopped typing a batch of letters. */

  float m_LineHeight;
    /* Height of a line of text in the font used for the word display.
    Includes the height of the letters plus a bit of extra space for between
    the lines (called leading). */

  BFont m_TextFont;
    /* The font used to draw the text in the window. */

  float m_TextHeight;
    /* Maximum total height of the letters in the text, includes the part above
    the baseline and the part below.  Doesn't include the sliver of space
    between lines. */

  rgb_color m_UnfocusedColour;
    /* The colour to use for unfocused mode, when user typing isn't active. */
};



/******************************************************************************
 * The BWindow class for this program.  It displays the database in real time,
 * and has various buttons and gadgets in the top half for changing settings
 * (live changes, no OK button, and they reflect changes done by other programs
 * using the server too).  The bottom half is a scrolling view listing all the
 * words in the database.  A simple graphic blotch behind each word shows
 * whether the word is strongly or weakly related to spam or genuine messages.
 * Most operations go through the scripting message system, but it also peeks
 * at the BApplication data for examining simple things and when redrawing the
 * list of words.
 */

class DatabaseWindow : public BWindow
{
public:
  /* Constructor and destructor. */
  DatabaseWindow ();

  /* BeOS virtual functions. */
  virtual void MessageReceived (BMessage *MessagePntr);
  virtual bool QuitRequested ();

private:
  /* Member variables. */
  ControlsView *m_ControlsViewPntr;
  WordsView    *m_WordsViewPntr;
};



/******************************************************************************
 * ABSApp is the BApplication class for this program.  This handles messages
 * from the outside world (requests to load a database, or to add files to the
 * collection).  It responds to command line arguments (if you start up the
 * program a second time, the system will just send the arguments to the
 * existing running program).  It responds to scripting messages.  And it
 * responds to messages from the window.  Its thread does the main work of
 * updating the database and reading / writing files.
 */

class ABSApp : public BApplication
{
public:
  /* Constructor and destructor. */
  ABSApp ();
  ~ABSApp ();

  /* BeOS virtual functions. */
  virtual void AboutRequested ();
  virtual void ArgvReceived (int32 argc, char **argv);
  virtual status_t GetSupportedSuites (BMessage *MessagePntr);
  virtual void MessageReceived (BMessage *MessagePntr);
  virtual void Pulse ();
  virtual bool QuitRequested ();
  virtual void ReadyToRun ();
  virtual void RefsReceived (BMessage *MessagePntr);
  virtual BHandler *ResolveSpecifier (BMessage *MessagePntr, int32 Index,
    BMessage *SpecifierMsgPntr, int32 SpecificationKind, const char *Property);

private:
  /* Our member functions. */
  status_t AddFileToDatabase (ClassificationTypes IsSpamOrWhat,
    const char *FileName, char *ErrorMessage);
  status_t AddPositionIOToDatabase (ClassificationTypes IsSpamOrWhat,
    BPositionIO *MessageIOPntr, const char *OptionalFileName,
    char *ErrorMessage);
  status_t AddStringToDatabase (ClassificationTypes IsSpamOrWhat,
    const char *String, char *ErrorMessage);
  void AddWordsToSet (const char *InputString, size_t NumberOfBytes,
    char PrefixCharacter, set<string> &WordSet);
  status_t CreateDatabaseFile (char *ErrorMessage);
  void DefaultSettings ();
  status_t DeleteDatabaseFile (char *ErrorMessage);
  status_t EvaluateFile (const char *PathName, BMessage *ReplyMessagePntr,
    char *ErrorMessage);
  status_t EvaluatePositionIO (BPositionIO *PositionIOPntr,
    const char *OptionalFileName, BMessage *ReplyMessagePntr,
    char *ErrorMessage);
  status_t EvaluateString (const char *BufferPntr, ssize_t BufferSize,
    BMessage *ReplyMessagePntr, char *ErrorMessage);
  status_t GetWordsFromPositionIO (BPositionIO *PositionIOPntr,
    const char *OptionalFileName, set<string> &WordSet, char *ErrorMessage);
  status_t InstallThings (char *ErrorMessage);
  status_t LoadDatabaseIfNeeded (char *ErrorMessage);
  status_t LoadSaveDatabase (bool DoLoad, char *ErrorMessage);
public:
  status_t LoadSaveSettings (bool DoLoad);
private:
  status_t MakeBackup (char *ErrorMessage);
  void MakeDatabaseEmpty ();
  void ProcessScriptingMessage (BMessage *MessagePntr,
    struct property_info *PropInfoPntr);
  status_t PurgeOldWords (char *ErrorMessage);
  status_t RecursivelyTokenizeMailComponent (
    BMailComponent *ComponentPntr, const char *OptionalFileName,
    set<string> &WordSet, char *ErrorMessage,
    int RecursionLevel, int MaxRecursionLevel);
  status_t SaveDatabaseIfNeeded (char *ErrorMessage);
  status_t TokenizeParts (BPositionIO *PositionIOPntr,
    const char *OptionalFileName, set<string> &WordSet, char *ErrorMessage);
  status_t TokenizeWhole (BPositionIO *PositionIOPntr,
    const char *OptionalFileName, set<string> &WordSet, char *ErrorMessage);

public:
  /* Member variables.  Many are read by the window thread to see if it needs
  updating, and to draw the words.  However, the other threads will lock the
  BApplication or using scripting commands if they want to make changes. */

  bool m_DatabaseHasChanged;
    /* Set to TRUE when the in-memory database (stored in m_WordMap) has
    changed and is different from the on-disk database file.  When the
    application exits, the database will be written out if it has changed. */

  BString m_DatabaseFileName;
    /* The absolute path name to use for the database file on disk. */

  bool m_IgnorePreviousClassification;
    /* If TRUE then the previous classification of a message (stored in an
    attribute on the message file) will be ignored, and the message will be
    added to the requested spam/genuine list.  If this is FALSE then the spam
    won't be added to the list if it has already been classified as specified,
    but if it was mis-classified, it will be removed from the old list and
    added to the new list. */

  uint32 m_OldestAge;
    /* The age of the oldest word.  This will be the smallest age number in the
    database.  Mostly useful for scaling graphics representing age in the word
    display.  If the oldest word is no longer the oldest, this variable won't
    get immediately updated since it would take a lot of effort to find the
    next older age.  Since it's only used for display, we'll let it be slightly
    incorrect.  The next database load or purge will fix it. */

  uint32 m_PurgeAge;
    /* When purging old words, they have to be at least this old to be eligible
    for deletion.  Age is measured as the number of e-mails added to the
    database since the word was last updated in the database.  Zero means all
    words are old. */

  uint32 m_PurgePopularity;
    /* When purging old words, they have to be less than or equal to this
    popularity limit to be eligible for deletion.  Popularity is measured as
    the number of messages (spam and genuine) which have the word.  Zero means
    no words. */

  ScoringModes m_ScoringMode;
    /* Controls how to combine the word probabilities into an overall score.
    See the PN_SCORING_MODE comments for details. */

  BPath m_SettingsDirectoryPath;
    /* The constructor initialises this to the settings directory path.  It
    never changes after that. */

  bool m_SettingsHaveChanged;
    /* Set to TRUE when the settings are changed (different than the ones which
    were loaded).  When the application exits, the settings will be written out
    if they have changed. */

  double m_SmallestUseableDouble;
    /* When multiplying fractional numbers together, avoid using numbers
    smaller than this because the double exponent range is close to being
    exhausted.  The IEEE STANDARD 754 floating-point arithmetic (used on the
    Intel i8087 and later math processors) has 64 bit numbers with 53 bits of
    mantissa, giving it an underflow starting at 0.5**1022 = 2.2e-308 where it
    rounds off to the nearest multiple of 0.5**1074 = 4.9e-324. */

  TokenizeModes m_TokenizeMode;
    /* Controls how to convert the raw message text into words.  See the
    PN_TOKENIZE_MODE comments for details. */

  uint32 m_TotalGenuineMessages;
    /* Number of genuine messages which are in the database. */

  uint32 m_TotalSpamMessages;
    /* Number of spam messages which are in the database. */

  uint32 m_WordCount;
    /* The number of words currently in the database.  Stored separately as a
    member variable to avoid having to call m_WordMap.size() all the time,
    which other threads can't do while the database is being updated (but they
    can look at the word count variable). */

  StatisticsMap m_WordMap;
    /* The in-memory data structure holding the set of words and their
    associated statistics.  When the database isn't in use, it is an empty
    collection.  You should lock the BApplication if you are using the word
    collection (reading or writing) from another thread. */
};



/******************************************************************************
 * Global utility function to display an error message and return.  The message
 * part describes the error, and if ErrorNumber is non-zero, gets the string
 * ", error code $X (standard description)." appended to it.  If the message
 * is NULL then it gets defaulted to "Something went wrong".  The title part
 * doesn't get displayed (no title bar in the dialog box, but you can see it in
 * the debugger as the window thread name), and defaults to "Error Message" if
 * you didn't specify one.  If running in command line mode, the error gets
 * printed to stderr rather than showing up in a dialog box.
 */

static void DisplayErrorMessage (
  const char *MessageString = NULL,
  int ErrorNumber = 0,
  const char *TitleString = NULL)
{
  BAlert *AlertPntr;
  char ErrorBuffer [PATH_MAX + 1500];

  if (TitleString == NULL)
    TitleString = "SpamDBM Error Message";

  if (MessageString == NULL)
  {
    if (ErrorNumber == 0)
      MessageString = "No error, no message, why bother?";
    else
      MessageString = "Something went wrong";
  }

  if (ErrorNumber != 0)
  {
    sprintf (ErrorBuffer, "%s, error code $%X/%d (%s) has occured.",
      MessageString, ErrorNumber, ErrorNumber, strerror (ErrorNumber));
    MessageString = ErrorBuffer;
  }

  if (g_CommandLineMode || g_ServerMode)
    cerr << TitleString << ": " << MessageString << endl;
  else
  {
    AlertPntr = new BAlert (TitleString, MessageString,
      "Acknowledge", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
    if (AlertPntr != NULL)
      AlertPntr->Go ();
  }
}



/******************************************************************************
 * Word wrap a long line of text into shorter 79 column lines and print the
 * result on the given output stream.
 */

static void WrapTextToStream (ostream& OutputStream, const char *TextPntr)
{
  const int LineLength = 79;
  char     *StringPntr;
  char      TempString [LineLength+1];

  TempString[LineLength] = 0; /* Only needs to be done once. */

  while (*TextPntr != 0)
  {
    while (isspace (*TextPntr))
      TextPntr++; /* Skip leading spaces. */
    if (*TextPntr == 0)
      break; /* It was all spaces, don't print any more. */

    strncpy (TempString, TextPntr, LineLength);

    /* Advance StringPntr to the end of the temp string, partly to see how long
    it is (rather than doing strlen). */

    StringPntr = TempString;
    while (*StringPntr != 0)
      StringPntr++;

    if (StringPntr - TempString < LineLength)
    {
      /* This line fits completely. */
      OutputStream << TempString << endl;
      TextPntr += StringPntr - TempString;
      continue;
    }

    /* Advance StringPntr to the last space in the temp string. */

    while (StringPntr > TempString)
    {
      if (isspace (*StringPntr))
        break; /* Found the trailing space. */
      else /* Go backwards, looking for the trailing space. */
        StringPntr--;
    }

    /* Remove more trailing spaces at the end of the line, in case there were
    several spaces in a row. */

    while (StringPntr > TempString && isspace (StringPntr[-1]))
      StringPntr--;

    /* Print the line of text and advance the text pointer too. */

    if (StringPntr == TempString)
    {
      /* This line has no spaces, don't wrap it, just split off a chunk. */
      OutputStream << TempString << endl;
      TextPntr += strlen (TempString);
      continue;
    }

    *StringPntr = 0; /* Cut off after the first trailing space. */
    OutputStream << TempString << endl;
    TextPntr += StringPntr - TempString;
  }
}



/******************************************************************************
 * Print the usage info to the stream.  Includes a list of all commands.
 */
ostream& PrintUsage (ostream& OutputStream);

ostream& PrintUsage (ostream& OutputStream)
{
  struct property_info *PropInfoPntr;

  OutputStream << "\nSpamDBM - A Spam Database Manager\n";
  OutputStream << "Copyright © 2002 by Alexander G. M. Smith.  ";
  OutputStream << "Released to the public domain.\n\n";
  WrapTextToStream (OutputStream, "Compiled on " __DATE__ " at " __TIME__
".  $Id: spamdbm.cpp 30630 2009-05-05 01:31:01Z bga $  $HeadURL: http://svn.haiku-os.org/haiku/haiku/trunk/src/bin/mail_utils/spamdbm.cpp $");
  OutputStream << "\n"
"This is a program for classifying e-mail messages as spam (junk mail which\n"
"you don't want to read) and regular genuine messages.  It can learn what's\n"
"spam and what's genuine.  You just give it a bunch of spam messages and a\n"
"bunch of non-spam ones.  It uses them to make a list of the words from the\n"
"messages with the probability that each word is from a spam message or from\n"
"a genuine message.  Later on, it can use those probabilities to classify\n"
"new messages as spam or not spam.  If the classifier stops working well\n"
"(because the spammers have changed their writing style and vocabulary, or\n"
"your regular correspondants are writing like spammers), you can use this\n"
"program to update the list of words to identify the new messages\n"
"correctly.\n"
"\n"
"The original idea was from Paul Graham's algorithm, which has an excellent\n"
"writeup at: http://www.paulgraham.com/spam.html\n"
"\n"
"Gary Robinson came up with the improved algorithm, which you can read about at:\n"
"http://radio.weblogs.com/0101454/stories/2002/09/16/spamDetection.html\n"
"\n"
"Then he, Tim Peters and the SpamBayes mailing list developed the Chi-Squared\n"
"test, see http://mail.python.org/pipermail/spambayes/2002-October/001036.html\n"
"for one of the earlier messages leading from the central limit theorem to\n"
"the current chi-squared scoring method.\n"
"\n"
"Thanks go to Isaac Yonemoto for providing a better icon, which we can\n"
"unfortunately no longer use, since the Hormel company wants people to\n"
"avoid associating their meat product with junk e-mail.\n"
"\n"
"Tokenising code updated in 2005 to use some of the tricks that SpamBayes\n"
"uses to extract words from messages.  In particular, HTML is now handled.\n"
"\n"
"Usage: Specify the operation as the first argument followed by more\n"
"information as appropriate.  The program's configuration will affect the\n"
"actual operation (things like the name of the database file to use, or\n"
"whether it should allow non-email messages to be added).  In command line\n"
"mode it will do the operation and exit.  In GUI/server mode a command line\n"
"invocation will just send the command to the running server.  You can also\n"
"use BeOS scripting (see the \"Hey\" command which you can get from\n"
"http://www.bebits.com/app/2042 ) to control the Spam server.  And finally,\n"
"there's also a GUI interface which shows up if you start it without any\n"
"command line arguments.\n"
"\n"
"Commands:\n"
"\n"
"Quit\n"
"Stop the program.  Useful if it's running as a server.\n"
"\n";

  /* Go through all our scripting commands and add a description of each one to
  the usage text. */

  for (PropInfoPntr = g_ScriptingPropertyList + 0;
  PropInfoPntr->name != 0;
  PropInfoPntr++)
  {
    switch (PropInfoPntr->commands[0])
    {
      case B_GET_PROPERTY:
        OutputStream << "Get " << PropInfoPntr->name << endl;
        break;

      case B_SET_PROPERTY:
        OutputStream << "Set " << PropInfoPntr->name << " NewValue" << endl;
        break;

      case B_COUNT_PROPERTIES:
        OutputStream << "Count " << PropInfoPntr->name << endl;
        break;

      case B_CREATE_PROPERTY:
        OutputStream << "Create " << PropInfoPntr->name << endl;
        break;

      case B_DELETE_PROPERTY:
        OutputStream << "Delete " << PropInfoPntr->name << endl;
        break;

      case B_EXECUTE_PROPERTY:
        OutputStream << PropInfoPntr->name << endl;
        break;

      default:
        OutputStream << "Buggy Command: " << PropInfoPntr->name << endl;
        break;
    }
    WrapTextToStream (OutputStream, (char *)PropInfoPntr->usage);
    OutputStream << endl;
  }

  return OutputStream;
}



/******************************************************************************
 * A utility function to send a command to the application, will return after a
 * short delay if the application is busy (doesn't wait for it to be executed).
 * The reply from the application is also thrown away.  It used to be an
 * overloaded function, but the system couldn't distinguish between bool and
 * int, so now it has slightly different names depending on the arguments.
 */

static void SubmitCommand (BMessage& CommandMessage)
{
  status_t ErrorCode;

  ErrorCode = be_app_messenger.SendMessage (&CommandMessage,
    be_app_messenger /* reply messenger, throw away the reply */,
    1000000 /* delivery timeout */);

  if (ErrorCode != B_OK)
    cerr << "SubmitCommand failed to send a command, code " <<
    ErrorCode << " (" << strerror (ErrorCode) << ")." << endl;
}


static void SubmitCommandString (
  PropertyNumbers Property,
  uint32 CommandCode,
  const char *StringArgument = NULL)
{
  BMessage CommandMessage (CommandCode);

  if (Property < 0 || Property >= PN_MAX)
  {
    DisplayErrorMessage ("SubmitCommandString bug.");
    return;
  }
  CommandMessage.AddSpecifier (g_PropertyNames [Property]);
  if (StringArgument != NULL)
    CommandMessage.AddString (g_DataName, StringArgument);
  SubmitCommand (CommandMessage);
}


static void SubmitCommandInt32 (
  PropertyNumbers Property,
  uint32 CommandCode,
  int32 Int32Argument)
{
  BMessage CommandMessage (CommandCode);

  if (Property < 0 || Property >= PN_MAX)
  {
    DisplayErrorMessage ("SubmitCommandInt32 bug.");
    return;
  }
  CommandMessage.AddSpecifier (g_PropertyNames [Property]);
  CommandMessage.AddInt32 (g_DataName, Int32Argument);
  SubmitCommand (CommandMessage);
}


static void SubmitCommandBool (
  PropertyNumbers Property,
  uint32 CommandCode,
  bool BoolArgument)
{
  BMessage CommandMessage (CommandCode);

  if (Property < 0 || Property >= PN_MAX)
  {
    DisplayErrorMessage ("SubmitCommandBool bug.");
    return;
  }
  CommandMessage.AddSpecifier (g_PropertyNames [Property]);
  CommandMessage.AddBool (g_DataName, BoolArgument);
  SubmitCommand (CommandMessage);
}



/******************************************************************************
 * A utility function which will estimate the spaminess of file(s), not
 * callable from the application thread since it sends a scripting command to
 * the application and waits for results.  For each file there will be an entry
 * reference in the message.  For each of those, run it through the spam
 * estimator and display a box with the results.  This function is used both by
 * the file requestor and by dragging and dropping into the middle of the words
 * view.
 */

static void EstimateRefFilesAndDisplay (BMessage *MessagePntr)
{
  BAlert     *AlertPntr;
  BEntry      Entry;
  entry_ref   EntryRef;
  status_t    ErrorCode;
  int         i, j;
  BPath       Path;
  BMessage    ReplyMessage;
  BMessage    ScriptingMessage;
  const char *StringPntr;
  float       TempFloat;
  int32       TempInt32;
  char        TempString [PATH_MAX + 1024 +
                g_MaxInterestingWords * (g_MaxWordLength + 16)];

  for (i = 0; MessagePntr->FindRef ("refs", i, &EntryRef) == B_OK; i++)
  {
    /* See if the entry is a valid file or directory or other thing. */

    ErrorCode = Entry.SetTo (&EntryRef, true /* traverse symbolic links */);
    if (ErrorCode != B_OK || !Entry.Exists () || Entry.GetPath (&Path) != B_OK)
      continue;

    /* Evaluate the spaminess of the file. */

    ScriptingMessage.MakeEmpty ();
    ScriptingMessage.what = B_SET_PROPERTY;
    ScriptingMessage.AddSpecifier (g_PropertyNames[PN_EVALUATE]);
    ScriptingMessage.AddString (g_DataName, Path.Path ());

    if (be_app_messenger.SendMessage (&ScriptingMessage,&ReplyMessage) != B_OK)
      break; /* App has died or something is wrong. */

    if (ReplyMessage.FindInt32 ("error", &TempInt32) != B_OK ||
    TempInt32 != B_OK)
      break; /* Error messages will be displayed elsewhere. */

    ReplyMessage.FindFloat (g_ResultName, &TempFloat);
    sprintf (TempString, "%f spam ratio for \"%s\".\nThe top words are:",
      (double) TempFloat, Path.Path ());

    for (j = 0; j < 20 /* Don't print too many! */; j++)
    {
      if (ReplyMessage.FindString ("words", j, &StringPntr) != B_OK ||
      ReplyMessage.FindFloat ("ratios", j, &TempFloat) != B_OK)
        break;

      sprintf (TempString + strlen (TempString), "\n%s / %f",
        StringPntr, TempFloat);
    }
    if (j >= 20 && j < g_MaxInterestingWords)
      sprintf (TempString + strlen (TempString), "\nAnd up to %d more words.",
        g_MaxInterestingWords - j);

    AlertPntr = new BAlert ("Estimate", TempString, "OK");
    if (AlertPntr != NULL)
      AlertPntr->Go ();
  }
}



/******************************************************************************
 * A utility function from the http://sourceforge.net/projects/spambayes
 * SpamBayes project.  Return prob(chisq >= x2, with v degrees of freedom).  It
 * computes the probability that the chi-squared value (a kind of normalized
 * error measurement), with v degrees of freedom, would be larger than a given
 * number (x2; chi is the Greek letter X thus x2).  So you can tell if the
 * error is really unusual (the returned probability is near zero meaning that
 * your measured error number is kind of large - actual chi-squared is rarely
 * above that number merely due to random effects), or if it happens often
 * (usually if the probability is over 5% then it's within 3 standard
 * deviations - meaning that chi-squared goes over your number fairly often due
 * merely to random effects).  v must be even for this calculation to work.
 */

static double ChiSquaredProbability (double x2, int v)
{
  int    halfV = v / 2;
  int    i;
  double m;
  double sum;
  double term;

  if (v & 1)
    return -1.0; /* Out of range return value as a hint v is odd. */

  /* If x2 is very large, exp(-m) will underflow to 0. */
  m = x2 / 2.0;
  sum = term = exp (-m);
  for (i = 1; i < halfV; i++)
  {
    term *= m / i;
    sum += term;
  }

  /* With small x2 and large v, accumulated roundoff error, plus error in the
  platform exp(), can cause this to spill a few ULP above 1.0.  For example,
  ChiSquaredProbability(100, 300) on my box has sum == 1.0 + 2.0**-52 at this
  point.  Returning a value even a teensy bit over 1.0 is no good. */

  if (sum > 1.0)
    return 1.0;
  return sum;
}



/******************************************************************************
 * A utility function to remove the "[Spam 99.9%] " from in front of the
 * MAIL:subject attribute of a file.
 */

static status_t RemoveSpamPrefixFromSubjectAttribute (BNode *BNodePntr)
{
  status_t    ErrorCode;
  const char *MailSubjectName = "MAIL:subject";
  char       *StringPntr;
  char        SubjectString [2000];

  ErrorCode = BNodePntr->ReadAttr (MailSubjectName,
    B_STRING_TYPE, 0 /* offset */, SubjectString,
    sizeof (SubjectString) - 1);
  if (ErrorCode <= 0)
    return 0; /* The attribute isn't there so we don't care. */
  if (ErrorCode >= (int) sizeof (SubjectString) - 1)
    return 0; /* Can't handle subjects which are too long. */

  SubjectString [ErrorCode] = 0;
  ErrorCode = 0; /* So do-nothing exit returns zero. */
  if (strncmp (SubjectString, "[Spam ", 6) == 0)
  {
    for (StringPntr = SubjectString;
    *StringPntr != 0 && *StringPntr != ']'; StringPntr++)
      ; /* No body in this for loop. */
    if (StringPntr[0] == ']' && StringPntr[1] == ' ')
    {
      ErrorCode = BNodePntr->RemoveAttr (MailSubjectName);
      ErrorCode = BNodePntr->WriteAttr (MailSubjectName,
        B_STRING_TYPE, 0 /* offset */,
        StringPntr + 2, strlen (StringPntr + 2) + 1);
      if (ErrorCode > 0)
        ErrorCode = 0;
    }
  }

  return ErrorCode;
}



/******************************************************************************
 * The tokenizing functions.  To make tokenization of the text easier to
 * understand, it is broken up into several passes.  Each pass goes over the
 * text (can include NUL bytes) and extracts all the words it can recognise
 * (can be none).  The extracted words are added to the WordSet, with the
 * PrefixCharacter prepended (zero if none) so we can distinguish between words
 * found in headers and in the text body.  It also modifies the input text
 * buffer in-place to change the text that the next pass will see (blanking out
 * words that it wants to delete, but not inserting much new text since the
 * buffer can't be enlarged).  They all return the number of bytes remaining in
 * InputString after it has been modified to be input for the next pass.
 * Returns zero if it has exhausted the possibility of getting more words, or
 * if something goes wrong.
 */

static size_t TokenizerPassLowerCase (
  char *BufferPntr,
  size_t NumberOfBytes)
{
  char *EndOfStringPntr;

  EndOfStringPntr = BufferPntr + NumberOfBytes;

  while (BufferPntr < EndOfStringPntr)
  {
    /* Do our own lower case conversion; tolower () has problems with UTF-8
    characters that have the high bit set. */

    if (*BufferPntr >= 'A' && *BufferPntr <= 'Z')
      *BufferPntr = *BufferPntr + ('a' - 'A');
    BufferPntr++;
  }
  return NumberOfBytes;
}


/* A utility function for some commonly repeated code.  If this was Modula-2,
we could use a nested procedure.  But it's not.  Adds the given word to the set
of words, checking for maximum word length and prepending the prefix to the
word, which gets modified by this function to reflect the word actually added
to the set. */

static void AddWordAndPrefixToSet (
  string &Word,
  const char *PrefixString,
  set<string> &WordSet)
{
  if (Word.empty ())
    return;

  if (Word.size () > g_MaxWordLength)
    Word.resize (g_MaxWordLength);
  Word.insert (0, PrefixString);
  WordSet.insert (Word);
}


/* Hunt through the text for various URLs and extract the components as
separate words.  Doesn't affect the text in the buffer.  Looks for
protocol://user:password@computer:port/path?query=key#anchor strings.  Also
www.blah strings are detected and broken down.  Doesn't do HREF="" strings
where the string has a relative path (no host computer name).  Assumes the
input buffer is already in lower case. */

static size_t TokenizerPassExtractURLs (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  char   *AtSignStringPntr;
  char   *HostStringPntr;
  char   *InputStringEndPntr;
  char   *InputStringPntr;
  char   *OptionsStringPntr;
  char   *PathStringPntr;
  char    PrefixString [2];
  char   *ProtocolStringPntr;
  string  Word;

  InputStringPntr = BufferPntr;
  InputStringEndPntr = BufferPntr + NumberOfBytes;
  PrefixString [0] = PrefixCharacter;
  PrefixString [1] = 0;

  while (InputStringPntr < InputStringEndPntr - 4)
  {
    HostStringPntr = NULL;
    if (memcmp (InputStringPntr, "www.", 4) == 0)
      HostStringPntr = InputStringPntr;
    else if (memcmp (InputStringPntr, "://", 3) == 0)
    {
      /* Find the protocol name, and add it as a word such as "ftp:" "http:" */
      ProtocolStringPntr = InputStringPntr;
      while (ProtocolStringPntr > BufferPntr &&
      isalpha (ProtocolStringPntr[-1]))
        ProtocolStringPntr--;
      Word.assign (ProtocolStringPntr,
        (InputStringPntr - ProtocolStringPntr) + 1 /* for the colon */);
      AddWordAndPrefixToSet (Word, PrefixString, WordSet);
      HostStringPntr = InputStringPntr + 3; /* Skip past the "://" */
    }
    if (HostStringPntr == NULL)
    {
      InputStringPntr++;
      continue;
    }

    /* Got a host name string starting at HostStringPntr.  It's everything
    until the next slash or space, like "user:password@computer:port". */

    InputStringPntr = HostStringPntr;
    AtSignStringPntr = NULL;
    while (InputStringPntr < InputStringEndPntr &&
    (*InputStringPntr != '/' && !isspace (*InputStringPntr)))
    {
      if (*InputStringPntr == '@')
        AtSignStringPntr = InputStringPntr;
      InputStringPntr++;
    }
    if (AtSignStringPntr != NULL)
    {
      /* Add a word with the user and password, unseparated. */
      Word.assign (HostStringPntr,
        AtSignStringPntr - HostStringPntr + 1 /* for the @ sign */);
      AddWordAndPrefixToSet (Word, PrefixString, WordSet);
      HostStringPntr = AtSignStringPntr + 1;
    }

    /* Add a word with the computer and port, unseparated. */

    Word.assign (HostStringPntr, InputStringPntr - HostStringPntr);
    AddWordAndPrefixToSet (Word, PrefixString, WordSet);

    /* Now get the path name, not including the extra junk after ?  and #
    separators (they're stored as separate options).  Stops at white space or a
    double quote mark. */

    PathStringPntr = InputStringPntr;
    OptionsStringPntr = NULL;
    while (InputStringPntr < InputStringEndPntr &&
    (*InputStringPntr != '"' && !isspace (*InputStringPntr)))
    {
      if (OptionsStringPntr == NULL &&
      (*InputStringPntr == '?' || *InputStringPntr == '#'))
        OptionsStringPntr = InputStringPntr;
      InputStringPntr++;
    }

    if (OptionsStringPntr == NULL)
    {
      /* No options, all path. */
      Word.assign (PathStringPntr, InputStringPntr - PathStringPntr);
      AddWordAndPrefixToSet (Word, PrefixString, WordSet);
    }
    else
    {
      /* Insert the path before the options. */
      Word.assign (PathStringPntr, OptionsStringPntr - PathStringPntr);
      AddWordAndPrefixToSet (Word, PrefixString, WordSet);

      /* Insert all the options as a word. */
      Word.assign (OptionsStringPntr, InputStringPntr - OptionsStringPntr);
      AddWordAndPrefixToSet (Word, PrefixString, WordSet);
    }
  }
  return NumberOfBytes;
}


/* Replace long Asian words (likely to actually be sentences) with the first
character in the word. */

static size_t TokenizerPassTruncateLongAsianWords (
  char *BufferPntr,
  size_t NumberOfBytes)
{
  char *EndOfStringPntr;
  char *InputStringPntr;
  int   Letter;
  char *OutputStringPntr;
  char *StartOfInputLongUnicodeWord;
  char *StartOfOutputLongUnicodeWord;

  InputStringPntr = BufferPntr;
  EndOfStringPntr = InputStringPntr + NumberOfBytes;
  OutputStringPntr = InputStringPntr;
  StartOfInputLongUnicodeWord = NULL; /* Non-NULL flags it as started. */
  StartOfOutputLongUnicodeWord = NULL;

  /* Copy the text from the input to the output (same buffer), but when we find
  a sequence of UTF-8 characters that is too long then truncate it down to one
  character and reset the output pointer to be after that character, thus
  deleting the word.  Replacing the deleted characters after it with spaces
  won't work since we need to preserve the lack of space to handle those sneaky
  HTML artificial word breakers.  So that Thelongword<blah>ing becomes
  "T<blah>ing" rather than "T <blah>ing", so the next step joins them up into
  "Ting" rather than "T" and "ing".  The first code in a UTF-8 character is
  11xxxxxx and subsequent ones are 10xxxxxx. */

  while (InputStringPntr < EndOfStringPntr)
  {
    Letter = (unsigned char) *InputStringPntr;
    if (Letter < 128) // Got a regular ASCII letter?
    {
      if (StartOfInputLongUnicodeWord != NULL)
      {
        if (InputStringPntr - StartOfInputLongUnicodeWord >
        (int) g_MaxWordLength * 2)
        {
          /* Need to truncate the long word (100 bytes or about 50 characters)
          back down to the first UTF-8 character, so find out where the first
          character ends (skip past the 10xxxxxx bytes), and rewind the output
          pointer to be just after that (ignoring the rest of the long word in
          effect). */

          OutputStringPntr = StartOfOutputLongUnicodeWord + 1;
          while (OutputStringPntr < InputStringPntr)
          {
            Letter = (unsigned char) *OutputStringPntr;
            if (Letter < 128 || Letter >= 192)
              break;
            ++OutputStringPntr; // Still a UTF-8 middle of the character code.
          }
        }
        StartOfInputLongUnicodeWord = NULL;
      }
    }
    else if (Letter >= 192 && StartOfInputLongUnicodeWord == NULL)
    {
      /* Got the start of a UTF-8 character.  Remember the spot so we can see
      if this is a too long UTF-8 word, which is often a whole sentence in
      asian languages, since they sort of use a single character per word. */

      StartOfInputLongUnicodeWord = InputStringPntr;
      StartOfOutputLongUnicodeWord = OutputStringPntr;
    }
    *OutputStringPntr++ = *InputStringPntr++;
  }
  return OutputStringPntr - BufferPntr;
}


/* Find all the words in the string and add them to our local set of words.
The characters considered white space are defined by g_SpaceCharacters.  This
function is also used as a subroutine by other tokenizer functions when they
have a bunch of presumably plain text they want broken into words and added. */

static size_t TokenizerPassGetPlainWords (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  string  AccumulatedWord;
  char   *EndOfStringPntr;
  size_t  Length;
  int     Letter;

  if (NumberOfBytes <= 0)
    return 0; /* Nothing to process. */

  if (PrefixCharacter != 0)
    AccumulatedWord = PrefixCharacter;
  EndOfStringPntr = BufferPntr + NumberOfBytes;
  while (true)
  {
    if (BufferPntr >= EndOfStringPntr)
      Letter = EOF; // Usually a negative number.
    else
      Letter = (unsigned char) *BufferPntr++;

    /* See if it is a letter we treat as white space.  Some word separators
    like dashes and periods aren't considered as space.  Note that codes above
    127 are UTF-8 characters, which we consider non-space. */

    if (Letter < 0 /* EOF is -1 */ ||
    (Letter < 128 && g_SpaceCharacters[Letter]))
    {
      /* That space finished off a word.  Remove trailing periods... */

      while ((Length = AccumulatedWord.size()) > 0 &&
      AccumulatedWord [Length-1] == '.')
        AccumulatedWord.resize (Length - 1);

      /* If there's anything left in the word, add it to the set.  Also ignore
      words which are too big (it's probably some binary encoded data).  But
      leave room for supercalifragilisticexpialidoceous.  According to one web
      site, pneumonoultramicroscopicsilicovolcanoconiosis is the longest word
      currently in English.  Note that some uuencoded data was seen with a 60
      character line length. */

      if (PrefixCharacter != 0)
        Length--; // Don't count prefix when judging size or emptiness.
      if (Length > 0 && Length <= g_MaxWordLength)
        WordSet.insert (AccumulatedWord);

      /* Empty out the string to get ready for the next word.  Not quite empty,
      start it off with the prefix character if any. */

      if (PrefixCharacter != 0)
        AccumulatedWord = PrefixCharacter;
      else
        AccumulatedWord.resize (0);
    }
    else /* Not a space-like character, add it to the word. */
      AccumulatedWord.append (1 /* one copy of the char */, (char) Letter);

    if (Letter < 0)
      break; /* End of data.  Exit here so that last word got processed. */
  }
  return NumberOfBytes;
}


/* Delete Things from the text.  The Thing is marked by a start string and an
end string, such as "<!--" and "--> for HTML comment things.  All the text
between the markers will be added to the word list before it gets deleted from
the buffer.  The markers must be prepared in lower case and the buffer is
assumed to have already been converted to lower case.  You can specify an empty
string for the end marker if you're just matching a string constant like
"&nbsp;", which you would put in the starting marker.  This is a utility
function used by other tokenizer functions. */

static size_t TokenizerUtilRemoveStartEndThing (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet,
  const char *ThingStartCode,
  const char *ThingEndCode,
  bool ReplaceWithSpace)
{
  char *EndOfStringPntr;
  bool  FoundAndDeletedThing;
  char *InputStringPntr;
  char *OutputStringPntr;
  int   ThingEndLength;
  char *ThingEndPntr;
  int   ThingStartLength;

  InputStringPntr = BufferPntr;
  EndOfStringPntr = InputStringPntr + NumberOfBytes;
  OutputStringPntr = InputStringPntr;
  ThingStartLength = strlen (ThingStartCode);
  ThingEndLength = strlen (ThingEndCode);

  if (ThingStartLength <= 0)
    return NumberOfBytes; /* Need some things to look for first! */

  while (InputStringPntr < EndOfStringPntr)
  {
    /* Search for the starting marker. */

    FoundAndDeletedThing = false;
    if (EndOfStringPntr - InputStringPntr >=
    ThingStartLength + ThingEndLength /* space remains for start + end */ &&
    *InputStringPntr == *ThingStartCode &&
    memcmp (InputStringPntr, ThingStartCode, ThingStartLength) == 0)
    {
      /* Found the start marker.  Look for the terminating string.  If it is an
      empty string, then we've found it right now! */

      ThingEndPntr = InputStringPntr + ThingStartLength;
      while (EndOfStringPntr - ThingEndPntr >= ThingEndLength)
      {
        if (ThingEndLength == 0 ||
        (*ThingEndPntr == *ThingEndCode &&
        memcmp (ThingEndPntr, ThingEndCode, ThingEndLength) == 0))
        {
          /* Got the end of the Thing.  First dump the text inbetween the start
          and end markers into the words list. */

          TokenizerPassGetPlainWords (InputStringPntr + ThingStartLength,
            ThingEndPntr - (InputStringPntr + ThingStartLength),
            PrefixCharacter, WordSet);

          /* Delete by not updating the output pointer while moving the input
          pointer to just after the ending tag. */

          InputStringPntr = ThingEndPntr + ThingEndLength;
          if (ReplaceWithSpace)
            *OutputStringPntr++ = ' ';
          FoundAndDeletedThing = true;
          break;
        }
        ThingEndPntr++;
      } /* End while ThingEndPntr */
    }
    if (!FoundAndDeletedThing)
      *OutputStringPntr++ = *InputStringPntr++;
  } /* End while InputStringPntr */

  return OutputStringPntr - BufferPntr;
}


static size_t TokenizerPassRemoveHTMLComments (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  return TokenizerUtilRemoveStartEndThing (BufferPntr, NumberOfBytes,
    PrefixCharacter, WordSet, "<!--", "-->", false);
}


static size_t TokenizerPassRemoveHTMLStyle (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  return TokenizerUtilRemoveStartEndThing (BufferPntr, NumberOfBytes,
    PrefixCharacter, WordSet,
    "<style", "/style>", false /* replace with space if true */);
}


/* Convert Japanese periods (a round hollow dot symbol) to spaces so that the
start of the next sentence is recognised at least as the start of a very long
word.  The Japanese comma also does the same job. */

static size_t TokenizerPassJapanesePeriodsToSpaces (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  size_t BytesRemaining = NumberOfBytes;

  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "。" /* period */, "", true);
  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "、" /* comma */, "", true);
  return BytesRemaining;
}


/* Delete HTML tags from the text.  The contents of the tag are added as words
before being deleted.  <P>, <BR> and &nbsp; are replaced by spaces at this
stage while other HTML things get replaced by nothing. */

static size_t TokenizerPassRemoveHTMLTags (
  char *BufferPntr,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  size_t BytesRemaining = NumberOfBytes;

  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "&nbsp;", "", true);
  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "<p", ">", true);
  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "<br", ">", true);
  BytesRemaining = TokenizerUtilRemoveStartEndThing (BufferPntr,
    BytesRemaining, PrefixCharacter, WordSet, "<", ">", false);
  return BytesRemaining;
}



/******************************************************************************
 * Implementation of the ABSApp class, constructor, destructor and the rest of
 * the member functions in mostly alphabetical order.
 */

ABSApp::ABSApp ()
: BApplication (g_ABSAppSignature),
  m_DatabaseHasChanged (false),
  m_SettingsHaveChanged (false)
{
  status_t    ErrorCode;
  int         HalvingCount;
  int         i;
  const void *ResourceData;
  size_t      ResourceSize;
  BResources *ResourcesPntr;

  MakeDatabaseEmpty ();

  /* Set up the pathname which identifies our settings directory.  Note that
  the actual settings are loaded later on (or set to defaults) by the main()
  function, before this BApplication starts running.  So we don't bother
  initialising the other setting related variables here. */

  ErrorCode =
    find_directory (B_USER_SETTINGS_DIRECTORY, &m_SettingsDirectoryPath);
  if (ErrorCode == B_OK)
    ErrorCode = m_SettingsDirectoryPath.Append (g_SettingsDirectoryName);
  if (ErrorCode != B_OK)
    m_SettingsDirectoryPath.SetTo (".");

  /* Set up the table which identifies which characters are spaces and which
  are not.  Spaces are all control characters and all punctuation except for:
  apostrophe (so "it's" and possessive versions of words get stored), dash (for
  hyphenated words), dollar sign (for cash amounts), period (for IP addresses,
  we later remove trailing periods). */

  memset (g_SpaceCharacters, 1, sizeof (g_SpaceCharacters));
  g_SpaceCharacters['\''] = false;
  g_SpaceCharacters['-'] = false;
  g_SpaceCharacters['$'] = false;
  g_SpaceCharacters['.'] = false;
  for (i = '0'; i <= '9'; i++)
    g_SpaceCharacters[i] = false;
  for (i = 'A'; i <= 'Z'; i++)
    g_SpaceCharacters[i] = false;
  for (i = 'a'; i <= 'z'; i++)
    g_SpaceCharacters[i] = false;

  /* Initialise the busy cursor from data in the application's resources. */

  if ((ResourcesPntr = AppResources ()) != NULL && (ResourceData =
  ResourcesPntr->LoadResource ('CURS', "Busy Cursor", &ResourceSize)) != NULL
  && ResourceSize >= 68 /* Size of a raw 2x16x16x8+4 cursor is 68 bytes */)
    g_BusyCursor = new BCursor (ResourceData);

  /* Find out the smallest usable double by seeing how small we can make it. */

  m_SmallestUseableDouble = 1.0;
  HalvingCount = 0;
  while (HalvingCount < 10000 && m_SmallestUseableDouble > 0.0)
  {
    HalvingCount++;
    m_SmallestUseableDouble /= 2;
  }

  /* Recreate the number.  But don't make quite as small, we want to allow some
  precision bits and a bit of extra margin for intermediate results in future
  calculations. */

  HalvingCount -= 50 + sizeof (double) * 8;

  m_SmallestUseableDouble = 1.0;
  while (HalvingCount > 0)
  {
    HalvingCount--;
    m_SmallestUseableDouble /= 2;
  }
}


ABSApp::~ABSApp ()
{
  status_t ErrorCode;
  char     ErrorMessage [PATH_MAX + 1024];

  if (m_SettingsHaveChanged)
    LoadSaveSettings (false /* DoLoad */);
  if ((ErrorCode = SaveDatabaseIfNeeded (ErrorMessage)) != B_OK)
    DisplayErrorMessage (ErrorMessage, ErrorCode, "Exiting Error");
  delete g_BusyCursor;
  g_BusyCursor = NULL;
}


/* Display a box showing information about this program. */

void ABSApp::AboutRequested ()
{
  BAlert *AboutAlertPntr;

  AboutAlertPntr = new BAlert ("About",
"SpamDBM - Spam Database Manager\n\n"

"This is a BeOS program for classifying e-mail messages as spam (unwanted \
junk mail) or as genuine mail using a Bayesian statistical approach.  There \
is also a Mail Daemon Replacement add-on to filter mail using the \
classification statistics collected earlier.\n\n"

"Written by Alexander G. M. Smith, fall 2002.\n\n"

"The original idea was from Paul Graham's algorithm, which has an excellent \
writeup at: http://www.paulgraham.com/spam.html\n\n"

"Gary Robinson came up with the improved algorithm, which you can read about \
at: http://radio.weblogs.com/0101454/stories/2002/09/16/spamDetection.html\n\n"

"Mr. Robinson, Tim Peters and the SpamBayes mailing list people then \
developed the even better chi-squared scoring method.\n\n"

"Icon courtesy of Isaac Yonemoto, though it is no longer used since Hormel \
doesn't want their meat product associated with junk e-mail.\n\n"

"Tokenising code updated in 2005 to use some of the tricks that SpamBayes \
uses to extract words from messages.  In particular, HTML is now handled.\n\n"

"Released to the public domain, with no warranty.\n"
"$Revision: 30630 $\n"
"Compiled on " __DATE__ " at " __TIME__ ".", "Done");
  if (AboutAlertPntr != NULL)
  {
    AboutAlertPntr->SetShortcut (0, B_ESCAPE);
    AboutAlertPntr->Go ();
  }
}


/* Add the text in the given file to the database as an example of a spam or
genuine message, or removes it from the database if you claim it is
CL_UNCERTAIN.  Also resets the spam ratio attribute to show the effect of the
database change. */

status_t ABSApp::AddFileToDatabase (
  ClassificationTypes IsSpamOrWhat,
  const char *FileName,
  char *ErrorMessage)
{
  status_t ErrorCode;
  BFile    MessageFile;
  BMessage TempBMessage;

  ErrorCode = MessageFile.SetTo (FileName, B_READ_ONLY);
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "Unable to open file \"%s\" for reading", FileName);
    return ErrorCode;
  }

  ErrorCode = AddPositionIOToDatabase (IsSpamOrWhat,
    &MessageFile, FileName, ErrorMessage);
  MessageFile.Unset ();
  if (ErrorCode != B_OK)
    return ErrorCode;

  /* Re-evaluate the file so that the user sees the new ratio attribute. */
  return EvaluateFile (FileName, &TempBMessage, ErrorMessage);
}


/* Add the given text to the database.  The unique words found in MessageIOPntr
will be added to the database (incrementing the count for the number of
messages using each word, either the spam or genuine count depending on
IsSpamOrWhat).  It will remove the message (decrement the word counts) if you
specify CL_UNCERTAIN as the new classification.  And if it switches from spam
to genuine or vice versa, it will do both - decrement the counts for the old
class and increment the counts for the new one.  An attribute will be added to
MessageIOPntr (if it is a file) to record that it has been marked as Spam or
Genuine (so that it doesn't get added to the database a second time).  If it is
being removed from the database, the classification attribute gets removed too.
If things go wrong, a non-zero error code will be returned and an explanation
written to ErrorMessage (assumed to be at least PATH_MAX + 1024 bytes long).
OptionalFileName is just used in the error message to identify the file to the
user. */

status_t ABSApp::AddPositionIOToDatabase (
  ClassificationTypes IsSpamOrWhat,
  BPositionIO *MessageIOPntr,
  const char *OptionalFileName,
  char *ErrorMessage)
{
  BNode                             *BNodePntr;
  char                               ClassificationString [NAME_MAX];
  StatisticsMap::iterator            DataIter;
  status_t                           ErrorCode = 0;
  pair<StatisticsMap::iterator,bool> InsertResult;
  uint32                             NewAge;
  StatisticsRecord                   NewStatistics;
  ClassificationTypes                PreviousClassification;
  StatisticsPointer                  StatisticsPntr;
  set<string>::iterator              WordEndIter;
  set<string>::iterator              WordIter;
  set<string>                        WordSet;

  NewAge = m_TotalGenuineMessages + m_TotalSpamMessages;
  if (NewAge >= 0xFFFFFFF0UL)
  {
    sprintf (ErrorMessage, "The database is full!  There are %lu messages in "
      "it and we can't add any more without overflowing the maximum integer "
      "representation in 32 bits", NewAge);
    return B_NO_MEMORY;
  }

  /* Check that this file hasn't already been added to the database. */

  PreviousClassification = CL_UNCERTAIN;
  BNodePntr = dynamic_cast<BNode *> (MessageIOPntr);
  if (BNodePntr != NULL) /* If this thing might have attributes. */
  {
    ErrorCode = BNodePntr->ReadAttr (g_AttributeNameClassification,
      B_STRING_TYPE, 0 /* offset */, ClassificationString,
      sizeof (ClassificationString) - 1);
    if (ErrorCode <= 0) /* Positive values for the number of bytes read */
      strcpy (ClassificationString, "none");
    else /* Just in case it needs a NUL at the end. */
      ClassificationString [ErrorCode] = 0;

    if (strcasecmp (ClassificationString, g_ClassifiedSpam) == 0)
      PreviousClassification = CL_SPAM;
    else if (strcasecmp (ClassificationString, g_ClassifiedGenuine) == 0)
      PreviousClassification = CL_GENUINE;
  }

  if (!m_IgnorePreviousClassification &&
  PreviousClassification != CL_UNCERTAIN)
  {
    if (IsSpamOrWhat == PreviousClassification)
    {
      sprintf (ErrorMessage, "Ignoring file \"%s\" since it seems to have "
        "already been classified as %s.", OptionalFileName,
        g_ClassificationTypeNames [IsSpamOrWhat]);
    }
    else
    {
      sprintf (ErrorMessage, "Changing existing classification of file \"%s\" "
        "from %s to %s.", OptionalFileName,
        g_ClassificationTypeNames [PreviousClassification],
        g_ClassificationTypeNames [IsSpamOrWhat]);
    }
    DisplayErrorMessage (ErrorMessage, 0, "Note");
  }

  if (!m_IgnorePreviousClassification &&
  IsSpamOrWhat == PreviousClassification)
    /* Nothing to do if it is already classified correctly and the user doesn't
    want double classification. */
    return B_OK;

  /* Get the list of unique words in the file. */

  ErrorCode = GetWordsFromPositionIO (MessageIOPntr, OptionalFileName,
    WordSet, ErrorMessage);
  if (ErrorCode != B_OK)
    return ErrorCode;

  /* Update the count of the number of messages processed, with corrections if
  reclassifying a message. */

  m_DatabaseHasChanged = true;

  if (!m_IgnorePreviousClassification &&
  PreviousClassification == CL_SPAM && m_TotalSpamMessages > 0)
    m_TotalSpamMessages--;

  if (IsSpamOrWhat == CL_SPAM)
    m_TotalSpamMessages++;

  if (!m_IgnorePreviousClassification &&
  PreviousClassification == CL_GENUINE && m_TotalGenuineMessages > 0)
      m_TotalGenuineMessages--;

  if (IsSpamOrWhat == CL_GENUINE)
    m_TotalGenuineMessages++;

  /* Mark the file's attributes with the new classification.  Don't care if it
  fails. */

  if (BNodePntr != NULL) /* If this thing might have attributes. */
  {
    ErrorCode = BNodePntr->RemoveAttr (g_AttributeNameClassification);
    if (IsSpamOrWhat != CL_UNCERTAIN)
    {
      strcpy (ClassificationString, g_ClassificationTypeNames [IsSpamOrWhat]);
      ErrorCode = BNodePntr->WriteAttr (g_AttributeNameClassification,
        B_STRING_TYPE, 0 /* offset */,
        ClassificationString, strlen (ClassificationString) + 1);
    }
  }

  /* Add the words to the database by incrementing or decrementing the counts
  for each word as appropriate. */

  WordEndIter = WordSet.end ();
  for (WordIter = WordSet.begin (); WordIter != WordEndIter; WordIter++)
  {
    if ((DataIter = m_WordMap.find (*WordIter)) == m_WordMap.end ())
    {
      /* No record in the database for the word. */

      if (IsSpamOrWhat == CL_UNCERTAIN)
        continue; /* Not adding words, don't have to subtract from nothing. */

      /* Create a new one record in the database for the new word. */

      memset (&NewStatistics, 0, sizeof (NewStatistics));
      InsertResult = m_WordMap.insert (
        StatisticsMap::value_type (*WordIter, NewStatistics));
      if (!InsertResult.second)
      {
        sprintf (ErrorMessage, "Failed to insert new database entry for "
          "word \"%s\", while processing file \"%s\"",
          WordIter->c_str (), OptionalFileName);
        return B_NO_MEMORY;
      }
      DataIter = InsertResult.first;
      m_WordCount++;
    }

    /* Got the database record for the word, update the statistics. */

    StatisticsPntr = &DataIter->second;

    StatisticsPntr->age = NewAge;

    /* Can't update m_OldestAge here, since it would take a lot of effort to
    find the next older age.  Since it's only used for display, we'll let it be
    slightly incorrect.  The next database load or purge will fix it. */

    if (IsSpamOrWhat == CL_SPAM)
      StatisticsPntr->spamCount++;

    if (IsSpamOrWhat == CL_GENUINE)
      StatisticsPntr->genuineCount++;

    if (!m_IgnorePreviousClassification &&
    PreviousClassification == CL_SPAM && StatisticsPntr->spamCount > 0)
      StatisticsPntr->spamCount--;

    if (!m_IgnorePreviousClassification &&
    PreviousClassification == CL_GENUINE && StatisticsPntr->genuineCount > 0)
      StatisticsPntr->genuineCount--;
  }

  return B_OK;
}


/* Add the text in the string to the database as an example of a spam or
genuine message. */

status_t ABSApp::AddStringToDatabase (
  ClassificationTypes IsSpamOrWhat,
  const char *String,
  char *ErrorMessage)
{
  BMemoryIO MemoryIO (String, strlen (String));

  return AddPositionIOToDatabase (IsSpamOrWhat, &MemoryIO,
   "Memory Buffer" /* OptionalFileName */, ErrorMessage);
}


/* Given a bunch of text, find the words within it (doing special tricks to
extract words from HTML), and add them to the set.  Allow NULs in the text.  If
the PrefixCharacter isn't zero then it is prepended to all words found (so you
can distinguish words as being from a header or from the body text).  See also
TokenizeWhole which does something similar. */

void ABSApp::AddWordsToSet (
  const char *InputString,
  size_t NumberOfBytes,
  char PrefixCharacter,
  set<string> &WordSet)
{
  char   *BufferPntr;
  size_t  CurrentSize;
  int     PassNumber;

  /* Copy the input buffer.  The code will be modifying it in-place as HTML
  fragments and other junk are deleted. */

  BufferPntr = new char [NumberOfBytes];
  if (BufferPntr == NULL)
    return;
  memcpy (BufferPntr, InputString, NumberOfBytes);

  /* Do the tokenization.  Each pass does something to the text in the buffer,
  and may add words to the word set. */

  CurrentSize = NumberOfBytes;
  for (PassNumber = 1; PassNumber <= 8 && CurrentSize > 0 ; PassNumber++)
  {
    switch (PassNumber)
    {
      case 1: /* Lowercase first, rest of them assume lower case inputs. */
        CurrentSize = TokenizerPassLowerCase (BufferPntr, CurrentSize);
        break;
      case 2: CurrentSize = TokenizerPassJapanesePeriodsToSpaces (
        BufferPntr, CurrentSize, PrefixCharacter, WordSet); break;
      case 3: CurrentSize = TokenizerPassTruncateLongAsianWords (
        BufferPntr, CurrentSize); break;
      case 4: CurrentSize = TokenizerPassRemoveHTMLComments (
        BufferPntr, CurrentSize, 'Z', WordSet); break;
      case 5: CurrentSize = TokenizerPassRemoveHTMLStyle (
        BufferPntr, CurrentSize, 'Z', WordSet); break;
      case 6: CurrentSize = TokenizerPassExtractURLs (
        BufferPntr, CurrentSize, 'Z', WordSet); break;
      case 7: CurrentSize = TokenizerPassRemoveHTMLTags (
        BufferPntr, CurrentSize, 'Z', WordSet); break;
      case 8: CurrentSize = TokenizerPassGetPlainWords (
        BufferPntr, CurrentSize, PrefixCharacter, WordSet); break;
      default: break;
    }
  }

  delete [] BufferPntr;
}


/* The user has provided a command line.  This could actually be from a
separate attempt to invoke the program (this application's resource/attributes
have the launch flags set to "single launch", so the shell doesn't start the
program but instead sends the arguments to the already running instance).  In
either case, the command is sent to an intermediary thread where it is
asynchronously converted into a scripting message(s) that are sent back to this
BApplication.  The intermediary is needed since we can't recursively execute
scripting messages while processing a message (this ArgsReceived one). */

void ABSApp::ArgvReceived (int32 argc, char **argv)
{
  if (g_CommanderLooperPntr != NULL)
    g_CommanderLooperPntr->CommandArguments (argc, argv);
}


/* Create a new empty database.  Note that we have to write out the new file
immediately, otherwise other operations will see the empty database and then
try to load the file, and complain that it doesn't exist.  Now they will see
the empty database and redundantly load the empty file. */

status_t ABSApp::CreateDatabaseFile (char *ErrorMessage)
{
  MakeDatabaseEmpty ();
  m_DatabaseHasChanged = true;
  return SaveDatabaseIfNeeded (ErrorMessage); /* Make it now. */
}


/* Set the settings to the defaults.  Needed in case there isn't a settings
file or it is obsolete. */

void ABSApp::DefaultSettings ()
{
  status_t ErrorCode;
  BPath    DatabasePath (m_SettingsDirectoryPath);
  char     TempString [PATH_MAX];

  /* The default database file is in the settings directory. */

  ErrorCode = DatabasePath.Append (g_DefaultDatabaseFileName);
  if (ErrorCode != B_OK)
    strcpy (TempString, g_DefaultDatabaseFileName); /* Unlikely to happen. */
  else
    strcpy (TempString, DatabasePath.Path ());
  m_DatabaseFileName.SetTo (TempString);

  // Users need to be allowed to undo their mistakes...
  m_IgnorePreviousClassification = true;
  g_ServerMode = true;
  m_PurgeAge = 2000;
  m_PurgePopularity = 2;
  m_ScoringMode = SM_CHISQUARED;
  m_TokenizeMode = TM_ANY_TEXT_HEADER;

  m_SettingsHaveChanged = true;
}


/* Deletes the database file, and the backup file, and clears the database but
marks it as not changed so that it doesn't get written out when the program
exits. */

status_t ABSApp::DeleteDatabaseFile (char *ErrorMessage)
{
  BEntry   FileEntry;
  status_t ErrorCode;
  int      i;
  char     TempString [PATH_MAX+20];

  /* Clear the in-memory database. */

  MakeDatabaseEmpty ();
  m_DatabaseHasChanged = false;

  /* Delete the backup files first.  Don't care if it fails. */

  for (i = 0; i < g_MaxBackups; i++)
  {
    strcpy (TempString, m_DatabaseFileName.String ());
    sprintf (TempString + strlen (TempString), g_BackupSuffix, i);
    ErrorCode = FileEntry.SetTo (TempString);
    if (ErrorCode == B_OK)
      FileEntry.Remove ();
  }

  /* Delete the main database file. */

  strcpy (TempString, m_DatabaseFileName.String ());
  ErrorCode = FileEntry.SetTo (TempString);
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "While deleting, failed to make BEntry for "
      "\"%s\" (does the directory exist?)", TempString);
    return ErrorCode;
  }

  ErrorCode = FileEntry.Remove ();
  if (ErrorCode != B_OK)
    sprintf (ErrorMessage, "While deleting, failed to remove file "
      "\"%s\"", TempString);

  return ErrorCode;
}


/* Evaluate the given file as being a spam message, and tag it with the
resulting spam probability ratio.  If it also has an e-mail subject attribute,
remove the [Spam 99.9%] prefix since the number usually changes. */

status_t ABSApp::EvaluateFile (
  const char *PathName,
  BMessage *ReplyMessagePntr,
  char *ErrorMessage)
{
  status_t ErrorCode;
  float    TempFloat;
  BFile    TextFile;

  /* Open the specified file. */

  ErrorCode = TextFile.SetTo (PathName, B_READ_ONLY);
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "Problems opening file \"%s\" for evaluating",
      PathName);
    return ErrorCode;
  }

  ErrorCode =
    EvaluatePositionIO (&TextFile, PathName, ReplyMessagePntr, ErrorMessage);

  if (ErrorCode == B_OK &&
  ReplyMessagePntr->FindFloat (g_ResultName, &TempFloat) == B_OK)
  {
    TextFile.WriteAttr (g_AttributeNameSpamRatio, B_FLOAT_TYPE,
      0 /* offset */, &TempFloat, sizeof (TempFloat));
    /* Don't know the spam cutoff ratio, that's in the e-mail filter, so just
    blindly remove the prefix, which would have the wrong percentage. */
    RemoveSpamPrefixFromSubjectAttribute (&TextFile);
  }

  return ErrorCode;
}


/* Evaluate a given file or memory buffer (a BPositionIO handles both cases)
for spaminess.  The output is added to the ReplyMessagePntr message, with the
probability ratio stored in "result" (0.0 means genuine and 1.0 means spam).
It also adds the most significant words (used in the ratio calculation) to the
array "words" and the associated per-word probability ratios in "ratios".  If
it fails, an error code is returned and an error message written to the
ErrorMessage string (which is at least MAX_PATH + 1024 bytes long).
OptionalFileName is only used in the error message.

The math used for combining the individual word probabilities in my method is
based on Gary Robinson's method (formerly it was a variation of Paul Graham's
method) or the Chi-Squared method.  It's input is the database of words that
has a count of the number of spam and number of genuine messages each word
appears in (doesn't matter if it appears more than once in a message, it still
counts as 1).

The spam word count is divided the by the total number of spam e-mail messages
in the database to get the probability of spam and probability of genuineness
is similarly computed for a particular word.  The spam probability is divided
by the sum of the spam and genuine probabilities to get the Raw Spam Ratio for
the word.  It's nearer to 0.0 for genuine and nearer to 1.0 for spam, and can
be exactly zero or one too.

To avoid multiplying later results by zero, and to compensate for a lack of
data points, the Raw Spam Ratio is adjusted towards the 0.5 halfway point.  The
0.5 is combined with the raw spam ratio, with a weight of 0.45 (determined to
be a good value by the "spambayes" mailing list tests) messages applied to the
half way point and a weight of the number of spam + genuine messages applied to
the raw spam ratio.  This gives you the compensated spam ratio for the word.

The top N (150 was good in the spambayes tests) extreme words are selected by
the distance of each word's compensated spam ratio from 0.5.  Then the ratios
of the words are combined.

The Gary Robinson combining (scoring) method gets one value from the Nth root
of the product of all the word ratios.  The other is the Nth root of the
product of (1 - ratio) for all the words.  The final result is the first value
divided by the sum of the two values.  The Nth root helps spread the resulting
range of values more evenly between 0.0 and 1.0, otherwise the values all clump
together at 0 or 1.  Also you can think of the Nth root as a kind of average
for products; it's like a generic word probability which when multiplied by
itself N times gives you the same result as the N separate actual word
probabilities multiplied together.

The Chi-Squared combining (scoring) method assumes that the spam word
probabilities are uniformly distributed and computes an error measurement
(called chi squared - see http://bmj.com/collections/statsbk/8.shtml for a good
tutorial) and then sees how likely that error value would be observed in
practice.  If it's rare to observe, then the words are likely not just randomly
occuring and it's spammy.  The same is done for genuine words.  The two
resulting unlikelynesses are compared to see which is more unlikely, if neither
is, then the method says it can't decide.  The SpamBayes notes (see the
classifier.py file in CVS in http://sourceforge.net/projects/spambayes) say:

"Across vectors of length n, containing random uniformly-distributed
probabilities, -2*sum(ln(p_i)) follows the chi-squared distribution with 2*n
degrees of freedom.  This has been proven (in some appropriate sense) to be the
most sensitive possible test for rejecting the hypothesis that a vector of
probabilities is uniformly distributed.  Gary Robinson's original scheme was
monotonic *with* this test, but skipped the details.  Turns out that getting
closer to the theoretical roots gives a much sharper classification, with a
very small (in # of msgs), but also very broad (in range of scores), "middle
ground", where most of the mistakes live.  In particular, this scheme seems
immune to all forms of "cancellation disease": if there are many strong ham
*and* spam clues, this reliably scores close to 0.5.  Most other schemes are
extremely certain then -- and often wrong."

I did a test with 448 example genuine messages including personal mail (some
with HTML attachments) and mailing lists, and 267 spam messages for 27471 words
total.  Test messages were more recent messages in the same groups.  Out of 100
test genuine messages, with Gary Robinson (0.56 cutoff limit), 1 (1%) was
falsely identified as spam and 8 of 73 (11%) spam messages were incorrectly
classified as genuine.  With my variation of Paul Graham's scheme (0.90 cutoff)
I got 6 of 100 (6%) genuine messages incorrectly marked as spam and 2 of 73
(3%) spam messages were incorrectly classified as genuine.  Pretty close, but
Robinson's values are more evenly spread out so you can tell just how spammy it
is by looking at the number. */

struct WordAndRatioStruct
{
  double        probabilityRatio; /* Actually the compensated ratio. */
  const string *wordPntr;

  bool operator() ( /* Our less-than comparison function for sorting. */
    const WordAndRatioStruct &ItemA,
    const WordAndRatioStruct &ItemB) const
  {
    return
      (fabs (ItemA.probabilityRatio - 0.5) <
      fabs (ItemB.probabilityRatio - 0.5));
  };
};

status_t ABSApp::EvaluatePositionIO (
  BPositionIO *PositionIOPntr,
  const char *OptionalFileName,
  BMessage *ReplyMessagePntr,
  char *ErrorMessage)
{
  StatisticsMap::iterator            DataEndIter;
  StatisticsMap::iterator            DataIter;
  status_t                           ErrorCode;
  double                             GenuineProbability;
  uint32                             GenuineSpamSum;
  int                                i;
  priority_queue<
    WordAndRatioStruct /* Data type stored in the queue */,
    vector<WordAndRatioStruct> /* Underlying container */,
    WordAndRatioStruct /* Function for comparing elements */>
                                     PriorityQueue;
  double                             ProductGenuine;
  double                             ProductLogGenuine;
  double                             ProductLogSpam;
  double                             ProductSpam;
  double                             RawProbabilityRatio;
  float                              ResultRatio;
  double                             SpamProbability;
  StatisticsPointer                  StatisticsPntr;
  double                             TempDouble;
  double                             TotalGenuine;
  double                             TotalSpam;
  WordAndRatioStruct                 WordAndRatio;
  set<string>::iterator              WordEndIter;
  set<string>::iterator              WordIter;
  const WordAndRatioStruct          *WordRatioPntr;
  set<string>                        WordSet;

  /* Get the list of unique words in the file / memory buffer. */

  ErrorCode = GetWordsFromPositionIO (PositionIOPntr, OptionalFileName,
    WordSet, ErrorMessage);
  if (ErrorCode != B_OK)
    return ErrorCode;

  /* Prepare a few variables.  Mostly these are stored double values of some of
  the numbers involved (to avoid the overhead of multiple conversions from
  integer to double), with extra precautions to avoid divide by zero. */

  if (m_TotalGenuineMessages <= 0)
    TotalGenuine = 1.0;
  else
    TotalGenuine = m_TotalGenuineMessages;

  if (m_TotalSpamMessages <= 0)
    TotalSpam = 1.0;
  else
    TotalSpam = m_TotalSpamMessages;

  /* Look up the words in the database and calculate their compensated spam
  ratio.  The results are stored in a priority queue so that we can later find
  the top g_MaxInterestingWords for doing the actual determination. */

  WordEndIter = WordSet.end ();
  DataEndIter = m_WordMap.end ();
  for (WordIter = WordSet.begin (); WordIter != WordEndIter; WordIter++)
  {
    WordAndRatio.wordPntr = &(*WordIter);

    if ((DataIter = m_WordMap.find (*WordIter)) != DataEndIter)
    {
      StatisticsPntr = &DataIter->second;

      /* Calculate the probability the word is spam and the probability it is
      genuine.  Then the raw probability ratio. */

      SpamProbability = StatisticsPntr->spamCount / TotalSpam;
      GenuineProbability = StatisticsPntr->genuineCount / TotalGenuine;

      if (SpamProbability + GenuineProbability > 0)
        RawProbabilityRatio =
        SpamProbability / (SpamProbability + GenuineProbability);
      else /* Word with zero statistics, perhaps due to reclassification. */
        RawProbabilityRatio = 0.5;

      /* The compensated ratio leans towards 0.5 (g_RobinsonX) more for fewer
      data points, with a weight of 0.45 (g_RobinsonS). */

      GenuineSpamSum =
        StatisticsPntr->spamCount + StatisticsPntr->genuineCount;

      WordAndRatio.probabilityRatio =
        (g_RobinsonS * g_RobinsonX + GenuineSpamSum * RawProbabilityRatio) /
        (g_RobinsonS + GenuineSpamSum);
    }
    else /* Unknown word. With N=0, compensated ratio equation is RobinsonX. */
      WordAndRatio.probabilityRatio = g_RobinsonX;

     PriorityQueue.push (WordAndRatio);
  }

  /* Compute the combined probability (multiply them together) of the top few
  words.  To avoid numeric underflow (doubles can only get as small as 1E-300),
  logarithms are also used.  But avoid the logarithms (sum of logs of numbers
  is the same as the product of numbers) as much as possible due to reduced
  accuracy and slowness. */

  ProductGenuine = 1.0;
  ProductLogGenuine = 0.0;
  ProductSpam = 1.0;
  ProductLogSpam = 0.0;
  for (i = 0;
  i < g_MaxInterestingWords && !PriorityQueue.empty();
  i++, PriorityQueue.pop())
  {
    WordRatioPntr = &PriorityQueue.top();
    ProductSpam *= WordRatioPntr->probabilityRatio;
    ProductGenuine *= 1.0 - WordRatioPntr->probabilityRatio;

    /* Check for the numbers getting dangerously small, close to underflowing.
    If they are, move the value into the logarithm storage part. */

    if (ProductSpam < m_SmallestUseableDouble)
    {
      ProductLogSpam += log (ProductSpam);
      ProductSpam = 1.0;
    }

    if (ProductGenuine < m_SmallestUseableDouble)
    {
      ProductLogGenuine += log (ProductGenuine);
      ProductGenuine = 1.0;
    }

    ReplyMessagePntr->AddString ("words", WordRatioPntr->wordPntr->c_str ());
    ReplyMessagePntr->AddFloat ("ratios", WordRatioPntr->probabilityRatio);
  }

  /* Get the resulting log of the complete products. */

  if (i > 0)
  {
    ProductLogSpam += log (ProductSpam);
    ProductLogGenuine += log (ProductGenuine);
  }

  if (m_ScoringMode == SM_ROBINSON)
  {
    /* Apply Gary Robinson's scoring method where we take the Nth root of the
    products.  This is easiest in logarithm form. */

    if (i > 0)
    {
      ProductSpam = exp (ProductLogSpam / i);
      ProductGenuine = exp (ProductLogGenuine / i);
      ResultRatio = ProductSpam / (ProductGenuine + ProductSpam);
    }
    else /* Somehow got no words! */
      ResultRatio = g_RobinsonX;
  }
  else if (m_ScoringMode == SM_CHISQUARED)
  {
    /* From the SpamBayes notes: "We compute two chi-squared statistics, one
    for ham and one for spam.  The sum-of-the-logs business is more sensitive
    to probs near 0 than to probs near 1, so the spam measure uses 1-p (so that
    high-spamprob words have greatest effect), and the ham measure uses p
    directly (so that lo-spamprob words have greatest effect)."  That means we
    just reversed the meaning of the previously calculated spam and genuine
    products!  Oh well. */

    TempDouble = ProductLogSpam;
    ProductLogSpam = ProductLogGenuine;
    ProductLogGenuine = TempDouble;

    if (i > 0)
    {
      ProductSpam =
        1.0 - ChiSquaredProbability (-2.0 * ProductLogSpam, 2 * i);
      ProductGenuine =
        1.0 - ChiSquaredProbability (-2.0 * ProductLogGenuine, 2 * i);

      /* The SpamBayes notes say: "How to combine these into a single spam
      score?  We originally used (S-H)/(S+H) scaled into [0., 1.], which equals
      S/(S+H).  A systematic problem is that we could end up being near-certain
      a thing was (for example) spam, even if S was small, provided that H was
      much smaller.  Rob Hooft stared at these problems and invented the
      measure we use now, the simpler S-H, scaled into [0., 1.]." */

      ResultRatio = (ProductSpam - ProductGenuine + 1.0) / 2.0;
    }
    else /* No words to analyse. */
      ResultRatio = 0.5;
  }
  else /* Unknown scoring mode. */
  {
    strcpy (ErrorMessage, "Unknown scoring mode specified in settings");
    return B_BAD_VALUE;
  }

  ReplyMessagePntr->AddFloat (g_ResultName, ResultRatio);
  return B_OK;
}


/* Just evaluate the given string as being spam text. */

status_t ABSApp::EvaluateString (
  const char *BufferPntr,
  ssize_t BufferSize,
  BMessage *ReplyMessagePntr,
  char *ErrorMessage)
{
  BMemoryIO MemoryIO (BufferPntr, BufferSize);

  return EvaluatePositionIO (&MemoryIO, "Memory Buffer",
    ReplyMessagePntr, ErrorMessage);
}


/* Tell other programs about the scripting commands we support.  Try this
command: "hey application/x-vnd.agmsmith.spamdbm getsuites" to
see it in action (this program has to be already running for it to work). */

status_t ABSApp::GetSupportedSuites (BMessage *MessagePntr)
{
  BPropertyInfo TempPropInfo (g_ScriptingPropertyList);

  MessagePntr->AddString ("suites", "suite/x-vnd.agmsmith.spamdbm");
  MessagePntr->AddFlat ("messages", &TempPropInfo);
  return BApplication::GetSupportedSuites (MessagePntr);
}


/* Add all the words in the given file or memory buffer to the supplied set.
The file name is only there for error messages, it assumes you have already
opened the PositionIO to the right file.  If things go wrong, a non-zero error
code will be returned and an explanation written to ErrorMessage (assumed to be
at least PATH_MAX + 1024 bytes long). */

status_t ABSApp::GetWordsFromPositionIO (
  BPositionIO *PositionIOPntr,
  const char *OptionalFileName,
  set<string> &WordSet,
  char *ErrorMessage)
{
  status_t ErrorCode;

  if (m_TokenizeMode == TM_WHOLE)
    ErrorCode = TokenizeWhole (PositionIOPntr, OptionalFileName,
      WordSet, ErrorMessage);
  else
    ErrorCode = TokenizeParts (PositionIOPntr, OptionalFileName,
      WordSet, ErrorMessage);

  if (ErrorCode == B_OK && WordSet.empty ())
  {
    /* ENOMSG usually means no message found in queue, but I'm using it to show
    no words, a good indicator of spam which is pure HTML. */

    sprintf (ErrorMessage, "No words were found in \"%s\"", OptionalFileName);
    ErrorCode = ENOMSG;
  }

  return ErrorCode;
}


/* Set up indices for attributes MAIL:classification (string) and
MAIL:ratio_spam (float) on all mounted disk volumes that support queries.  Also
tell the system to make those attributes visible to the user (so they can see
them in Tracker) and associate them with e-mail messages.  Also set up the
database file MIME type (provide a description and associate it with this
program so that it picks up the right icon).  And register the names for our
sound effects. */

status_t ABSApp::InstallThings (char *ErrorMessage)
{
  int32       Cookie;
  dev_t       DeviceID;
  status_t    ErrorCode = B_OK;
  fs_info     FSInfo;
  int32       i;
  int32       iClassification;
  int32       iProbability;
  int32       j;
  index_info  IndexInfo;
  BMimeType   MimeType;
  BMessage    Parameters;
  const char *StringPntr;
  bool        TempBool;
  int32       TempInt32;

  /* Iterate through all mounted devices and try to make the indices on each
  one.  Don't bother if the index exists or the device doesn't support indices
  (actually queries). */

  Cookie = 0;
  while ((DeviceID = next_dev (&Cookie)) >= 0)
  {
    if (!fs_stat_dev (DeviceID, &FSInfo) && (FSInfo.flags & B_FS_HAS_QUERY))
    {
      if (fs_stat_index (DeviceID, g_AttributeNameClassification, &IndexInfo)
      && errno == B_ENTRY_NOT_FOUND)
      {
        if (fs_create_index (DeviceID, g_AttributeNameClassification,
        B_STRING_TYPE, 0 /* flags */))
        {
          ErrorCode = errno;
          sprintf (ErrorMessage, "Unable to make string index %s on "
            "volume #%d, volume name \"%s\", file system type \"%s\", "
            "on device \"%s\"", g_AttributeNameClassification,
            (int) DeviceID, FSInfo.volume_name, FSInfo.fsh_name,
            FSInfo.device_name);
        }
      }

      if (fs_stat_index (DeviceID, g_AttributeNameSpamRatio,
      &IndexInfo) && errno == B_ENTRY_NOT_FOUND)
      {
        if (fs_create_index (DeviceID, g_AttributeNameSpamRatio,
        B_FLOAT_TYPE, 0 /* flags */))
        {
          ErrorCode = errno;
          sprintf (ErrorMessage, "Unable to make float index %s on "
            "volume #%d, volume name \"%s\", file system type \"%s\", "
            "on device \"%s\"", g_AttributeNameSpamRatio,
            (int) DeviceID, FSInfo.volume_name, FSInfo.fsh_name,
            FSInfo.device_name);
        }
      }
    }
  }
  if (ErrorCode != B_OK)
    return ErrorCode;

  /* Set up the MIME types for the classification attributes, associate them
  with e-mail and make them visible to the user (but not editable).  First need
  to get the existing MIME settings, then add ours to them (otherwise the
  existing ones get wiped out). */

  ErrorCode = MimeType.SetTo ("text/x-email");
  if (ErrorCode != B_OK || !MimeType.IsInstalled ())
  {
    sprintf (ErrorMessage, "No e-mail MIME type (%s) in the system, can't "
      "update it to add our special attributes, and without e-mail this "
      "program is useless!", MimeType.Type ());
    if (ErrorCode == B_OK)
      ErrorCode = -1;
    return ErrorCode;
  }

  ErrorCode = MimeType.GetAttrInfo (&Parameters);
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "Unable to retrieve list of attributes "
      "associated with e-mail messages in the MIME database");
    return ErrorCode;
  }

  for (i = 0, iClassification = -1, iProbability = -1;
  i < 1000 && (iClassification < 0 || iProbability < 0);
  i++)
  {
    ErrorCode = Parameters.FindString ("attr:name", i, &StringPntr);
    if (ErrorCode != B_OK)
      break; /* Reached the end of the attributes. */
    if (strcmp (StringPntr, g_AttributeNameClassification) == 0)
      iClassification = i;
    else if (strcmp (StringPntr, g_AttributeNameSpamRatio) == 0)
      iProbability = i;
  }

  /* Add extra default settings for those programs which previously didn't
  update the MIME database with all the attributes that exist (so our new
  additions don't show up at the wrong index). */

  i--; /* Set i to index of last valid attribute. */

  for (j = 0; j <= i; j++)
  {
    if (Parameters.FindString ("attr:public_name", j, &StringPntr) ==
    B_BAD_INDEX)
    {
      if (Parameters.FindString ("attr:name", j, &StringPntr) != B_OK)
        StringPntr = "None!";
      Parameters.AddString ("attr:public_name", StringPntr);
    }
  }

  while (Parameters.FindInt32 ("attr:type", i, &TempInt32) == B_BAD_INDEX)
    Parameters.AddInt32 ("attr:type", B_STRING_TYPE);

  while (Parameters.FindBool ("attr:viewable", i, &TempBool) == B_BAD_INDEX)
    Parameters.AddBool ("attr:viewable", true);

  while (Parameters.FindBool ("attr:editable", i, &TempBool) == B_BAD_INDEX)
    Parameters.AddBool ("attr:editable", false);

  while (Parameters.FindInt32 ("attr:width", i, &TempInt32) == B_BAD_INDEX)
    Parameters.AddInt32 ("attr:width", 60);

  while (Parameters.FindInt32 ("attr:alignment", i, &TempInt32) == B_BAD_INDEX)
    Parameters.AddInt32 ("attr:alignment", B_ALIGN_LEFT);

  while (Parameters.FindBool ("attr:extra", i, &TempBool) == B_BAD_INDEX)
    Parameters.AddBool ("attr:extra", false);

  /* Add our new attributes to e-mail related things, if not already there. */

  if (iClassification < 0)
  {
    Parameters.AddString ("attr:name", g_AttributeNameClassification);
    Parameters.AddString ("attr:public_name", "Classification Group");
    Parameters.AddInt32 ("attr:type", B_STRING_TYPE);
    Parameters.AddBool ("attr:viewable", true);
    Parameters.AddBool ("attr:editable", false);
    Parameters.AddInt32 ("attr:width", 45);
    Parameters.AddInt32 ("attr:alignment", B_ALIGN_LEFT);
    Parameters.AddBool ("attr:extra", false);
  }

  if (iProbability < 0)
  {
    Parameters.AddString ("attr:name", g_AttributeNameSpamRatio);
    Parameters.AddString ("attr:public_name", "Spam/Genuine Estimate");
    Parameters.AddInt32 ("attr:type", B_FLOAT_TYPE);
    Parameters.AddBool ("attr:viewable", true);
    Parameters.AddBool ("attr:editable", false);
    Parameters.AddInt32 ("attr:width", 50);
    Parameters.AddInt32 ("attr:alignment", B_ALIGN_LEFT);
    Parameters.AddBool ("attr:extra", false);
  }

  if (iClassification < 0 || iProbability < 0)
  {
    ErrorCode = MimeType.SetAttrInfo (&Parameters);
    if (ErrorCode != B_OK)
    {
      sprintf (ErrorMessage, "Unable to associate the classification "
        "attributes with e-mail messages in the MIME database");
      return ErrorCode;
    }
  }

  /* Set up the MIME type for the database file. */

  sprintf (ErrorMessage, "Problems with setting up MIME type (%s) for "
    "the database files", g_ABSDatabaseFileMIMEType); /* A generic message. */

  ErrorCode = MimeType.SetTo (g_ABSDatabaseFileMIMEType);
  if (ErrorCode != B_OK)
    return ErrorCode;

  MimeType.Delete ();
  ErrorCode = MimeType.Install ();
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "Failed to install MIME type (%s) in the system",
      MimeType.Type ());
    return ErrorCode;
  }

  MimeType.SetShortDescription ("Spam Database");
  MimeType.SetLongDescription ("Bayesian Statistical Database for "
    "Classifying Junk E-Mail");
  sprintf (ErrorMessage, "1.0 ('%s')", g_DatabaseRecognitionString);
  MimeType.SetSnifferRule (ErrorMessage);
  MimeType.SetPreferredApp (g_ABSAppSignature);

  /* Set up the names of the sound effects.  Later on the user can associate
  sound files with the names by using the Sounds preferences panel or the
  installsound command.  The MDR add-on filter will trigger these sounds. */

  add_system_beep_event (g_BeepGenuine);
  add_system_beep_event (g_BeepSpam);
  add_system_beep_event (g_BeepUncertain);

  return B_OK;
}


/* Load the database if it hasn't been loaded yet.  Otherwise do nothing. */

status_t ABSApp::LoadDatabaseIfNeeded (char *ErrorMessage)
{
  if (m_WordMap.empty ())
    return LoadSaveDatabase (true /* DoLoad */, ErrorMessage);

  return B_OK;
}


/* Either load the database of spam words (DoLoad is TRUE) from the file
specified in the settings, or write (DoLoad is FALSE) the database to it.  If
it doesn't exist (and its parent directories do exist) then it will be created
when saving.  If it doesn't exist when loading, the in-memory database will be
set to an empty one and an error will be returned with an explanation put into
ErrorMessage (should be big enough for a path name and a couple of lines of
text).

The database file format is a UTF-8 text file (well, there could be some
latin-1 characters and other junk in there - it just copies the bytes from the
e-mail messages directly), with tab characters to separate fields (so that you
can also load it into a spreadsheet).  The first line identifies the overall
file type.  The second lists pairs of classifications plus the number of
messages in each class.  Currently it is just Genuine and Spam, but for future
compatability, that could be followed by more classification pairs.  The
remaining lines each contain a word, the date it was last updated (actually
it's the number of messages in the database when the word was added, smaller
numbers mean it was updated longer ago), the genuine count and the spam count.
*/

status_t ABSApp::LoadSaveDatabase (bool DoLoad, char *ErrorMessage)
{
  time_t                             CurrentTime;
  FILE                              *DatabaseFile = NULL;
  BNode                              DatabaseNode;
  BNodeInfo                          DatabaseNodeInfo;
  StatisticsMap::iterator            DataIter;
  StatisticsMap::iterator            EndIter;
  status_t                           ErrorCode;
  int                                i;
  pair<StatisticsMap::iterator,bool> InsertResult;
  char                               LineString [10240];
  StatisticsRecord                   Statistics;
  const char                        *StringPntr;
  char                              *TabPntr;
  const char                        *WordPntr;

  if (DoLoad)
  {
    MakeDatabaseEmpty ();
    m_DatabaseHasChanged = false; /* In case of early error exit. */
  }
  else /* Saving the database, backup the old version on disk. */
  {
    ErrorCode = MakeBackup (ErrorMessage);
    if (ErrorCode != B_OK) /* Usually because the directory isn't there. */
      return ErrorCode;
  }

  DatabaseFile = fopen (m_DatabaseFileName.String (), DoLoad ? "rb" : "wb");
  if (DatabaseFile == NULL)
  {
    ErrorCode = errno;
    sprintf (ErrorMessage, "Can't open database file \"%s\" for %s",
      m_DatabaseFileName.String (), DoLoad ? "reading" : "writing");
    goto ErrorExit;
  }

  /* Process the first line, which identifies the file. */

  if (DoLoad)
  {
    sprintf (ErrorMessage, "Can't read first line of database file \"%s\", "
      "expected it to start with \"%s\"",
      m_DatabaseFileName.String (), g_DatabaseRecognitionString);
    ErrorCode = -1;

    if (fgets (LineString, sizeof (LineString), DatabaseFile) == NULL)
      goto ErrorExit;
    if (strncmp (LineString, g_DatabaseRecognitionString,
    strlen (g_DatabaseRecognitionString)) != 0)
      goto ErrorExit;
  }
  else /* Saving */
  {
    CurrentTime = time (NULL);
    if (fprintf (DatabaseFile, "%s V1 (word, age, genuine count, spam count)\t"
    "Written by SpamDBM $Revision: 30630 $\t"
    "Compiled on " __DATE__ " at " __TIME__ "\tThis file saved on %s",
    g_DatabaseRecognitionString, ctime (&CurrentTime)) <= 0)
    {
      ErrorCode = errno;
      sprintf (ErrorMessage, "Problems when writing to database file \"%s\"",
        m_DatabaseFileName.String ());
      goto ErrorExit;
    }
  }

  /* The second line lists the different classifications.  We just check to see
  that the first two are Genuine and Spam.  If there are others, they'll be
  ignored and lost when the database is saved. */

  if (DoLoad)
  {
    sprintf (ErrorMessage, "Can't read second line of database file \"%s\", "
      "expected it to list classifications %s and %s along with their totals",
      m_DatabaseFileName.String (), g_ClassifiedGenuine, g_ClassifiedSpam);
    ErrorCode = B_BAD_VALUE;

    if (fgets (LineString, sizeof (LineString), DatabaseFile) == NULL)
      goto ErrorExit;
    i = strlen (LineString);
    if (i > 0 && LineString[i-1] == '\n')
      LineString[i-1] = 0; /* Remove trailing line feed character. */

    /* Look for the title word at the start of the line. */

    TabPntr = LineString;
    for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
      ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

    if (strncmp (StringPntr, "Classifications", 15) != 0)
      goto ErrorExit;

    /* Look for the Genuine class and count. */

    for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
      ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

    if (strcmp (StringPntr, g_ClassifiedGenuine) != 0)
      goto ErrorExit;

    for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
      ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

    m_TotalGenuineMessages = atoll (StringPntr);

    /* Look for the Spam class and count. */

    for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
      ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

    if (strcmp (StringPntr, g_ClassifiedSpam) != 0)
      goto ErrorExit;

    for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
      ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

    m_TotalSpamMessages = atoll (StringPntr);
  }
  else /* Saving */
  {
    fprintf (DatabaseFile,
      "Classifications and total messages:\t%s\t%lu\t%s\t%lu\n",
      g_ClassifiedGenuine, m_TotalGenuineMessages,
      g_ClassifiedSpam, m_TotalSpamMessages);
  }

  /* The remainder of the file is the list of words and statistics.  Each line
  has a word, a tab, the time when the word was last changed in the database
  (sequence number of message addition, starts at 0 and goes up by one for each
  message added to the database), a tab then the number of messages in the
  first class (genuine) that had that word, then a tab, then the number of
  messages in the second class (spam) with that word, and so on. */

  if (DoLoad)
  {
    while (!feof (DatabaseFile))
    {
      if (fgets (LineString, sizeof (LineString), DatabaseFile) == NULL)
      {
        ErrorCode = errno;
        if (feof (DatabaseFile))
          break;
        if (ErrorCode == B_OK)
          ErrorCode = -1;
        sprintf (ErrorMessage, "Error while reading words and statistics "
          "from database file \"%s\"", m_DatabaseFileName.String ());
        goto ErrorExit;
      }

      i = strlen (LineString);
      if (i > 0 && LineString[i-1] == '\n')
        LineString[i-1] = 0; /* Remove trailing line feed character. */

      /* Get the word at the start of the line, save in WordPntr. */

      TabPntr = LineString;
      for (WordPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
        ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

      /* Get the date stamp.  Actually a sequence number, not a date. */

      for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
        ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

      Statistics.age = atoll (StringPntr);

      /* Get the Genuine count. */

      for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
        ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

      Statistics.genuineCount = atoll (StringPntr);

      /* Get the Spam count. */

      for (StringPntr = TabPntr; *TabPntr != 0 && *TabPntr != '\t'; TabPntr++)
        ; if (*TabPntr == '\t') *TabPntr++ = 0; /* Stringify up to next tab. */

      Statistics.spamCount = atoll (StringPntr);

      /* Ignore empty words, totally unused words and ones which are too long
      (avoids lots of length checking everywhere). */

      if (WordPntr[0] == 0 || strlen (WordPntr) > g_MaxWordLength ||
      (Statistics.genuineCount <= 0 && Statistics.spamCount <= 0))
        continue; /* Ignore this line of text, start on next one. */

      /* Add the combination to the database. */

      InsertResult = m_WordMap.insert (
        StatisticsMap::value_type (WordPntr, Statistics));
      if (InsertResult.second == false)
      {
        ErrorCode = B_BAD_VALUE;
        sprintf (ErrorMessage, "Error while inserting word \"%s\" from "
          "database \"%s\", perhaps it is a duplicate",
          WordPntr, m_DatabaseFileName.String ());
        goto ErrorExit;
      }
      m_WordCount++;

      /* And the hunt for the oldest word. */

      if (Statistics.age < m_OldestAge)
        m_OldestAge = Statistics.age;
    }
  }
  else /* Saving, dump all words and statistics to the file. */
  {
    EndIter = m_WordMap.end ();
    for (DataIter = m_WordMap.begin (); DataIter != EndIter; DataIter++)
    {
      if (fprintf (DatabaseFile, "%s\t%lu\t%lu\t%lu\n",
      DataIter->first.c_str (), DataIter->second.age,
      DataIter->second.genuineCount, DataIter->second.spamCount) <= 0)
      {
        ErrorCode = errno;
        sprintf (ErrorMessage, "Error while writing word \"%s\" to "
          "database \"%s\"",
          DataIter->first.c_str(), m_DatabaseFileName.String ());
        goto ErrorExit;
      }
    }
  }

  /* Set the file type so that the new file gets associated with this program,
  and picks up the right icon. */

  if (!DoLoad)
  {
    sprintf (ErrorMessage, "Unable to set attributes (file type) of database "
      "file \"%s\"", m_DatabaseFileName.String ());
    ErrorCode = DatabaseNode.SetTo (m_DatabaseFileName.String ());
    if (ErrorCode != B_OK)
      goto ErrorExit;
    DatabaseNodeInfo.SetTo (&DatabaseNode);
    ErrorCode = DatabaseNodeInfo.SetType (g_ABSDatabaseFileMIMEType);
    if (ErrorCode != B_OK)
      goto ErrorExit;
  }

  /* Success! */
  m_DatabaseHasChanged = false;
  ErrorCode = B_OK;

ErrorExit:
  if (DatabaseFile != NULL)
    fclose (DatabaseFile);
  return ErrorCode;
}


/* Either load the settings (DoLoad is TRUE) from the configuration file or
write them (DoLoad is FALSE) to it.  The configuration file is a flattened
BMessage containing the various program settings.  If it doesn't exist (and its
parent directories don't exist) then it will be created when saving.  If it
doesn't exist when loading, the settings will be set to default values. */

status_t ABSApp::LoadSaveSettings (bool DoLoad)
{
  status_t    ErrorCode;
  const char *NamePntr;
  BMessage    Settings;
  BDirectory  SettingsDirectory;
  BFile       SettingsFile;
  const char *StringPntr;
  bool        TempBool;
  int32       TempInt32;
  char        TempString [PATH_MAX + 100];

  /* Preset things to default values if loading, in case of an error or it's an
  older version of the settings file which doesn't have every field defined. */

  if (DoLoad)
    DefaultSettings ();

  /* Look for our settings directory.  When saving we can try to create it. */

  ErrorCode = SettingsDirectory.SetTo (m_SettingsDirectoryPath.Path ());
  if (ErrorCode != B_OK)
  {
    if (DoLoad || ErrorCode != B_ENTRY_NOT_FOUND)
    {
      sprintf (TempString, "Can't find settings directory \"%s\"",
        m_SettingsDirectoryPath.Path ());
      goto ErrorExit;
    }
    ErrorCode = create_directory (m_SettingsDirectoryPath.Path (), 0755);
    if (ErrorCode == B_OK)
      ErrorCode = SettingsDirectory.SetTo (m_SettingsDirectoryPath.Path ());
    if (ErrorCode != B_OK)
    {
      sprintf (TempString, "Can't create settings directory \"%s\"",
        m_SettingsDirectoryPath.Path ());
      goto ErrorExit;
    }
  }

  ErrorCode = SettingsFile.SetTo (&SettingsDirectory, g_SettingsFileName,
    DoLoad ? B_READ_ONLY : B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
  if (ErrorCode != B_OK)
  {
    sprintf (TempString, "Can't open settings file \"%s\" in directory \"%s\" "
      "for %s", g_SettingsFileName, m_SettingsDirectoryPath.Path(),
      DoLoad ? "reading" : "writing");
    goto ErrorExit;
  }

  if (DoLoad)
  {
    ErrorCode = Settings.Unflatten (&SettingsFile);
    if (ErrorCode != 0 || Settings.what != g_SettingsWhatCode)
    {
      sprintf (TempString, "Corrupt data detected while reading settings "
        "file \"%s\" in directory \"%s\", will revert to defaults",
        g_SettingsFileName, m_SettingsDirectoryPath.Path());
      goto ErrorExit;
    }
  }

  /* Transfer the settings between the BMessage and our various global
  variables.  For loading, if the setting isn't present, leave it at the
  default value.  Note that loading and saving are intermingled here to make
  code maintenance easier (less chance of forgetting to update it if load and
  save were separate functions). */

  ErrorCode = B_OK; /* So that saving settings can record an error. */

  NamePntr = "DatabaseFileName";
  if (DoLoad)
  {
    if (Settings.FindString (NamePntr, &StringPntr) == B_OK)
      m_DatabaseFileName.SetTo (StringPntr);
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddString (NamePntr, m_DatabaseFileName);

  NamePntr = "ServerMode";
  if (DoLoad)
  {
    if (Settings.FindBool (NamePntr, &TempBool) == B_OK)
      g_ServerMode = TempBool;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddBool (NamePntr, g_ServerMode);

  NamePntr = "IgnorePreviousClassification";
  if (DoLoad)
  {
    if (Settings.FindBool (NamePntr, &TempBool) == B_OK)
      m_IgnorePreviousClassification = TempBool;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddBool (NamePntr, m_IgnorePreviousClassification);

  NamePntr = "PurgeAge";
  if (DoLoad)
  {
    if (Settings.FindInt32 (NamePntr, &TempInt32) == B_OK)
      m_PurgeAge = TempInt32;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddInt32 (NamePntr, m_PurgeAge);

  NamePntr = "PurgePopularity";
  if (DoLoad)
  {
    if (Settings.FindInt32 (NamePntr, &TempInt32) == B_OK)
      m_PurgePopularity = TempInt32;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddInt32 (NamePntr, m_PurgePopularity);

  NamePntr = "ScoringMode";
  if (DoLoad)
  {
    if (Settings.FindInt32 (NamePntr, &TempInt32) == B_OK)
      m_ScoringMode = (ScoringModes) TempInt32;
    if (m_ScoringMode < 0 || m_ScoringMode >= SM_MAX)
      m_ScoringMode = (ScoringModes) 0;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddInt32 (NamePntr, m_ScoringMode);

  NamePntr = "TokenizeMode";
  if (DoLoad)
  {
    if (Settings.FindInt32 (NamePntr, &TempInt32) == B_OK)
      m_TokenizeMode = (TokenizeModes) TempInt32;
    if (m_TokenizeMode < 0 || m_TokenizeMode >= TM_MAX)
      m_TokenizeMode = (TokenizeModes) 0;
  }
  else if (ErrorCode == B_OK)
    ErrorCode = Settings.AddInt32 (NamePntr, m_TokenizeMode);

  if (ErrorCode != B_OK)
  {
    strcpy (TempString, "Unable to stuff the program settings into a "
      "temporary BMessage, settings not saved");
    goto ErrorExit;
  }

  /* Save the settings BMessage to the settings file. */

  if (!DoLoad)
  {
    Settings.what = g_SettingsWhatCode;
    ErrorCode = Settings.Flatten (&SettingsFile);
    if (ErrorCode != 0)
    {
      sprintf (TempString, "Problems while writing settings file \"%s\" in "
        "directory \"%s\"", g_SettingsFileName,
        m_SettingsDirectoryPath.Path ());
      goto ErrorExit;
    }
  }

  m_SettingsHaveChanged = false;
  return B_OK;

ErrorExit: /* Error message in TempString, code in ErrorCode. */
  DisplayErrorMessage (TempString, ErrorCode, DoLoad ?
    "Loading Settings Error" : "Saving Settings Error");
  return ErrorCode;
}


void ABSApp::MessageReceived (BMessage *MessagePntr)
{
  const char           *PropertyName;
  struct property_info *PropInfoPntr;
  int32                 SpecifierIndex;
  int32                 SpecifierKind;
  BMessage              SpecifierMessage;

  /* See if it is a scripting message that applies to the database or one of
  the other operations this program supports.  Pass on other scripting messages
  to the inherited parent MessageReceived function (they're usually scripting
  messages for the BApplication). */

  switch (MessagePntr->what)
  {
    case B_GET_PROPERTY:
    case B_SET_PROPERTY:
    case B_COUNT_PROPERTIES:
    case B_CREATE_PROPERTY:
    case B_DELETE_PROPERTY:
    case B_EXECUTE_PROPERTY:
      if (MessagePntr->GetCurrentSpecifier (&SpecifierIndex, &SpecifierMessage,
      &SpecifierKind, &PropertyName) == B_OK &&
      SpecifierKind == B_DIRECT_SPECIFIER)
      {
        for (PropInfoPntr = g_ScriptingPropertyList + 0; true; PropInfoPntr++)
        {
          if (PropInfoPntr->name == 0)
            break; /* Ran out of commands. */

          if (PropInfoPntr->commands[0] == MessagePntr->what &&
          strcasecmp (PropInfoPntr->name, PropertyName) == 0)
          {
            ProcessScriptingMessage (MessagePntr, PropInfoPntr);
            return;
          }
        }
      }
      break;
  }

  /* Pass the unprocessed message to the inherited function, maybe it knows
  what to do.  This includes replies to messages we sent ourselves. */

  BApplication::MessageReceived (MessagePntr);
}


/* Rename the existing database file to a backup file name, potentially
replacing an older backup.  If something goes wrong, returns an error code and
puts an explanation in ErrorMessage. */

status_t ABSApp::MakeBackup (char *ErrorMessage)
{
  BEntry   Entry;
  status_t ErrorCode;
  int      i;
  char     LeafName [NAME_MAX];
  char     NewName [PATH_MAX+20];
  char     OldName [PATH_MAX+20];

  ErrorCode = Entry.SetTo (m_DatabaseFileName.String ());
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "While making backup, failed to make a BEntry for "
      "\"%s\" (maybe the directory doesn't exist?)",
      m_DatabaseFileName.String ());
    return ErrorCode;
  }
  if (!Entry.Exists ())
    return B_OK; /* No existing file to worry about overwriting. */
  Entry.GetName (LeafName);

  /* Find the first hole (no file) where we will stop the renaming chain. */

  for (i = 0; i < g_MaxBackups - 1; i++)
  {
    strcpy (OldName, m_DatabaseFileName.String ());
    sprintf (OldName + strlen (OldName), g_BackupSuffix, i);
    Entry.SetTo (OldName);
    if (!Entry.Exists ())
      break;
  }

  /* Move the files down by one to fill in the hole in the name series. */

  for (i--; i >= 0; i--)
  {
    strcpy (OldName, m_DatabaseFileName.String ());
    sprintf (OldName + strlen (OldName), g_BackupSuffix, i);
    Entry.SetTo (OldName);
    strcpy (NewName, LeafName);
    sprintf (NewName + strlen (NewName), g_BackupSuffix, i + 1);
    ErrorCode = Entry.Rename (NewName, true /* clobber */);
  }

  Entry.SetTo (m_DatabaseFileName.String ());
  strcpy (NewName, LeafName);
  sprintf (NewName + strlen (NewName), g_BackupSuffix, 0);
  ErrorCode = Entry.Rename (NewName, true /* clobber */);
  if (ErrorCode != B_OK)
    sprintf (ErrorMessage, "While making backup, failed to rename "
      "\"%s\" to \"%s\"", m_DatabaseFileName.String (), NewName);

  return ErrorCode;
}


void ABSApp::MakeDatabaseEmpty ()
{
  m_WordMap.clear (); /* Sets the map to empty, deallocating any old data. */
  m_WordCount = 0;
  m_TotalGenuineMessages = 0;
  m_TotalSpamMessages = 0;
  m_OldestAge = (uint32) -1 /* makes largest number possible */;
}


/* Do what the scripting command says.  A reply message will be sent back with
several fields: "error" containing the numerical error code (0 for success),
"CommandText" with a text representation of the command, "result" with the
resulting data for a get or count command.  If it isn't understood, then rather
than a B_REPLY kind of message, it will be a B_MESSAGE_NOT_UNDERSTOOD message
with an "error" number and an "message" string with a description. */

void ABSApp::ProcessScriptingMessage (
  BMessage *MessagePntr,
  struct property_info *PropInfoPntr)
{
  bool        ArgumentBool = false;
  bool        ArgumentGotBool = false;
  bool        ArgumentGotInt32 = false;
  bool        ArgumentGotString = false;
  int32       ArgumentInt32 = 0;
  const char *ArgumentString = NULL;
  BString     CommandText;
  status_t    ErrorCode;
  int         i;
  BMessage    ReplyMessage (B_MESSAGE_NOT_UNDERSTOOD);
  ssize_t     StringBufferSize;
  BMessage    TempBMessage;
  BPath       TempPath;
  char        TempString [PATH_MAX + 1024];

  if (g_QuitCountdown >= 0 && !g_CommandLineMode)
  {
    g_QuitCountdown = -1;
    cerr << "Quit countdown aborted due to a scripting command arriving.\n";
  }

  if (g_BusyCursor != NULL)
    SetCursor (g_BusyCursor);

  ErrorCode = MessagePntr->FindData (g_DataName, B_STRING_TYPE,
    (const void **) &ArgumentString, &StringBufferSize);
  if (ErrorCode == B_OK)
  {
    if (PropInfoPntr->extra_data != PN_EVALUATE_STRING &&
    PropInfoPntr->extra_data != PN_SPAM_STRING &&
    PropInfoPntr->extra_data != PN_GENUINE_STRING &&
    strlen (ArgumentString) >= PATH_MAX)
    {
      sprintf (TempString, "\"data\" string of a scripting message is too "
        "long, for SET %s action", PropInfoPntr->name);
      ErrorCode = B_NAME_TOO_LONG;
      goto ErrorExit;
    }
    ArgumentGotString = true;
  }
  else if (MessagePntr->FindBool (g_DataName, &ArgumentBool) == B_OK)
    ArgumentGotBool = true;
  else if (MessagePntr->FindInt32 (g_DataName, &ArgumentInt32) == B_OK)
    ArgumentGotInt32 = true;

  /* Prepare a Human readable description of the scripting command. */

  switch (PropInfoPntr->commands[0])
  {
    case B_SET_PROPERTY:
      CommandText.SetTo ("Set ");
      break;

    case B_GET_PROPERTY:
      CommandText.SetTo ("Get ");
      break;

    case B_COUNT_PROPERTIES:
      CommandText.SetTo ("Count ");
      break;

    case B_CREATE_PROPERTY:
      CommandText.SetTo ("Create ");
      break;

    case B_DELETE_PROPERTY:
      CommandText.SetTo ("Delete ");
      break;

    case B_EXECUTE_PROPERTY:
      CommandText.SetTo ("Execute ");
      break;

    default:
      sprintf (TempString, "Bug: scripting command for \"%s\" has an unknown "
        "action code %d", PropInfoPntr->name,
        (int) PropInfoPntr->commands[0]);
      ErrorCode = -1;
      goto ErrorExit;
  }
  CommandText.Append (PropInfoPntr->name);

  /* Add on the argument value to our readable command, if there is one. */

  if (ArgumentGotString)
  {
    CommandText.Append (" \"");
    CommandText.Append (ArgumentString);
    CommandText.Append ("\"");
  }
  if (ArgumentGotBool)
    CommandText.Append (ArgumentBool ? " true" : " false");
  if (ArgumentGotInt32)
  {
    sprintf (TempString, " %ld", ArgumentInt32);
    CommandText.Append (TempString);
  }

  /* From now on the scripting command has been recognized and is in the
  correct format, so it always returns a B_REPLY message.  A readable version
  of the command is also added to make debugging easier. */

  ReplyMessage.what = B_REPLY;
  ReplyMessage.AddString ("CommandText", CommandText);

  /* Now actually do the command.  First prepare a default error message. */

  sprintf (TempString, "Operation code %d (get, set, count, etc) "
    "unsupported for property %s",
    (int) PropInfoPntr->commands[0], PropInfoPntr->name);
  ErrorCode = B_BAD_INDEX;

  switch (PropInfoPntr->extra_data)
  {
    case PN_DATABASE_FILE:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY: /* Get the database file name. */
          ReplyMessage.AddString (g_ResultName, m_DatabaseFileName);
          break;

        case B_SET_PROPERTY: /* Set the database file name to a new one. */
          if (!ArgumentGotString)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a string for the "
              "SET %s command", PropInfoPntr->name);
            goto ErrorExit;
          }
          ErrorCode = TempPath.SetTo (ArgumentString, NULL /* leaf */,
            true /* normalize - verifies parent directories exist */);
          if (ErrorCode != B_OK)
          {
            sprintf (TempString, "New database path name of \"%s\" is invalid "
              "(parent directories must exist)", ArgumentString);
            goto ErrorExit;
          }
          if ((ErrorCode = SaveDatabaseIfNeeded (TempString)) != B_OK)
            goto ErrorExit;
          MakeDatabaseEmpty (); /* So that the new one gets loaded if used. */

          if (strlen (TempPath.Leaf ()) > NAME_MAX-strlen(g_BackupSuffix)-1)
          {
            /* Truncate the name so that there is enough space for the backup
            extension.  Approximately. */
            strcpy (TempString, TempPath.Leaf ());
            TempString [NAME_MAX - strlen (g_BackupSuffix) - 1] = 0;
            TempPath.GetParent (&TempPath);
            TempPath.Append (TempString);
          }
          m_DatabaseFileName.SetTo (TempPath.Path ());
          m_SettingsHaveChanged = true;
          break;

        case B_CREATE_PROPERTY: /* Make a new database file plus more. */
          if ((ErrorCode = CreateDatabaseFile (TempString)) != B_OK)
            goto ErrorExit;
          break;

        case B_DELETE_PROPERTY: /* Delete the file and its backups too. */
          if ((ErrorCode = DeleteDatabaseFile (TempString)) != B_OK)
            goto ErrorExit;
          break;

        case B_COUNT_PROPERTIES:
          if ((ErrorCode = LoadDatabaseIfNeeded (TempString)) != B_OK)
            goto ErrorExit;
          ReplyMessage.AddInt32 (g_ResultName, m_WordCount);
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_SPAM:
    case PN_SPAM_STRING:
    case PN_GENUINE:
    case PN_GENUINE_STRING:
    case PN_UNCERTAIN:
      switch (PropInfoPntr->commands[0])
      {
        case B_COUNT_PROPERTIES: /* Get the number of spam/genuine messages. */
          if ((ErrorCode = LoadDatabaseIfNeeded (TempString)) != B_OK)
            goto ErrorExit;
          if (PropInfoPntr->extra_data == PN_SPAM ||
          PropInfoPntr->extra_data == PN_SPAM_STRING)
            ReplyMessage.AddInt32 (g_ResultName, m_TotalSpamMessages);
          else
            ReplyMessage.AddInt32 (g_ResultName, m_TotalGenuineMessages);
          break;

        case B_SET_PROPERTY: /* Add spam/genuine/uncertain to database. */
          if (!ArgumentGotString)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a string (%s) "
              "for the SET %s command",
              (PropInfoPntr->extra_data == PN_GENUINE_STRING ||
              PropInfoPntr->extra_data == PN_SPAM_STRING)
              ? "text of the message to be added"
              : "pathname of the file containing the text to be added",
              PropInfoPntr->name);
            goto ErrorExit;
          }
          if ((ErrorCode = LoadDatabaseIfNeeded (TempString)) != B_OK)
            goto ErrorExit;
          if (PropInfoPntr->extra_data == PN_GENUINE ||
          PropInfoPntr->extra_data == PN_SPAM ||
          PropInfoPntr->extra_data == PN_UNCERTAIN)
            ErrorCode = AddFileToDatabase (
              (PropInfoPntr->extra_data == PN_SPAM) ? CL_SPAM :
              ((PropInfoPntr->extra_data == PN_GENUINE) ? CL_GENUINE :
              CL_UNCERTAIN),
              ArgumentString, TempString /* ErrorMessage */);
          else
            ErrorCode = AddStringToDatabase (
              (PropInfoPntr->extra_data == PN_SPAM_STRING) ?
              CL_SPAM : CL_GENUINE,
              ArgumentString, TempString /* ErrorMessage */);
          if (ErrorCode != B_OK)
            goto ErrorExit;
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_IGNORE_PREVIOUS_CLASSIFICATION:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddBool (g_ResultName, m_IgnorePreviousClassification);
          break;

        case B_SET_PROPERTY:
          if (!ArgumentGotBool)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a boolean (true/yes, "
              "false/no) for the SET %s command", PropInfoPntr->name);
            goto ErrorExit;
          }
          m_IgnorePreviousClassification = ArgumentBool;
          m_SettingsHaveChanged = true;
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_SERVER_MODE:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddBool (g_ResultName, g_ServerMode);
          break;

        case B_SET_PROPERTY:
          if (!ArgumentGotBool)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a boolean (true/yes, "
              "false/no) for the SET %s command", PropInfoPntr->name);
            goto ErrorExit;
          }
          g_ServerMode = ArgumentBool;
          m_SettingsHaveChanged = true;
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_FLUSH:
      if (PropInfoPntr->commands[0] == B_EXECUTE_PROPERTY &&
      (ErrorCode = SaveDatabaseIfNeeded (TempString)) == B_OK)
        break;
      goto ErrorExit;

    case PN_PURGE_AGE:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddInt32 (g_ResultName, m_PurgeAge);
          break;

        case B_SET_PROPERTY:
          if (!ArgumentGotInt32)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a 32 bit integer "
              "for the SET %s command", PropInfoPntr->name);
            goto ErrorExit;
          }
          m_PurgeAge = ArgumentInt32;
          m_SettingsHaveChanged = true;
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_PURGE_POPULARITY:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddInt32 (g_ResultName, m_PurgePopularity);
          break;

        case B_SET_PROPERTY:
          if (!ArgumentGotInt32)
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You need to specify a 32 bit integer "
              "for the SET %s command", PropInfoPntr->name);
            goto ErrorExit;
          }
          m_PurgePopularity = ArgumentInt32;
          m_SettingsHaveChanged = true;
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_PURGE:
      if (PropInfoPntr->commands[0] == B_EXECUTE_PROPERTY &&
      (ErrorCode = LoadDatabaseIfNeeded (TempString)) == B_OK &&
      (ErrorCode = PurgeOldWords (TempString)) == B_OK)
        break;
      goto ErrorExit;

    case PN_OLDEST:
      if (PropInfoPntr->commands[0] == B_GET_PROPERTY &&
      (ErrorCode = LoadDatabaseIfNeeded (TempString)) == B_OK)
      {
        ReplyMessage.AddInt32 (g_ResultName, m_OldestAge);
        break;
      }
      goto ErrorExit;

    case PN_EVALUATE:
    case PN_EVALUATE_STRING:
      if (PropInfoPntr->commands[0] == B_SET_PROPERTY)
      {
        if (!ArgumentGotString)
        {
          ErrorCode = B_BAD_TYPE;
          sprintf (TempString, "You need to specify a string for the "
            "SET %s command", PropInfoPntr->name);
          goto ErrorExit;
        }
        if ((ErrorCode = LoadDatabaseIfNeeded (TempString)) == B_OK)
        {
          if (PropInfoPntr->extra_data == PN_EVALUATE)
          {
            if ((ErrorCode = EvaluateFile (ArgumentString, &ReplyMessage,
            TempString)) == B_OK)
              break;
          }
          else /* PN_EVALUATE_STRING */
          {
            if ((ErrorCode = EvaluateString (ArgumentString, StringBufferSize,
            &ReplyMessage, TempString)) == B_OK)
              break;
          }
        }
      }
      goto ErrorExit;

    case PN_RESET_TO_DEFAULTS:
      if (PropInfoPntr->commands[0] == B_EXECUTE_PROPERTY)
      {
        DefaultSettings ();
        break;
      }
      goto ErrorExit;

    case PN_INSTALL_THINGS:
      if (PropInfoPntr->commands[0] == B_EXECUTE_PROPERTY &&
      (ErrorCode = InstallThings (TempString)) == B_OK)
        break;
      goto ErrorExit;

    case PN_SCORING_MODE:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddString (g_ResultName,
            g_ScoringModeNames[m_ScoringMode]);
          break;

        case B_SET_PROPERTY:
          i = SM_MAX;
          if (ArgumentGotString)
            for (i = 0; i < SM_MAX; i++)
            {
              if (strcasecmp (ArgumentString, g_ScoringModeNames [i]) == 0)
              {
                m_ScoringMode = (ScoringModes) i;
                m_SettingsHaveChanged = true;
                break;
              }
            }
          if (i >= SM_MAX) /* Didn't find a valid scoring mode word. */
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You used the unrecognized \"%s\" as "
              "a scoring mode for the SET %s command.  Should be one of: ",
              ArgumentGotString ? ArgumentString : "not specified",
              PropInfoPntr->name);
            for (i = 0; i < SM_MAX; i++)
            {
              strcat (TempString, g_ScoringModeNames [i]);
              if (i < SM_MAX - 1)
                strcat (TempString, ", ");
            }
            goto ErrorExit;
          }
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    case PN_TOKENIZE_MODE:
      switch (PropInfoPntr->commands[0])
      {
        case B_GET_PROPERTY:
          ReplyMessage.AddString (g_ResultName,
            g_TokenizeModeNames[m_TokenizeMode]);
          break;

        case B_SET_PROPERTY:
          i = TM_MAX;
          if (ArgumentGotString)
            for (i = 0; i < TM_MAX; i++)
            {
              if (strcasecmp (ArgumentString, g_TokenizeModeNames [i]) == 0)
              {
                m_TokenizeMode = (TokenizeModes) i;
                m_SettingsHaveChanged = true;
                break;
              }
            }
          if (i >= TM_MAX) /* Didn't find a valid tokenize mode word. */
          {
            ErrorCode = B_BAD_TYPE;
            sprintf (TempString, "You used the unrecognized \"%s\" as "
              "a tokenize mode for the SET %s command.  Should be one of: ",
              ArgumentGotString ? ArgumentString : "not specified",
              PropInfoPntr->name);
            for (i = 0; i < TM_MAX; i++)
            {
              strcat (TempString, g_TokenizeModeNames [i]);
              if (i < TM_MAX - 1)
                strcat (TempString, ", ");
            }
            goto ErrorExit;
          }
          break;

        default: /* Unknown operation code, error message already set. */
          goto ErrorExit;
      }
      break;

    default:
      sprintf (TempString, "Bug!  Unrecognized property identification "
        "number %d (should be between 0 and %d).  Fix the entry in "
        "the g_ScriptingPropertyList array!",
        (int) PropInfoPntr->extra_data, PN_MAX - 1);
      goto ErrorExit;
  }

  /* Success. */

  ReplyMessage.AddInt32 ("error", B_OK);
  ErrorCode = MessagePntr->SendReply (&ReplyMessage,
    this /* Reply's reply handler */, 500000 /* send timeout */);
  if (ErrorCode != B_OK)
    cerr << "ProcessScriptingMessage failed to send a reply message, code " <<
    ErrorCode << " (" << strerror (ErrorCode) << ")" << " for " <<
    CommandText.String () << endl;
  SetCursor (B_CURSOR_SYSTEM_DEFAULT);
  return;

ErrorExit: /* Error message in TempString, return code in ErrorCode. */
  ReplyMessage.AddInt32 ("error", ErrorCode);
  ReplyMessage.AddString ("message", TempString);
  DisplayErrorMessage (TempString, ErrorCode);
  ErrorCode = MessagePntr->SendReply (&ReplyMessage,
    this /* Reply's reply handler */, 500000 /* send timeout */);
  if (ErrorCode != B_OK)
    cerr << "ProcessScriptingMessage failed to send an error message, code " <<
    ErrorCode << " (" << strerror (ErrorCode) << ")" << " for " <<
    CommandText.String () << endl;
  SetCursor (B_CURSOR_SYSTEM_DEFAULT);
}


/* Since quitting stops the program before the results of a script command are
received, we use a time delay to do the quit and make sure there are no pending
commands being processed by the auxiliary looper which is sending us commands.
Also, we have a countdown which can be interrupted by an incoming scripting
message in case one client tells us to quit while another one is still using us
(happens when you have two or more e-mail accounts).  But if the system is
shutting down, quit immediately! */

void ABSApp::Pulse ()
{
  if (g_QuitCountdown == 0)
  {
    if (g_CommanderLooperPntr == NULL ||
    !g_CommanderLooperPntr->IsBusy ())
      PostMessage (B_QUIT_REQUESTED);
  }
  else if (g_QuitCountdown > 0)
  {
    cerr << "SpamDBM quitting in " << g_QuitCountdown << ".\n";
    g_QuitCountdown--;
  }
}


/* A quit request message has come in.  If the quit countdown has reached zero,
allow the request, otherwise reject it (and start the countdown if it hasn't
been started). */

bool ABSApp::QuitRequested ()
{
  BMessage  *QuitMessage;
  team_info  RemoteInfo;
  BMessenger RemoteMessenger;
  team_id    RemoteTeam;

  /* See if the quit is from the system shutdown command (which goes through
  the registrar server), if so, quit immediately. */

  QuitMessage = CurrentMessage ();
  if (QuitMessage != NULL && QuitMessage->IsSourceRemote ())
  {
    RemoteMessenger = QuitMessage->ReturnAddress ();
    RemoteTeam = RemoteMessenger.Team ();
    if (get_team_info (RemoteTeam, &RemoteInfo) == B_OK &&
    strstr (RemoteInfo.args, "registrar") != NULL)
      g_QuitCountdown = 0;
  }

  if (g_QuitCountdown == 0)
    return BApplication::QuitRequested ();

  if (g_QuitCountdown < 0)
//    g_QuitCountdown = 10; /* Start the countdown. */
    g_QuitCountdown = 5; /* Quit more quickly */

  return false;
}


/* Go through the current database and delete words which are too old (time is
equivalent to the number of messages added to the database) and too unpopular
(words not used by many messages).  Hopefully this will get rid of words which
are just hunks of binary or other garbage.  The database has been loaded
elsewhere. */

status_t ABSApp::PurgeOldWords (char *ErrorMessage)
{
  uint32                  CurrentTime;
  StatisticsMap::iterator CurrentIter;
  StatisticsMap::iterator EndIter;
  StatisticsMap::iterator NextIter;
  char                    TempString [80];

  strcpy (ErrorMessage, "Purge can't fail"); /* So argument gets used. */
  CurrentTime = m_TotalGenuineMessages + m_TotalSpamMessages - 1;
  m_OldestAge = (uint32) -1 /* makes largest number possible */;

  EndIter = m_WordMap.end ();
  NextIter = m_WordMap.begin ();
  while (NextIter != EndIter)
  {
    CurrentIter = NextIter++;

    if (CurrentTime - CurrentIter->second.age >= m_PurgeAge &&
    CurrentIter->second.genuineCount + CurrentIter->second.spamCount <=
    m_PurgePopularity)
    {
      /* Delete this word, it is unpopular and old.  Sob. */

      m_WordMap.erase (CurrentIter);
      if (m_WordCount > 0)
        m_WordCount--;

      m_DatabaseHasChanged = true;
    }
    else /* This word is still in the database.  Update oldest age. */
    {
      if (CurrentIter->second.age < m_OldestAge)
        m_OldestAge = CurrentIter->second.age;
    }
  }

  /* Just a little bug check here.  Just in case. */

  if (m_WordCount != m_WordMap.size ())
  {
    sprintf (TempString, "Our word count of %lu doesn't match the "
      "size of the database, %lu", m_WordCount, m_WordMap.size ());
    DisplayErrorMessage (TempString, -1, "Bug!");
    m_WordCount = m_WordMap.size ();
  }

  return B_OK;
}


void ABSApp::ReadyToRun ()
{
  DatabaseWindow *DatabaseWindowPntr;
  float           JunkFloat;
  BButton        *TempButtonPntr;
  BCheckBox      *TempCheckBoxPntr;
  font_height     TempFontHeight;
  BMenuBar       *TempMenuBarPntr;
  BMenuItem      *TempMenuItemPntr;
  BPopUpMenu     *TempPopUpMenuPntr;
  BRadioButton   *TempRadioButtonPntr;
  BRect           TempRect;
  const char     *TempString = "Testing My Things";
  BStringView    *TempStringViewPntr;
  BTextControl   *TempTextPntr;
  BWindow        *TempWindowPntr;

  /* This batch of code gets some measurements which will be used for laying
  out controls and other GUI elements.  Set the spacing between buttons and
  other controls to the width of the letter "M" in the user's desired font. */

 g_MarginBetweenControls = (int) be_plain_font->StringWidth ("M");

  /* Also find out how much space a line of text uses. */

  be_plain_font->GetHeight (&TempFontHeight);
  g_LineOfTextHeight = ceilf (
    TempFontHeight.ascent + TempFontHeight.descent + TempFontHeight.leading);

  /* Start finding out the height of various user interface gadgets, which can
  vary based on the current font size.  Make a temporary gadget, which is
  attached to our window, then resize it to its prefered size so that it
  accomodates the font size and other frills it needs. */

  TempWindowPntr = new BWindow (BRect (10, 20, 200, 200), "Temporary Window",
    B_DOCUMENT_WINDOW, B_NO_WORKSPACE_ACTIVATION | B_ASYNCHRONOUS_CONTROLS);
  if (TempWindowPntr == NULL)
  {
    DisplayErrorMessage ("Unable to create temporary window for finding "
      "sizes of controls.");
    g_QuitCountdown = 0;
    return;
  }

  TempRect = TempWindowPntr->Bounds ();

  /* Find the height of a single line of text in a BStringView. */

  TempStringViewPntr = new BStringView (TempRect, TempString, TempString);
  if (TempStringViewPntr != NULL)
  {
    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempStringViewPntr);
    TempStringViewPntr->GetPreferredSize (&JunkFloat, &g_StringViewHeight);
    TempWindowPntr->RemoveChild (TempStringViewPntr);
    TempWindowPntr->Unlock ();
    delete TempStringViewPntr;
  }

  /* Find the height of a button, which seems to be larger than a text
  control and can make life difficult.  Make a temporary button, which
  is attached to our window so that it resizes to accomodate the font size. */

  TempButtonPntr = new BButton (TempRect, TempString, TempString, NULL);
  if (TempButtonPntr != NULL)
  {
    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempButtonPntr);
    TempButtonPntr->GetPreferredSize (&JunkFloat, &g_ButtonHeight);
    TempWindowPntr->RemoveChild (TempButtonPntr);
    TempWindowPntr->Unlock ();
    delete TempButtonPntr;
  }

  /* Find the height of a text box. */

  TempTextPntr = new BTextControl (TempRect, TempString, NULL /* label */,
    TempString, NULL);
  if (TempTextPntr != NULL)
  {
    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempTextPntr);
    TempTextPntr->GetPreferredSize (&JunkFloat, &g_TextBoxHeight);
    TempWindowPntr->RemoveChild (TempTextPntr);
    TempWindowPntr->Unlock ();
    delete TempTextPntr;
  }

  /* Find the height of a checkbox control. */

  TempCheckBoxPntr = new BCheckBox (TempRect, TempString, TempString, NULL);
  if (TempCheckBoxPntr != NULL)
  {
    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempCheckBoxPntr);
    TempCheckBoxPntr->GetPreferredSize (&JunkFloat, &g_CheckBoxHeight);
    TempWindowPntr->RemoveChild (TempCheckBoxPntr);
    TempWindowPntr->Unlock ();
    delete TempCheckBoxPntr;
  }

  /* Find the height of a radio button control. */

  TempRadioButtonPntr =
    new BRadioButton (TempRect, TempString, TempString, NULL);
  if (TempRadioButtonPntr != NULL)
  {
    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempRadioButtonPntr);
    TempRadioButtonPntr->GetPreferredSize (&JunkFloat, &g_RadioButtonHeight);
    TempWindowPntr->RemoveChild (TempRadioButtonPntr);
    TempWindowPntr->Unlock ();
    delete TempRadioButtonPntr;
  }

  /* Find the height of a pop-up menu. */

  TempMenuBarPntr = new BMenuBar (TempRect, TempString,
    B_FOLLOW_LEFT | B_FOLLOW_TOP, B_ITEMS_IN_COLUMN,
    true /* resize to fit items */);
  TempPopUpMenuPntr = new BPopUpMenu (TempString);
  TempMenuItemPntr = new BMenuItem (TempString, new BMessage (12345), 'g');

  if (TempMenuBarPntr != NULL && TempPopUpMenuPntr != NULL &&
  TempMenuItemPntr != NULL)
  {
    TempPopUpMenuPntr->AddItem (TempMenuItemPntr);
    TempMenuBarPntr->AddItem (TempPopUpMenuPntr);

    TempWindowPntr->Lock ();
    TempWindowPntr->AddChild (TempMenuBarPntr);
    TempMenuBarPntr->GetPreferredSize (&JunkFloat, &g_PopUpMenuHeight);
    TempWindowPntr->RemoveChild (TempMenuBarPntr);
    TempWindowPntr->Unlock ();
    delete TempMenuBarPntr; // It will delete contents too.
  }

  TempWindowPntr->Lock ();
  TempWindowPntr->Quit ();

  SetPulseRate (500000);

  if (g_CommandLineMode)
    g_QuitCountdown = 0; /* Quit as soon as queued up commands done. */
  else /* GUI mode, make a window. */
  {
    DatabaseWindowPntr = new DatabaseWindow ();
    if (DatabaseWindowPntr == NULL)
    {
      DisplayErrorMessage ("Unable to create window.");
      g_QuitCountdown = 0;
    }
    else {
      DatabaseWindowPntr->Show (); /* Starts the window's message loop. */
    }
  }

  g_AppReadyToRunCompleted = true;
}


/* Given a mail component (body text, attachment, whatever), look for words in
it.  If the tokenize mode specifies that it isn't one of the ones we are
looking for, just skip it.  For container type components, recursively examine
their contents, up to the maximum depth specified. */

status_t ABSApp::RecursivelyTokenizeMailComponent (
  BMailComponent *ComponentPntr,
  const char *OptionalFileName,
  set<string> &WordSet,
  char *ErrorMessage,
  int RecursionLevel,
  int MaxRecursionLevel)
{
  char                        AttachmentName [B_FILE_NAME_LENGTH];
  BMailAttachment            *AttachmentPntr;
  BMimeType                   ComponentMIMEType;
  BMailContainer             *ContainerPntr;
  BMallocIO                   ContentsIO;
  const char                 *ContentsBufferPntr;
  size_t                      ContentsBufferSize;
  status_t                    ErrorCode;
  bool                        ExamineComponent;
  const char                 *HeaderKeyPntr;
  const char                 *HeaderValuePntr;
  int                         i;
  int                         j;
  const char                 *NameExtension;
  int                         NumComponents;
  BMimeType                   TextAnyMIMEType ("text");
  BMimeType                   TextPlainMIMEType ("text/plain");

  if (ComponentPntr == NULL)
    return B_OK;

  /* Add things in the sub-headers that might be useful.  Things like the file
  name of attachments, the encoding type, etc. */

  if (m_TokenizeMode == TM_PLAIN_TEXT_HEADER ||
  m_TokenizeMode == TM_ANY_TEXT_HEADER ||
  m_TokenizeMode == TM_ALL_PARTS_HEADER ||
  m_TokenizeMode == TM_JUST_HEADER)
  {
    for (i = 0; i < 1000; i++)
    {
      HeaderKeyPntr = ComponentPntr->HeaderAt (i);
      if (HeaderKeyPntr == NULL)
        break;
      AddWordsToSet (HeaderKeyPntr, strlen (HeaderKeyPntr),
        'H' /* Prefix for Headers, uppercase unlike normal words. */, WordSet);
      for (j = 0; j < 1000; j++)
      {
        HeaderValuePntr = ComponentPntr->HeaderField (HeaderKeyPntr, j);
        if (HeaderValuePntr == NULL)
          break;
        AddWordsToSet (HeaderValuePntr, strlen (HeaderValuePntr),
          'H', WordSet);
      }
    }
  }

  /* Check the MIME type of the thing.  It's used to decide if the contents are
  worth examining for words. */

  ErrorCode = ComponentPntr->MIMEType (&ComponentMIMEType);
  if (ErrorCode != B_OK)
  {
    sprintf (ErrorMessage, "ABSApp::RecursivelyTokenizeMailComponent: "
      "Unable to get MIME type at level %d in \"%s\"",
      RecursionLevel, OptionalFileName);
    return ErrorCode;
  }
  if (ComponentMIMEType.Type() == NULL)
  {
    /* Have to make up a MIME type for things which don't have them, such as
    the main body text, otherwise it would get ignored. */

    if (NULL != dynamic_cast<BTextMailComponent *>(ComponentPntr))
      ComponentMIMEType.SetType ("text/plain");
  }
  if (!TextAnyMIMEType.Contains (&ComponentMIMEType) &&
  NULL != (AttachmentPntr = dynamic_cast<BMailAttachment *>(ComponentPntr)))
  {
    /* Sometimes spam doesn't give a text MIME type for text when they do an
    attachment (which is often base64 encoded).  Use the file name extension to
    see if it really is text. */
    NameExtension = NULL;
    if (AttachmentPntr->FileName (AttachmentName) >= 0)
      NameExtension = strrchr (AttachmentName, '.');
    if (NameExtension != NULL)
    {
      if (strcasecmp (NameExtension, ".txt") == 0)
        ComponentMIMEType.SetType ("text/plain");
      else if (strcasecmp (NameExtension, ".htm") == 0 ||
      strcasecmp (NameExtension, ".html") == 0)
        ComponentMIMEType.SetType ("text/html");
    }
  }

  switch (m_TokenizeMode)
  {
    case TM_PLAIN_TEXT:
    case TM_PLAIN_TEXT_HEADER:
      ExamineComponent = TextPlainMIMEType.Contains (&ComponentMIMEType);
      break;

    case TM_ANY_TEXT:
    case TM_ANY_TEXT_HEADER:
      ExamineComponent = TextAnyMIMEType.Contains (&ComponentMIMEType);
      break;

    case TM_ALL_PARTS:
    case TM_ALL_PARTS_HEADER:
      ExamineComponent = true;
      break;

    default:
      ExamineComponent = false;
      break;
  }

  if (ExamineComponent)
  {
    /* Get the contents of the component.  This will be UTF-8 text (converted
    from whatever encoding was used) for text attachments.  For other ones,
    it's just the raw data, or perhaps decoded from base64 encoding. */

    ContentsIO.SetBlockSize (16 * 1024);
    ErrorCode = ComponentPntr->GetDecodedData (&ContentsIO);
    if (ErrorCode == B_OK) /* Can fail for container components: no data. */
    {
      /* Look for words in the decoded data. */

      ContentsBufferPntr = (const char *) ContentsIO.Buffer ();
      ContentsBufferSize = ContentsIO.BufferLength ();
      if (ContentsBufferPntr != NULL /* can be empty */)
        AddWordsToSet (ContentsBufferPntr, ContentsBufferSize,
          0 /* no prefix character, this is body text */, WordSet);
    }
  }

  /* Examine any sub-components in the message. */

  if (RecursionLevel + 1 <= MaxRecursionLevel &&
  NULL != (ContainerPntr = dynamic_cast<BMailContainer *>(ComponentPntr)))
  {
    NumComponents = ContainerPntr->CountComponents ();

    for (i = 0; i < NumComponents; i++)
    {
      ComponentPntr = ContainerPntr->GetComponent (i);

      ErrorCode = RecursivelyTokenizeMailComponent (ComponentPntr,
        OptionalFileName, WordSet, ErrorMessage, RecursionLevel + 1,
        MaxRecursionLevel);
      if (ErrorCode != B_OK)
        break;
    }
  }

  return ErrorCode;
}


/* The user has tried to open a file or several files with this application,
via Tracker's open-with menu item.  If it is a database type file, then change
the database file name to it.  Otherwise, ask the user whether they want to
classify it as spam or non-spam.  There will be at most around 100 files, BeOS
R5.0.3's Tracker crashes if it tries to pass on more than that many using Open
With... etc.  The command is sent to an intermediary thread where it is
asynchronously converted into a scripting message(s) that are sent back to this
BApplication.  The intermediary is needed since we can't recursively execute
scripting messages while processing a message (this RefsReceived one). */

void ABSApp::RefsReceived (BMessage *MessagePntr)
{
  if (g_CommanderLooperPntr != NULL)
    g_CommanderLooperPntr->CommandReferences (MessagePntr);
}


/* A scripting command is looking for something to execute it.  See if it is
targetted at our database. */

BHandler * ABSApp::ResolveSpecifier (
  BMessage *MessagePntr,
  int32 Index,
  BMessage *SpecifierMsgPntr,
  int32 SpecificationKind,
  const char *PropertyPntr)
{
  int i;

  /* See if it is one of our commands. */

  if (SpecificationKind == B_DIRECT_SPECIFIER)
  {
    for (i = PN_MAX - 1; i >= 0; i--)
    {
      if (strcasecmp (PropertyPntr, g_PropertyNames [i]) == 0)
        return this; /* Found it!  Return the Handler (which is us). */
    }
  }

  /* Handle an unrecognized scripting command, let the parent figure it out. */

  return BApplication::ResolveSpecifier (
    MessagePntr, Index, SpecifierMsgPntr, SpecificationKind, PropertyPntr);
}


/* Save the database if it hasn't been saved yet.  Otherwise do nothing. */

status_t ABSApp::SaveDatabaseIfNeeded (char *ErrorMessage)
{
  if (m_DatabaseHasChanged)
    return LoadSaveDatabase (false /* DoLoad */, ErrorMessage);

  return B_OK;
}


/* Presumably the file is an e-mail message (or at least the header portion of
one).  Break it into parts: header, body and MIME components.  Then add the
words in the portions that match the current tokenization settings to the set
of words. */

status_t ABSApp::TokenizeParts (
  BPositionIO *PositionIOPntr,
  const char *OptionalFileName,
  set<string> &WordSet,
  char *ErrorMessage)
{
  status_t        ErrorCode = B_OK;
  BEmailMessage   WholeEMail;

  sprintf (ErrorMessage, "ABSApp::TokenizeParts: While getting e-mail "
    "headers, had problems with \"%s\"", OptionalFileName);

  ErrorCode = WholeEMail.SetToRFC822 (
    PositionIOPntr /* it does its own seeking to the start */,
    -1 /* length */, true /* parse_now */);
  if (ErrorCode < 0) goto ErrorExit;

  ErrorCode = RecursivelyTokenizeMailComponent (&WholeEMail,
    OptionalFileName, WordSet, ErrorMessage, 0 /* Initial recursion level */,
    (m_TokenizeMode == TM_JUST_HEADER) ? 0 : 500 /* Max recursion level */);

ErrorExit:
  return ErrorCode;
}


/* Add all the words in the whole file or memory buffer to the supplied set.
The file doesn't have to be an e-mail message since it isn't parsed for e-mail
headers or MIME headers or anything.  It blindly adds everything that looks
like a word, though it does convert quoted printable codes to the characters
they represent.  See also AddWordsToSet which does something more advanced. */

status_t ABSApp::TokenizeWhole (
  BPositionIO *PositionIOPntr,
  const char *OptionalFileName,
  set<string> &WordSet,
  char *ErrorMessage)
{
  string                AccumulatedWord;
  uint8                 Buffer [16 * 1024];
  uint8                *BufferCurrentPntr = Buffer + 0;
  uint8                *BufferEndPntr = Buffer + 0;
  const char           *IOErrorString =
                          "TokenizeWhole: Error %ld while reading \"%s\"";
  size_t                Length;
  int                   Letter = ' ';
  char                  HexString [4];
  int                   NextLetter = ' ';
  int                   NextNextLetter = ' ';

  /* Use a buffer since reading single characters from a BFile is so slow.
  BufferCurrentPntr is the position of the next character to be read.  When it
  reaches BufferEndPntr, it is time to fill the buffer again. */

#define ReadChar(CharVar) \
  { \
    if (BufferCurrentPntr < BufferEndPntr) \
      CharVar = *BufferCurrentPntr++; \
    else /* Try to fill the buffer. */ \
    { \
      ssize_t AmountRead; \
      AmountRead = PositionIOPntr->Read (Buffer, sizeof (Buffer)); \
      if (AmountRead < 0) \
      { \
        sprintf (ErrorMessage, IOErrorString, AmountRead, OptionalFileName); \
        return AmountRead; \
      } \
      else if (AmountRead == 0) \
        CharVar = EOF; \
      else \
      { \
        BufferEndPntr = Buffer + AmountRead; \
        BufferCurrentPntr = Buffer + 0; \
        CharVar = *BufferCurrentPntr++; \
      } \
    } \
  }

  /* Read all the words in the file and add them to our local set of words.  A
  set is used since we don't care how many times a word occurs. */

  while (true)
  {
    /* We read two letters ahead so that we can decode quoted printable
    characters (an equals sign followed by two hex digits or a new line).  Note
    that Letter can become EOF (-1) when end of file is reached. */

    Letter = NextLetter;
    NextLetter = NextNextLetter;
    ReadChar (NextNextLetter);

    /* Decode quoted printable codes first, so that the rest of the code just
    sees an ordinary character.  Or even nothing, if it is the hidden line
    break combination.  This may falsely corrupt stuff following an equals
    sign, but usually won't. */

    if (Letter == '=')
    {
      if ((NextLetter == '\r' && NextNextLetter == '\n') ||
      (NextLetter == '\n' && NextNextLetter == '\r'))
      {
        /* Make the "=\r\n" pair disappear.  It's not even white space. */
        ReadChar (NextLetter);
        ReadChar (NextNextLetter);
        continue;
      }
      if (NextLetter == '\n' || NextLetter == '\r')
      {
        /* Make the "=\n" pair disappear.  It's not even white space. */
        NextLetter = NextNextLetter;
        ReadChar (NextNextLetter);
        continue;
      }
      if (NextNextLetter != EOF &&
      isxdigit (NextLetter) && isxdigit (NextNextLetter))
      {
        /* Convert the hex code to a letter. */
        HexString[0] = NextLetter;
        HexString[1] = NextNextLetter;
        HexString[2] = 0;
        Letter = strtoul (HexString, NULL, 16 /* number system base */);
        ReadChar (NextLetter);
        ReadChar (NextNextLetter);
      }
    }

    /* Convert to lower case to improve word matches.  Of course this loses a
    bit of information, such as MONEY vs Money, an indicator of spam.  Well,
    apparently that isn't all that useful a distinction, so do it. */

    if (Letter >= 'A' && Letter < 'Z')
      Letter = Letter + ('a' - 'A');

    /* See if it is a letter we treat as white space - all control characters
    and all punctuation except for: apostrophe (so "it's" and possessive
    versions of words get stored), dash (for hyphenated words), dollar sign
    (for cash amounts), period (for IP addresses, we later remove trailing
    (periods).  Note that codes above 127 are UTF-8 characters, which we
    consider non-space. */

    if (Letter < 0 /* EOF */ || (Letter < 128 && g_SpaceCharacters[Letter]))
    {
      /* That space finished off a word.  Remove trailing periods... */

      while ((Length = AccumulatedWord.size()) > 0 &&
      AccumulatedWord [Length-1] == '.')
        AccumulatedWord.resize (Length - 1);

      /* If there's anything left in the word, add it to the set.  Also ignore
      words which are too big (it's probably some binary encoded data).  But
      leave room for supercalifragilisticexpialidoceous.  According to one web
      site, pneumonoultramicroscopicsilicovolcanoconiosis is the longest word
      currently in English.  Note that some uuencoded data was seen with a 60
      character line length. */

      if (Length > 0 && Length <= g_MaxWordLength)
        WordSet.insert (AccumulatedWord);

      /* Empty out the string to get ready for the next word. */

      AccumulatedWord.resize (0);
    }
    else /* Not a space-like character, add it to the word. */
      AccumulatedWord.append (1 /* one copy of the char */, (char) Letter);

    /* Stop at end of file or error.  Don't care which.  Exit here so that last
    word got processed. */

    if (Letter == EOF)
      break;
  }

  return B_OK;
}



/******************************************************************************
 * Implementation of the ClassificationChoicesView class, constructor,
 * destructor and the rest of the member functions in mostly alphabetical
 * order.
 */

ClassificationChoicesWindow::ClassificationChoicesWindow (
  BRect FrameRect,
  const char *FileName,
  int NumberOfFiles)
: BWindow (FrameRect, "Classification Choices", B_TITLED_WINDOW,
    B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
  m_BulkModeSelectedPntr (NULL),
  m_ChoosenClassificationPntr (NULL)
{
  ClassificationChoicesView *SubViewPntr;

  SubViewPntr = new ClassificationChoicesView (Bounds(),
    FileName, NumberOfFiles);
  AddChild (SubViewPntr);
  SubViewPntr->ResizeToPreferred ();
  ResizeTo (SubViewPntr->Frame().Width(), SubViewPntr->Frame().Height());
}


void ClassificationChoicesWindow::MessageReceived (BMessage *MessagePntr)
{
  BControl *ControlPntr;

  if (MessagePntr->what >= MSG_CLASS_BUTTONS &&
  MessagePntr->what < MSG_CLASS_BUTTONS + CL_MAX)
  {
    if (m_ChoosenClassificationPntr != NULL)
      *m_ChoosenClassificationPntr =
        (ClassificationTypes) (MessagePntr->what - MSG_CLASS_BUTTONS);
    PostMessage (B_QUIT_REQUESTED); // Close and destroy the window.
    return;
  }

  if (MessagePntr->what == MSG_BULK_CHECKBOX)
  {
    if (m_BulkModeSelectedPntr != NULL &&
    MessagePntr->FindPointer ("source", (void **) &ControlPntr) == B_OK)
      *m_BulkModeSelectedPntr = (ControlPntr->Value() == B_CONTROL_ON);
    return;
  }

  if (MessagePntr->what == MSG_CANCEL_BUTTON)
  {
    PostMessage (B_QUIT_REQUESTED); // Close and destroy the window.
    return;
  }

  BWindow::MessageReceived (MessagePntr);
}


void ClassificationChoicesWindow::Go (
  bool *BulkModeSelectedPntr,
  ClassificationTypes *ChoosenClassificationPntr)
{
  status_t  ErrorCode = 0;
  BView    *MainViewPntr;
  thread_id WindowThreadID;

  m_BulkModeSelectedPntr = BulkModeSelectedPntr;
  m_ChoosenClassificationPntr = ChoosenClassificationPntr;
  if (m_ChoosenClassificationPntr != NULL)
    *m_ChoosenClassificationPntr = CL_MAX;

  Show (); // Starts the window thread running.

  /* Move the window to the center of the screen it is now being displayed on
  (have to wait for it to be showing). */

  Lock ();
  MainViewPntr = FindView ("ClassificationChoicesView");
  if (MainViewPntr != NULL)
  {
    BRect   TempRect;
    BScreen TempScreen (this);
    float   X;
    float   Y;

    TempRect = TempScreen.Frame ();
    X = TempRect.Width() / 2;
    Y = TempRect.Height() / 2;
    TempRect = MainViewPntr->Frame();
    X -= TempRect.Width() / 2;
    Y -= TempRect.Height() / 2;
    MoveTo (ceilf (X), ceilf (Y));
  }
  Unlock ();

  /* Wait for the window to go away. */

  WindowThreadID = Thread ();
  if (WindowThreadID >= 0)
    // Delay until the window thread has died, presumably window deleted now.
    wait_for_thread (WindowThreadID, &ErrorCode);
}



/******************************************************************************
 * Implementation of the ClassificationChoicesView class, constructor,
 * destructor and the rest of the member functions in mostly alphabetical
 * order.
 */

ClassificationChoicesView::ClassificationChoicesView (
  BRect FrameRect,
  const char *FileName,
  int NumberOfFiles)
: BView (FrameRect, "ClassificationChoicesView",
    B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW | B_NAVIGABLE_JUMP),
  m_FileName (FileName),
  m_NumberOfFiles (NumberOfFiles),
  m_PreferredBottomY (ceilf (g_ButtonHeight * 10))
{
}


void ClassificationChoicesView::AttachedToWindow ()
{
  BButton            *ButtonPntr;
  BCheckBox          *CheckBoxPntr;
  ClassificationTypes Classification;
  float               Margin;
  float               RowHeight;
  float               RowTop;
  BTextView          *TextViewPntr;
  BRect               TempRect;
  char                TempString [2048];
  BRect               TextRect;
  float               X;

  SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));

  RowHeight = g_ButtonHeight;
  if (g_CheckBoxHeight > RowHeight)
    RowHeight = g_CheckBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  TempRect = Bounds ();
  RowTop = TempRect.top;

  /* Show the file name text. */

  Margin = ceilf ((RowHeight - g_StringViewHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TextRect = TempRect;
  TextRect.OffsetTo (0, 0);
  TextRect.InsetBy (g_MarginBetweenControls, 2);
  sprintf (TempString, "How do you want to classify the file named \"%s\"?",
    m_FileName);
  TextViewPntr = new BTextView (TempRect, "FileText", TextRect,
    B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
  AddChild (TextViewPntr);
  TextViewPntr->SetText (TempString);
  TextViewPntr->MakeEditable (false);
  TextViewPntr->SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));
  TextViewPntr->ResizeTo (TempRect.Width (),
    3 + TextViewPntr->TextHeight (0, sizeof (TempString)));
  RowTop = TextViewPntr->Frame().bottom + Margin;

  /* Make the classification buttons. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  X = Bounds().left + g_MarginBetweenControls;
  for (Classification = (ClassificationTypes) 0; Classification < CL_MAX;
  Classification = (ClassificationTypes) ((int) Classification + 1))
  {
    TempRect = Bounds ();
    TempRect.top = RowTop + Margin;
    TempRect.left = X;
    sprintf (TempString, "%s Button",
      g_ClassificationTypeNames [Classification]);
    ButtonPntr = new BButton (TempRect, TempString,
      g_ClassificationTypeNames [Classification], new BMessage (
      ClassificationChoicesWindow::MSG_CLASS_BUTTONS + Classification));
    AddChild (ButtonPntr);
    ButtonPntr->ResizeToPreferred ();
    X = ButtonPntr->Frame().right + 3 * g_MarginBetweenControls;
  }
  RowTop += ceilf (RowHeight * 1.2);

  /* Make the Cancel button. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.left += g_MarginBetweenControls;
  ButtonPntr = new BButton (TempRect, "Cancel Button",
    "Cancel", new BMessage (ClassificationChoicesWindow::MSG_CANCEL_BUTTON));
  AddChild (ButtonPntr);
  ButtonPntr->ResizeToPreferred ();
  X = ButtonPntr->Frame().right + g_MarginBetweenControls;

  /* Make the checkbox for bulk operations. */

  if (m_NumberOfFiles > 1)
  {
    Margin = ceilf ((RowHeight - g_CheckBoxHeight) / 2);
    TempRect = Bounds ();
    TempRect.top = RowTop + Margin;
    TempRect.left = X;
    sprintf (TempString, "Mark all %d remaining messages the same way.",
      m_NumberOfFiles - 1);
    CheckBoxPntr = new BCheckBox (TempRect, "BulkBox", TempString,
      new BMessage (ClassificationChoicesWindow::MSG_BULK_CHECKBOX));
    AddChild (CheckBoxPntr);
    CheckBoxPntr->ResizeToPreferred ();
  }
  RowTop += RowHeight;

  m_PreferredBottomY = RowTop;
}


void ClassificationChoicesView::GetPreferredSize (float *width, float *height)
{
  if (width != NULL)
    *width = Bounds().Width();
  if (height != NULL)
    *height = m_PreferredBottomY;
}



/******************************************************************************
 * Implementation of the CommanderLooper class, constructor, destructor and the
 * rest of the member functions in mostly alphabetical order.
 */

CommanderLooper::CommanderLooper ()
: BLooper ("CommanderLooper", B_NORMAL_PRIORITY),
  m_IsBusy (false)
{
}


CommanderLooper::~CommanderLooper ()
{
  g_CommanderLooperPntr = NULL;
  delete g_CommanderMessenger;
  g_CommanderMessenger = NULL;
}


/* Process some command line arguments.  Basically just send a message to this
looper itself to do the work later.  That way the caller can continue doing
whatever they're doing, particularly if it's the BApplication. */

void CommanderLooper::CommandArguments (int argc, char **argv)
{
  int      i;
  BMessage InternalMessage;

  InternalMessage.what = MSG_COMMAND_ARGUMENTS;
  for (i = 0; i < argc; i++)
    InternalMessage.AddString ("arg", argv[i]);

  PostMessage (&InternalMessage);
}


/* Copy the refs out of the given message and stuff them into an internal
message to ourself (so that the original message can be returned to the caller,
and if it is Tracker, it can close the file handles it has open).  Optionally
allow preset classification rather than asking the user (set BulkMode to TRUE
and specify the class with BulkClassification). */

void CommanderLooper::CommandReferences (
  BMessage *MessagePntr,
  bool BulkMode,
  ClassificationTypes BulkClassification)
{
  entry_ref EntryRef;
  int       i;
  BMessage  InternalMessage;

  InternalMessage.what = MSG_COMMAND_FILE_REFS;
  for (i = 0; MessagePntr->FindRef ("refs", i, &EntryRef) == B_OK; i++)
    InternalMessage.AddRef ("refs", &EntryRef);
  InternalMessage.AddBool ("BulkMode", BulkMode);
  InternalMessage.AddInt32 ("BulkClassification", BulkClassification);

  PostMessage (&InternalMessage);
}


/* This function is called by other threads to see if the CommanderLooper is
busy working on something. */

bool CommanderLooper::IsBusy ()
{
  if (m_IsBusy)
    return true;

  if (IsLocked () || !MessageQueue()->IsEmpty ())
    return true;

  return false;
}


void CommanderLooper::MessageReceived (BMessage *MessagePntr)
{
  m_IsBusy = true;

  if (MessagePntr->what == MSG_COMMAND_ARGUMENTS)
    ProcessArgs (MessagePntr);
  else if (MessagePntr->what == MSG_COMMAND_FILE_REFS)
    ProcessRefs (MessagePntr);
  else
    BLooper::MessageReceived (MessagePntr);

  m_IsBusy = false;
}


/* Process the command line by converting it into a series of scripting
messages (possibly thousands) and sent them to the BApplication synchronously
(so we can print the result). */

void CommanderLooper::ProcessArgs (BMessage *MessagePntr)
{
  int32                 argc = 0;
  const char          **argv = NULL;
  int                   ArgumentIndex;
  uint32                CommandCode;
  const char           *CommandWord;
  status_t              ErrorCode;
  const char           *ErrorTitle = "ProcessArgs";
  char                 *EndPntr;
  int32                 i;
  BMessage              ReplyMessage;
  BMessage              ScriptMessage;
  struct property_info *PropInfoPntr;
  const char           *PropertyName;
  bool                  TempBool;
  float                 TempFloat;
  int32                 TempInt32;
  const char           *TempStringPntr;
  type_code             TypeCode;
  const char           *ValuePntr;

  /* Get the argument count and pointers to arguments out of the message and
  into our argc and argv. */

  ErrorCode = MessagePntr->GetInfo ("arg", &TypeCode, &argc);
  if (ErrorCode != B_OK || TypeCode != B_STRING_TYPE)
  {
    DisplayErrorMessage ("Unable to find argument strings in message",
      ErrorCode, ErrorTitle);
    goto ErrorExit;
  }

  if (argc < 2)
  {
    cerr << PrintUsage;
    DisplayErrorMessage ("You need to specify a command word, like GET, SET "
      "and so on followed by a property, like DatabaseFile, and maybe "
      "followed by a value of some sort", -1, ErrorTitle);
    goto ErrorExit;
  }

  argv = (const char **) malloc (sizeof (char *) * argc);
  if (argv == NULL)
  {
    DisplayErrorMessage ("Out of memory when allocating argv array",
      ENOMEM, ErrorTitle);
    goto ErrorExit;
  }

  for (i = 0; i < argc; i++)
  {
    if ((ErrorCode = MessagePntr->FindString ("arg", i, &argv[i])) != B_OK)
    {
      DisplayErrorMessage ("Unable to find argument in the BMessage",
        ErrorCode, ErrorTitle);
      goto ErrorExit;
    }
  }

  CommandWord = argv[1];

  /* Special case for the Quit command since it isn't a scripting command. */

  if (strcasecmp (CommandWord, "quit") == 0)
  {
    g_QuitCountdown = 10;
    goto ErrorExit;
  }

  /* Find the corresponding scripting command. */

  if (strcasecmp (CommandWord, "set") == 0)
    CommandCode = B_SET_PROPERTY;
  else if (strcasecmp (CommandWord, "get") == 0)
    CommandCode = B_GET_PROPERTY;
  else if (strcasecmp (CommandWord, "count") == 0)
    CommandCode = B_COUNT_PROPERTIES;
  else if (strcasecmp (CommandWord, "create") == 0)
    CommandCode = B_CREATE_PROPERTY;
  else if (strcasecmp (CommandWord, "delete") == 0)
    CommandCode = B_DELETE_PROPERTY;
  else
    CommandCode = B_EXECUTE_PROPERTY;

  if (CommandCode == B_EXECUTE_PROPERTY)
  {
    PropertyName = CommandWord;
    ArgumentIndex = 2; /* Arguments to the command start at this index. */
  }
  else
  {
    if (CommandCode == B_SET_PROPERTY)
    {
      /* SET commands require at least one argument value. */
      if (argc < 4)
      {
        cerr << PrintUsage;
        DisplayErrorMessage ("SET commands require at least one "
          "argument value after the property name", -1, ErrorTitle);
        goto ErrorExit;
      }
    }
    else
      if (argc < 3)
      {
        cerr << PrintUsage;
        DisplayErrorMessage ("You need to specify a property to act on",
          -1, ErrorTitle);
        goto ErrorExit;
      }
    PropertyName = argv[2];
    ArgumentIndex = 3;
  }

  /* See if it is one of our commands. */

  for (PropInfoPntr = g_ScriptingPropertyList + 0; true; PropInfoPntr++)
  {
    if (PropInfoPntr->name == 0)
    {
      cerr << PrintUsage;
      DisplayErrorMessage ("The property specified isn't known or "
        "doesn't support the requested action (usually means it is an "
        "unknown command)", -1, ErrorTitle);
      goto ErrorExit; /* Unrecognized command. */
    }

    if (PropInfoPntr->commands[0] == CommandCode &&
    strcasecmp (PropertyName, PropInfoPntr->name) == 0)
      break;
  }

  /* Make the equivalent command message.  For commands with multiple
  arguments, repeat the message for each single argument and just change the
  data portion for each extra argument.  Send the command and wait for a reply,
  which we'll print out. */

  ScriptMessage.MakeEmpty ();
  ScriptMessage.what = CommandCode;
  ScriptMessage.AddSpecifier (PropertyName);
  while (true)
  {
    if (ArgumentIndex < argc) /* If there are arguments to be added. */
    {
      ValuePntr = argv[ArgumentIndex];

      /* Convert the value into the likely kind of data. */

      if (strcasecmp (ValuePntr, "yes") == 0 ||
      strcasecmp (ValuePntr, "true") == 0)
        ScriptMessage.AddBool (g_DataName, true);
      else if (strcasecmp (ValuePntr, "no") == 0 ||
      strcasecmp (ValuePntr, "false") == 0)
        ScriptMessage.AddBool (g_DataName, false);
      else
      {
        /* See if it is a number. */
        i = strtol (ValuePntr, &EndPntr, 0);
        if (*EndPntr == 0)
          ScriptMessage.AddInt32 (g_DataName, i);
        else /* Nope, it's just a string. */
          ScriptMessage.AddString (g_DataName, ValuePntr);
      }
    }

    ErrorCode = be_app_messenger.SendMessage (&ScriptMessage, &ReplyMessage);
    if (ErrorCode != B_OK)
    {
      DisplayErrorMessage ("Unable to send scripting command",
        ErrorCode, ErrorTitle);
      goto ErrorExit;
    }

    /* Print the reply to the scripting command.  Even in server mode.  To
    standard output. */

    if (ReplyMessage.FindString ("CommandText", &TempStringPntr) == B_OK)
    {
      TempInt32 = -1;
      if (ReplyMessage.FindInt32 ("error", &TempInt32) == B_OK &&
      TempInt32 == B_OK)
      {
        /* It's a successful reply to one of our scripting messages.  Print out
        the returned values code for command line users to see. */

        cout << "Result of command to " << TempStringPntr << " is:\t";
        if (ReplyMessage.FindString (g_ResultName, &TempStringPntr) == B_OK)
          cout << "\"" << TempStringPntr << "\"";
        else if (ReplyMessage.FindInt32 (g_ResultName, &TempInt32) == B_OK)
          cout << TempInt32;
        else if (ReplyMessage.FindFloat (g_ResultName, &TempFloat) == B_OK)
          cout << TempFloat;
        else if (ReplyMessage.FindBool (g_ResultName, &TempBool) == B_OK)
          cout << (TempBool ? "true" : "false");
        else
          cout << "just plain success";
        if (ReplyMessage.FindInt32 ("count", &TempInt32) == B_OK)
          cout << "\t(count " << TempInt32 << ")";
        for (i = 0; (i < 50) &&
        ReplyMessage.FindString ("words", i, &TempStringPntr) == B_OK &&
        ReplyMessage.FindFloat ("ratios", i, &TempFloat) == B_OK;
        i++)
        {
          if (i == 0)
            cout << "\twith top words:\t";
          else
            cout << "\t";
          cout << TempStringPntr << "/" << TempFloat;
        }
        cout << endl;
      }
      else /* An error reply, print out the error, even in server mode. */
      {
        cout << "Failure of command " << TempStringPntr << ", error ";
        cout << TempInt32 << " (" << strerror (TempInt32) << ")";
        if (ReplyMessage.FindString ("message", &TempStringPntr) == B_OK)
          cout << ", message: " << TempStringPntr;
        cout << "." << endl;
      }
    }

    /* Advance to the next argument and its scripting message. */

    ScriptMessage.RemoveName (g_DataName);
    if (++ArgumentIndex >= argc)
      break;
  }

ErrorExit:
  free (argv);
}


/* Given a bunch of references to files, open the files.  If it's a database
file, switch to using it as a database.  Otherwise, treat them as text files
and add them to the database.  Prompt the user for the spam or genuine or
uncertain (declassification) choice, with the option to bulk mark many files at
once. */

void CommanderLooper::ProcessRefs (BMessage *MessagePntr)
{
  bool                         BulkMode = false;
  ClassificationTypes          BulkClassification = CL_GENUINE;
  ClassificationChoicesWindow *ChoiceWindowPntr;
  BEntry                       Entry;
  entry_ref                    EntryRef;
  status_t                     ErrorCode;
  const char                  *ErrorTitle = "CommanderLooper::ProcessRefs";
  int32                        NumberOfRefs = 0;
  BPath                        Path;
  int                          RefIndex;
  BMessage                     ReplyMessage;
  BMessage                     ScriptingMessage;
  bool                         TempBool;
  BFile                        TempFile;
  int32                        TempInt32;
  char                         TempString [PATH_MAX + 1024];
  type_code                    TypeCode;

  // Wait for ReadyToRun to finish initializing the globals with the sizes of
  // the controls, since they are needed when we show the custom alert box for
  // choosing the message type.

  TempInt32 = 0;
  while (!g_AppReadyToRunCompleted && TempInt32++ < 10)
    snooze (200000);

  ErrorCode = MessagePntr->GetInfo ("refs", &TypeCode, &NumberOfRefs);
  if (ErrorCode != B_OK || TypeCode != B_REF_TYPE || NumberOfRefs <= 0)
  {
    DisplayErrorMessage ("Unable to get refs from the message",
      ErrorCode, ErrorTitle);
    return;
  }

  if (MessagePntr->FindBool ("BulkMode", &TempBool) == B_OK)
    BulkMode = TempBool;
  if (MessagePntr->FindInt32 ("BulkClassification", &TempInt32) == B_OK &&
  TempInt32 >= 0 && TempInt32 < CL_MAX)
    BulkClassification = (ClassificationTypes) TempInt32;

  for (RefIndex = 0;
  MessagePntr->FindRef ("refs", RefIndex, &EntryRef) == B_OK;
  RefIndex++)
  {
    ScriptingMessage.MakeEmpty ();
    ScriptingMessage.what = 0; /* Haven't figured out what to do yet. */

    /* See if the entry is a valid file or directory or other thing. */

    ErrorCode = Entry.SetTo (&EntryRef, true /* traverse symbolic links */);
    if (ErrorCode != B_OK ||
    ((ErrorCode = /* assignment */ B_ENTRY_NOT_FOUND) != 0 /* this pacifies
    mwcc -nwhitehorn */ && !Entry.Exists ()) ||
    ((ErrorCode = Entry.GetPath (&Path)) != B_OK))
    {
      DisplayErrorMessage ("Bad entry reference encountered, will skip it",
        ErrorCode, ErrorTitle);
      BulkMode = false;
      continue; /* Bad file reference, try the next one. */
    }

    /* If it's a file, check if it is a spam database file.  Go by the magic
    text at the start of the file, in case someone has edited the file with a
    spreadsheet or other tool and lost the MIME type. */

    if (Entry.IsFile ())
    {
      ErrorCode = TempFile.SetTo (&Entry, B_READ_ONLY);
      if (ErrorCode != B_OK)
      {
        sprintf (TempString, "Unable to open file \"%s\" for reading, will "
          "skip it", Path.Path ());
        DisplayErrorMessage (TempString, ErrorCode, ErrorTitle);
        BulkMode = false;
        continue;
      }
      if (TempFile.Read (TempString, strlen (g_DatabaseRecognitionString)) ==
      (int) strlen (g_DatabaseRecognitionString) && strncmp (TempString,
      g_DatabaseRecognitionString, strlen (g_DatabaseRecognitionString)) == 0)
      {
        ScriptingMessage.what = B_SET_PROPERTY;
        ScriptingMessage.AddSpecifier (g_PropertyNames[PN_DATABASE_FILE]);
        ScriptingMessage.AddString (g_DataName, Path.Path ());
      }
      TempFile.Unset ();
    }

    /* Not a database file.  Could be a directory or a file.  Submit it as
    something to be marked spam or genuine. */

    if (ScriptingMessage.what == 0)
    {
      if (!Entry.IsFile ())
      {
        sprintf (TempString, "\"%s\" is not a file, can't do anything with it",
          Path.Path ());
        DisplayErrorMessage (TempString, -1, ErrorTitle);
        BulkMode = false;
        continue;
      }

      if (!BulkMode) /* Have to ask the user. */
      {
        ChoiceWindowPntr = new ClassificationChoicesWindow (
          BRect (40, 40, 40 + 50 * g_MarginBetweenControls,
          40 + g_ButtonHeight * 5), Path.Path (), NumberOfRefs - RefIndex);
        ChoiceWindowPntr->Go (&BulkMode, &BulkClassification);
        if (BulkClassification == CL_MAX)
          break; /* Cancel was picked. */
      }

      /* Format the command for classifying the file. */

      ScriptingMessage.what = B_SET_PROPERTY;

      if (BulkClassification == CL_GENUINE)
        ScriptingMessage.AddSpecifier (g_PropertyNames[PN_GENUINE]);
      else if (BulkClassification == CL_SPAM)
        ScriptingMessage.AddSpecifier (g_PropertyNames[PN_SPAM]);
      else if (BulkClassification == CL_UNCERTAIN)
        ScriptingMessage.AddSpecifier (g_PropertyNames[PN_UNCERTAIN]);
      else /* Broken code */
        break;
      ScriptingMessage.AddString (g_DataName, Path.Path ());
    }

    /* Tell the BApplication to do the work, and wait for it to finish.  The
    BApplication will display any error messages for us. */

    ErrorCode =
      be_app_messenger.SendMessage (&ScriptingMessage, &ReplyMessage);
    if (ErrorCode != B_OK)
    {
      DisplayErrorMessage ("Unable to send scripting command",
        ErrorCode, ErrorTitle);
      return;
    }

    /* If there was an error, allow the user to stop by switching off bulk
    mode.  The message will already have been displayed in an alert box, if
    server mode is off. */

    if (ReplyMessage.FindInt32 ("error", &TempInt32) != B_OK ||
    TempInt32 != B_OK)
      BulkMode = false;
  }
}



/******************************************************************************
 * Implementation of the ControlsView class, constructor, destructor and the
 * rest of the member functions in mostly alphabetical order.
 */

ControlsView::ControlsView (BRect NewBounds)
: BView (NewBounds, "ControlsView", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
    B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE_JUMP | B_FRAME_EVENTS),
  m_AboutButtonPntr (NULL),
  m_AddExampleButtonPntr (NULL),
  m_BrowseButtonPntr (NULL),
  m_BrowseFilePanelPntr (NULL),
  m_CreateDatabaseButtonPntr (NULL),
  m_DatabaseFileNameTextboxPntr (NULL),
  m_DatabaseLoadDone (false),
  m_EstimateSpamButtonPntr (NULL),
  m_EstimateSpamFilePanelPntr (NULL),
  m_GenuineCountTextboxPntr (NULL),
  m_IgnorePreviousClassCheckboxPntr (NULL),
  m_InstallThingsButtonPntr (NULL),
  m_PurgeAgeTextboxPntr (NULL),
  m_PurgeButtonPntr (NULL),
  m_PurgePopularityTextboxPntr (NULL),
  m_ResetToDefaultsButtonPntr (NULL),
  m_ScoringModeMenuBarPntr (NULL),
  m_ScoringModePopUpMenuPntr (NULL),
  m_ServerModeCheckboxPntr (NULL),
  m_SpamCountTextboxPntr (NULL),
  m_TimeOfLastPoll (0),
  m_TokenizeModeMenuBarPntr (NULL),
  m_TokenizeModePopUpMenuPntr (NULL),
  m_WordCountTextboxPntr (NULL)
{
}


ControlsView::~ControlsView ()
{
  if (m_BrowseFilePanelPntr != NULL)
  {
    delete m_BrowseFilePanelPntr;
    m_BrowseFilePanelPntr = NULL;
  }

  if (m_EstimateSpamFilePanelPntr != NULL)
  {
    delete m_EstimateSpamFilePanelPntr;
    m_EstimateSpamFilePanelPntr = NULL;
  }
}


void ControlsView::AttachedToWindow ()
{
  float         BigPurgeButtonTop;
  BMessage      CommandMessage;
  const char   *EightDigitsString = " 12345678 ";
  float         Height;
  float         Margin;
  float         RowHeight;
  float         RowTop;
  ScoringModes  ScoringMode;
  const char   *StringPntr;
  BMenuItem    *TempMenuItemPntr;
  BRect         TempRect;
  char          TempString [PATH_MAX];
  TokenizeModes TokenizeMode;
  float         Width;
  float         X;

  SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));

  TempRect = Bounds ();
  X = TempRect.right;
  RowTop = TempRect.top;
  RowHeight = g_ButtonHeight;
  if (g_TextBoxHeight > RowHeight)
    RowHeight = g_TextBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  /* Make the Create button at the far right of the first row of controls,
  which are all database file related. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;

  CommandMessage.MakeEmpty ();
  CommandMessage.what = B_CREATE_PROPERTY;
  CommandMessage.AddSpecifier (g_PropertyNames[PN_DATABASE_FILE]);
  m_CreateDatabaseButtonPntr = new BButton (TempRect, "Create Button",
    "Create", new BMessage (CommandMessage), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
  if (m_CreateDatabaseButtonPntr == NULL) goto ErrorExit;
  AddChild (m_CreateDatabaseButtonPntr);
  m_CreateDatabaseButtonPntr->SetTarget (be_app);
  m_CreateDatabaseButtonPntr->ResizeToPreferred ();
  m_CreateDatabaseButtonPntr->GetPreferredSize (&Width, &Height);
  m_CreateDatabaseButtonPntr->MoveTo (X - Width, TempRect.top);
  X -= Width + g_MarginBetweenControls;

  /* Make the Browse button, middle of the first row. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;

  m_BrowseButtonPntr = new BButton (TempRect, "Browse Button",
    "Browse…", new BMessage (MSG_BROWSE_BUTTON), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
  if (m_BrowseButtonPntr == NULL) goto ErrorExit;
  AddChild (m_BrowseButtonPntr);
  m_BrowseButtonPntr->SetTarget (this);
  m_BrowseButtonPntr->ResizeToPreferred ();
  m_BrowseButtonPntr->GetPreferredSize (&Width, &Height);
  m_BrowseButtonPntr->MoveTo (X - Width, TempRect.top);
  X -= Width + g_MarginBetweenControls;

  /* Fill the rest of the space on the first row with the file name box. */

  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  TempRect.right = X;

  StringPntr = "Word Database:";
  strcpy (m_DatabaseFileNameCachedValue, "Unknown...");
  m_DatabaseFileNameTextboxPntr = new BTextControl (TempRect,
    "File Name",
    StringPntr /* label */,
    m_DatabaseFileNameCachedValue /* text */,
    new BMessage (MSG_DATABASE_NAME),
    B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
    B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
  AddChild (m_DatabaseFileNameTextboxPntr);
  m_DatabaseFileNameTextboxPntr->SetTarget (this);
  m_DatabaseFileNameTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);

  /* Second row contains the purge age, and a long line explaining it.  There
  is space to the right where the top half of the big purge button will go. */

  RowTop += RowHeight /* previous row's RowHeight */;
  BigPurgeButtonTop = RowTop;
  TempRect = Bounds ();
  X = TempRect.left;
  RowHeight = g_TextBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  StringPntr = "Number of occurrences needed to store a word:";
  m_PurgeAgeCachedValue = 12345678;

  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  TempRect.left = X;
  TempRect.right = TempRect.left +
    be_plain_font->StringWidth (StringPntr) +
    be_plain_font->StringWidth (EightDigitsString) +
    3 * g_MarginBetweenControls;

  sprintf (TempString, "%d", (int) m_PurgeAgeCachedValue);
  m_PurgeAgeTextboxPntr = new BTextControl (TempRect,
    "Purge Age",
    StringPntr /* label */,
    TempString /* text */,
    new BMessage (MSG_PURGE_AGE),
    B_FOLLOW_LEFT | B_FOLLOW_TOP,
    B_WILL_DRAW | B_NAVIGABLE);
  AddChild (m_PurgeAgeTextboxPntr);
  m_PurgeAgeTextboxPntr->SetTarget (this);
  m_PurgeAgeTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);

  /* Third row contains the purge popularity and bottom half of the purge
  button. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  X = TempRect.left;
  RowHeight = g_TextBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  StringPntr = "Number of messages to store words from:";
  m_PurgePopularityCachedValue = 87654321;
  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  TempRect.left = X;
  TempRect.right = TempRect.left +
    be_plain_font->StringWidth (StringPntr) +
    be_plain_font->StringWidth (EightDigitsString) +
    3 * g_MarginBetweenControls;
  X = TempRect.right + g_MarginBetweenControls;

  sprintf (TempString, "%d", (int) m_PurgePopularityCachedValue);
  m_PurgePopularityTextboxPntr = new BTextControl (TempRect,
    "Purge Popularity",
    StringPntr /* label */,
    TempString /* text */,
    new BMessage (MSG_PURGE_POPULARITY),
    B_FOLLOW_LEFT | B_FOLLOW_TOP,
    B_WILL_DRAW | B_NAVIGABLE);
  AddChild (m_PurgePopularityTextboxPntr);
  m_PurgePopularityTextboxPntr->SetTarget (this);
  m_PurgePopularityTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);

  /* Make the purge button, which will take up space in the 2nd and 3rd rows,
  on the right side.  Twice as tall as a regular button too. */

  StringPntr = "Remove Old Words";
  Margin = ceilf ((((RowTop + RowHeight) - BigPurgeButtonTop) -
    2 * g_TextBoxHeight) / 2);
  TempRect.top = BigPurgeButtonTop + Margin;
  TempRect.bottom = TempRect.top + 2 * g_TextBoxHeight;
  TempRect.left = X;
  TempRect.right = X + ceilf (2 * be_plain_font->StringWidth (StringPntr));

  CommandMessage.MakeEmpty ();
  CommandMessage.what = B_EXECUTE_PROPERTY;
  CommandMessage.AddSpecifier (g_PropertyNames[PN_PURGE]);
  m_PurgeButtonPntr = new BButton (TempRect, "Purge Button",
    StringPntr, new BMessage (CommandMessage), B_FOLLOW_LEFT | B_FOLLOW_TOP);
  if (m_PurgeButtonPntr == NULL) goto ErrorExit;
  m_PurgeButtonPntr->ResizeToPreferred();
  AddChild (m_PurgeButtonPntr);
  m_PurgeButtonPntr->SetTarget (be_app);

  /* The fourth row contains the ignore previous classification checkbox. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  X = TempRect.left;
  RowHeight = g_CheckBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  StringPntr = "Allow Retraining on a Message";
  m_IgnorePreviousClassCachedValue = false;

  Margin = ceilf ((RowHeight - g_CheckBoxHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_CheckBoxHeight;
  TempRect.left = X;
  m_IgnorePreviousClassCheckboxPntr = new BCheckBox (TempRect,
    "Ignore Check",
    StringPntr,
    new BMessage (MSG_IGNORE_CLASSIFICATION),
    B_FOLLOW_TOP | B_FOLLOW_LEFT);
  if (m_IgnorePreviousClassCheckboxPntr == NULL) goto ErrorExit;
  AddChild (m_IgnorePreviousClassCheckboxPntr);
  m_IgnorePreviousClassCheckboxPntr->SetTarget (this);
  m_IgnorePreviousClassCheckboxPntr->ResizeToPreferred ();
  m_IgnorePreviousClassCheckboxPntr->GetPreferredSize (&Width, &Height);
  X += Width + g_MarginBetweenControls;

  /* The fifth row contains the server mode checkbox. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  RowHeight = g_CheckBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  StringPntr = "Print errors to Terminal";
  m_ServerModeCachedValue = false;

  Margin = ceilf ((RowHeight - g_CheckBoxHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_CheckBoxHeight;
  m_ServerModeCheckboxPntr = new BCheckBox (TempRect,
    "ServerMode Check",
    StringPntr,
    new BMessage (MSG_SERVER_MODE),
    B_FOLLOW_TOP | B_FOLLOW_LEFT);
  if (m_ServerModeCheckboxPntr == NULL) goto ErrorExit;
  AddChild (m_ServerModeCheckboxPntr);
  m_ServerModeCheckboxPntr->SetTarget (this);
  m_ServerModeCheckboxPntr->ResizeToPreferred ();
  m_ServerModeCheckboxPntr->GetPreferredSize (&Width, &Height);

  /* This row just contains a huge pop-up menu which shows the tokenize mode
  and an explanation of what each mode does. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  RowHeight = g_PopUpMenuHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  Margin = ceilf ((RowHeight - g_PopUpMenuHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_PopUpMenuHeight;

  m_TokenizeModeCachedValue = TM_MAX; /* Illegal value will force redraw. */
  m_TokenizeModeMenuBarPntr = new BMenuBar (TempRect, "TokenizeModeMenuBar",
    B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_COLUMN,
    false /* resize to fit items */);
  if (m_TokenizeModeMenuBarPntr == NULL) goto ErrorExit;
  m_TokenizeModePopUpMenuPntr = new BPopUpMenu ("TokenizeModePopUpMenu");
  if (m_TokenizeModePopUpMenuPntr == NULL) goto ErrorExit;

  for (TokenizeMode = (TokenizeModes) 0;
  TokenizeMode < TM_MAX;
  TokenizeMode = (TokenizeModes) ((int) TokenizeMode + 1))
  {
    /* Each different tokenize mode gets its own menu item.  Selecting the item
    will send a canned command to the application to switch to the appropriate
    tokenize mode.  An optional explanation of each mode is added to the mode
    name string. */

    CommandMessage.MakeEmpty ();
    CommandMessage.what = B_SET_PROPERTY;
    CommandMessage.AddSpecifier (g_PropertyNames[PN_TOKENIZE_MODE]);
    CommandMessage.AddString (g_DataName, g_TokenizeModeNames[TokenizeMode]);
    strcpy (TempString, g_TokenizeModeNames[TokenizeMode]);
    switch (TokenizeMode)
    {
      case TM_WHOLE:
        strcat (TempString, " - Scan everything");
        break;

      case TM_PLAIN_TEXT:
        strcat (TempString, " - Scan e-mail body text except rich text");
        break;

      case TM_PLAIN_TEXT_HEADER:
        strcat (TempString, " - Scan entire e-mail text except rich text");
        break;

      case TM_ANY_TEXT:
        strcat (TempString, " - Scan e-mail body text and text attachments");
        break;

      case TM_ANY_TEXT_HEADER:
       strcat (TempString, " - Scan entire e-mail text and text attachments (recommended)");
        break;

      case TM_ALL_PARTS:
        strcat (TempString, " - Scan e-mail body and all attachments");
        break;

      case TM_ALL_PARTS_HEADER:
        strcat (TempString, " - Scan all parts of the e-mail");
        break;

      case TM_JUST_HEADER:
        strcat (TempString, " - Scan just the header (mail routing information)");
        break;

      default:
        break;
    }
    TempMenuItemPntr =
      new BMenuItem (TempString, new BMessage (CommandMessage));
    if (TempMenuItemPntr == NULL) goto ErrorExit;
    TempMenuItemPntr->SetTarget (be_app);
    m_TokenizeModePopUpMenuPntr->AddItem (TempMenuItemPntr);
  }
  m_TokenizeModeMenuBarPntr->AddItem (m_TokenizeModePopUpMenuPntr);
  AddChild (m_TokenizeModeMenuBarPntr);

  /* This row just contains a huge pop-up menu which shows the scoring mode
  and an explanation of what each mode does. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  RowHeight = g_PopUpMenuHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  Margin = ceilf ((RowHeight - g_PopUpMenuHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_PopUpMenuHeight;

  m_ScoringModeCachedValue = SM_MAX; /* Illegal value will force redraw. */
  m_ScoringModeMenuBarPntr = new BMenuBar (TempRect, "ScoringModeMenuBar",
    B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_COLUMN,
    false /* resize to fit items */);
  if (m_ScoringModeMenuBarPntr == NULL) goto ErrorExit;
  m_ScoringModePopUpMenuPntr = new BPopUpMenu ("ScoringModePopUpMenu");
  if (m_ScoringModePopUpMenuPntr == NULL) goto ErrorExit;

  for (ScoringMode = (ScoringModes) 0;
  ScoringMode < SM_MAX;
  ScoringMode = (ScoringModes) ((int) ScoringMode + 1))
  {
    /* Each different scoring mode gets its own menu item.  Selecting the item
    will send a canned command to the application to switch to the appropriate
    scoring mode.  An optional explanation of each mode is added to the mode
    name string. */

    CommandMessage.MakeEmpty ();
    CommandMessage.what = B_SET_PROPERTY;
    CommandMessage.AddSpecifier (g_PropertyNames[PN_SCORING_MODE]);
    CommandMessage.AddString (g_DataName, g_ScoringModeNames[ScoringMode]);
/*
    strcpy (TempString, g_ScoringModeNames[ScoringMode]);
    switch (ScoringMode)
    {
      case SM_ROBINSON:
        strcat (TempString, " - Learning Method 1: Naive Bayesian");
        break;

      case SM_CHISQUARED:
        strcat (TempString, " - Learning Method 2: Chi-Squared");
        break;

      default:
        break;
    }
*/
    switch (ScoringMode)
    {
      case SM_ROBINSON:
        strcpy (TempString, "Learning method 1: Naive Bayesian");
        break;

      case SM_CHISQUARED:
        strcpy (TempString, "Learning method 2: Chi-Squared");
        break;

      default:
        break;
    }
    TempMenuItemPntr =
      new BMenuItem (TempString, new BMessage (CommandMessage));
    if (TempMenuItemPntr == NULL) goto ErrorExit;
    TempMenuItemPntr->SetTarget (be_app);
    m_ScoringModePopUpMenuPntr->AddItem (TempMenuItemPntr);
  }
  m_ScoringModeMenuBarPntr->AddItem (m_ScoringModePopUpMenuPntr);
  AddChild (m_ScoringModeMenuBarPntr);

  /* The next row has the install MIME types button and the reset to defaults
  button, one on the left and the other on the right. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  RowHeight = g_ButtonHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;

  CommandMessage.MakeEmpty ();
  CommandMessage.what = B_EXECUTE_PROPERTY;
  CommandMessage.AddSpecifier (g_PropertyNames[PN_INSTALL_THINGS]);
  m_InstallThingsButtonPntr = new BButton (TempRect, "Install Button",
    "Install spam types",
    new BMessage (CommandMessage),
    B_FOLLOW_LEFT | B_FOLLOW_TOP);
  if (m_InstallThingsButtonPntr == NULL) goto ErrorExit;
  AddChild (m_InstallThingsButtonPntr);
  m_InstallThingsButtonPntr->SetTarget (be_app);
  m_InstallThingsButtonPntr->ResizeToPreferred ();

  /* The Reset to Defaults button.  On the right side of the row. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;

  CommandMessage.MakeEmpty ();
  CommandMessage.what = B_EXECUTE_PROPERTY;
  CommandMessage.AddSpecifier (g_PropertyNames[PN_RESET_TO_DEFAULTS]);
  m_ResetToDefaultsButtonPntr = new BButton (TempRect, "Reset Button",
    "Default settings", new BMessage (CommandMessage),
    B_FOLLOW_RIGHT | B_FOLLOW_TOP);
  if (m_ResetToDefaultsButtonPntr == NULL) goto ErrorExit;
  AddChild (m_ResetToDefaultsButtonPntr);
  m_ResetToDefaultsButtonPntr->SetTarget (be_app);
  m_ResetToDefaultsButtonPntr->ResizeToPreferred ();
  m_ResetToDefaultsButtonPntr->GetPreferredSize (&Width, &Height);
  m_ResetToDefaultsButtonPntr->MoveTo (TempRect.right - Width, TempRect.top);

  /* The next row contains the Estimate, Add Examples and About buttons. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  X = TempRect.left;
  RowHeight = g_ButtonHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;
  TempRect.left = X;

  m_EstimateSpamButtonPntr = new BButton (TempRect, "Estimate Button",
    "Scan a message",
    new BMessage (MSG_ESTIMATE_BUTTON),
    B_FOLLOW_LEFT | B_FOLLOW_TOP);
  if (m_EstimateSpamButtonPntr == NULL) goto ErrorExit;
  AddChild (m_EstimateSpamButtonPntr);
  m_EstimateSpamButtonPntr->SetTarget (this);
  m_EstimateSpamButtonPntr->ResizeToPreferred ();
  X = m_EstimateSpamButtonPntr->Frame().right + g_MarginBetweenControls;

  /* The Add Example button in the middle.  Does the same as the browse button,
  but don't tell anyone that! */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;
  TempRect.left = X;

  m_AddExampleButtonPntr = new BButton (TempRect, "Example Button",
    "Train spam filter on a message",
    new BMessage (MSG_BROWSE_BUTTON),
    B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
    B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
  if (m_AddExampleButtonPntr == NULL) goto ErrorExit;
  AddChild (m_AddExampleButtonPntr);
  m_AddExampleButtonPntr->SetTarget (this);
  m_AddExampleButtonPntr->ResizeToPreferred ();
  X = m_AddExampleButtonPntr->Frame().right + g_MarginBetweenControls;

  /* Add the About button on the right. */

  Margin = ceilf ((RowHeight - g_ButtonHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_ButtonHeight;
  TempRect.left = X;

  m_AboutButtonPntr = new BButton (TempRect, "About Button",
    "About…",
    new BMessage (B_ABOUT_REQUESTED),
    B_FOLLOW_RIGHT | B_FOLLOW_TOP);
  if (m_AboutButtonPntr == NULL) goto ErrorExit;
  AddChild (m_AboutButtonPntr);
  m_AboutButtonPntr->SetTarget (be_app);

  /* This row displays various counters.  Starting with the genuine messages
  count on the left. */

  RowTop += RowHeight /* previous row's RowHeight */;
  TempRect = Bounds ();
  RowHeight = g_TextBoxHeight;
  RowHeight = ceilf (RowHeight * 1.1);

  StringPntr = "Genuine messages:";
  m_GenuineCountCachedValue = 87654321;
  sprintf (TempString, "%d", (int) m_GenuineCountCachedValue);

  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  TempRect.right = TempRect.left +
    be_plain_font->StringWidth (StringPntr) +
    be_plain_font->StringWidth (TempString) +
    3 * g_MarginBetweenControls;

  m_GenuineCountTextboxPntr = new BTextControl (TempRect,
    "Genuine count",
    StringPntr /* label */,
    TempString /* text */,
    NULL /* no message */,
    B_FOLLOW_LEFT | B_FOLLOW_TOP,
    B_WILL_DRAW /* not B_NAVIGABLE */);
  AddChild (m_GenuineCountTextboxPntr);
  m_GenuineCountTextboxPntr->SetTarget (this); /* Not that it matters. */
  m_GenuineCountTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);
  m_GenuineCountTextboxPntr->SetEnabled (false); /* For display only. */

  /* The word count in the center. */

  StringPntr = "Word count:";
  m_WordCountCachedValue = 87654321;
  sprintf (TempString, "%d", (int) m_WordCountCachedValue);

  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  Width = be_plain_font->StringWidth (StringPntr) +
    be_plain_font->StringWidth (TempString) +
    3 * g_MarginBetweenControls;
  TempRect.left = ceilf ((TempRect.right - TempRect.left) / 2 - Width / 2);
  TempRect.right = TempRect.left + Width;

  m_WordCountTextboxPntr = new BTextControl (TempRect,
    "Word count",
    StringPntr /* label */,
    TempString /* text */,
    NULL /* no message */,
    B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
    B_WILL_DRAW /* not B_NAVIGABLE */);
  AddChild (m_WordCountTextboxPntr);
  m_WordCountTextboxPntr->SetTarget (this); /* Not that it matters. */
  m_WordCountTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);
  m_WordCountTextboxPntr->SetEnabled (false); /* For display only. */

  /* The spam count on the far right. */

  StringPntr = "Spam messages:";
  m_SpamCountCachedValue = 87654321;
  sprintf (TempString, "%d", (int) m_SpamCountCachedValue);

  Margin = ceilf ((RowHeight - g_TextBoxHeight) / 2);
  TempRect = Bounds ();
  TempRect.top = RowTop + Margin;
  TempRect.bottom = TempRect.top + g_TextBoxHeight;
  TempRect.left = TempRect.right -
    be_plain_font->StringWidth (StringPntr) -
    be_plain_font->StringWidth (TempString) -
    3 * g_MarginBetweenControls;

  m_SpamCountTextboxPntr = new BTextControl (TempRect,
    "Spam count",
    StringPntr /* label */,
    TempString /* text */,
    NULL /* no message */,
    B_FOLLOW_RIGHT | B_FOLLOW_TOP,
    B_WILL_DRAW /* not B_NAVIGABLE */);
  AddChild (m_SpamCountTextboxPntr);
  m_SpamCountTextboxPntr->SetTarget (this); /* Not that it matters. */
  m_SpamCountTextboxPntr->SetDivider (
    be_plain_font->StringWidth (StringPntr) + g_MarginBetweenControls);
  m_SpamCountTextboxPntr->SetEnabled (false); /* For display only. */

  /* Change the size of our view so it only takes up the space needed by the
  buttons. */

  RowTop += RowHeight /* previous row's RowHeight */;
  ResizeTo (Bounds().Width(), RowTop - Bounds().top + 1);

  return; /* Successful. */

ErrorExit:
  DisplayErrorMessage ("Unable to initialise the controls view.");
}


void ControlsView::BrowseForDatabaseFile ()
{
  if (m_BrowseFilePanelPntr == NULL)
  {
    BEntry      DirectoryEntry;
    entry_ref   DirectoryEntryRef;
    BMessage    GetDatabasePathCommand;
    BMessage    GetDatabasePathResult;
    const char *StringPntr = NULL;

    /* Create a new file panel.  First set up the entry ref stuff so that the
    file panel can open to show the initial directory (the one where the
    database file currently is).  Note that we have to create it after the
    window and view are up and running, otherwise the BMessenger won't point to
    a valid looper/handler.  First find out the current database file name to
    use as a starting point. */

    GetDatabasePathCommand.what = B_GET_PROPERTY;
    GetDatabasePathCommand.AddSpecifier (g_PropertyNames[PN_DATABASE_FILE]);
    be_app_messenger.SendMessage (&GetDatabasePathCommand,
      &GetDatabasePathResult, 5000000 /* delivery timeout */,
      5000000 /* reply timeout */);
    if (GetDatabasePathResult.FindString (g_ResultName, &StringPntr) != B_OK ||
    DirectoryEntry.SetTo (StringPntr) != B_OK ||
    DirectoryEntry.GetParent (&DirectoryEntry) != B_OK)
      DirectoryEntry.SetTo ("."); /* Default directory if we can't find it. */
    if (DirectoryEntry.GetRef (&DirectoryEntryRef) != B_OK)
    {
      DisplayErrorMessage (
        "Unable to set up the file requestor starting directory.  Sorry.");
      return;
    }

    m_BrowseFilePanelPntr = new BFilePanel (
      B_OPEN_PANEL /* mode */,
      &be_app_messenger /* target for event messages */,
      &DirectoryEntryRef /* starting directory */,
      B_FILE_NODE,
      true /* true for multiple selections */,
      NULL /* canned message */,
      NULL /* ref filter */,
      false /* true for modal */,
      true /* true to hide when done */);
  }

  if (m_BrowseFilePanelPntr != NULL)
    m_BrowseFilePanelPntr->Show (); /* Answer returned later in RefsReceived. */
}


void ControlsView::BrowseForFileToEstimate ()
{
  if (m_EstimateSpamFilePanelPntr == NULL)
  {
    BEntry      DirectoryEntry;
    entry_ref   DirectoryEntryRef;
    status_t    ErrorCode;
    BMessenger  MessengerToSelf (this);
    BPath       PathToMailDirectory;

    /* Create a new file panel.  First set up the entry ref stuff so that the
    file panel can open to show the initial directory (the user's mail
    directory).  Note that we have to create the panel after the window and
    view are up and running, otherwise the BMessenger won't point to a valid
    looper/handler. */

    ErrorCode = find_directory (B_USER_DIRECTORY, &PathToMailDirectory);
    if (ErrorCode == B_OK)
    {
      PathToMailDirectory.Append ("mail");
      ErrorCode = DirectoryEntry.SetTo (PathToMailDirectory.Path(),
        true /* traverse symbolic links*/);
      if (ErrorCode != B_OK || !DirectoryEntry.Exists ())
      {
        /* If no mail directory, try home directory. */
        find_directory (B_USER_DIRECTORY, &PathToMailDirectory);
        ErrorCode = DirectoryEntry.SetTo (PathToMailDirectory.Path(), true);
      }
    }
    if (ErrorCode != B_OK)
      PathToMailDirectory.SetTo (".");

    DirectoryEntry.SetTo (PathToMailDirectory.Path(), true);
    if (DirectoryEntry.GetRef (&DirectoryEntryRef) != B_OK)
    {
      DisplayErrorMessage (
        "Unable to set up the file requestor starting directory.  Sorry.");
      return;
    }

    m_EstimateSpamFilePanelPntr = new BFilePanel (
      B_OPEN_PANEL /* mode */,
      &MessengerToSelf /* target for event messages */,
      &DirectoryEntryRef /* starting directory */,
      B_FILE_NODE,
      true /* true for multiple selections */,
      new BMessage (MSG_ESTIMATE_FILE_REFS) /* canned message */,
      NULL /* ref filter */,
      false /* true for modal */,
      true /* true to hide when done */);
  }

  if (m_EstimateSpamFilePanelPntr != NULL)
    m_EstimateSpamFilePanelPntr->Show (); /* Answer sent via a message. */
}


/* The display has been resized.  Have to manually adjust the popup menu bar to
show the new size (the sub-items need to be resized too).  Then make it redraw.
Well, actually just resetting the mark on the current item will resize it
properly. */

void ControlsView::FrameResized (float, float)
{
  m_ScoringModeCachedValue = SM_MAX; /* Force it to reset the mark. */
  m_TokenizeModeCachedValue = TM_MAX; /* Force it to reset the mark. */
}


void ControlsView::MessageReceived (BMessage *MessagePntr)
{
  BMessage CommandMessage;
  bool     TempBool;
  uint32   TempUint32;

  switch (MessagePntr->what)
  {
    case MSG_BROWSE_BUTTON:
      BrowseForDatabaseFile ();
      break;

    case MSG_DATABASE_NAME:
      if (strcmp (m_DatabaseFileNameCachedValue,
      m_DatabaseFileNameTextboxPntr->Text ()) != 0)
        SubmitCommandString (PN_DATABASE_FILE, B_SET_PROPERTY,
        m_DatabaseFileNameTextboxPntr->Text ());
      break;

    case MSG_ESTIMATE_BUTTON:
      BrowseForFileToEstimate ();
      break;

    case MSG_ESTIMATE_FILE_REFS:
      EstimateRefFilesAndDisplay (MessagePntr);
      break;

    case MSG_IGNORE_CLASSIFICATION:
      TempBool = (m_IgnorePreviousClassCheckboxPntr->Value() == B_CONTROL_ON);
      if (m_IgnorePreviousClassCachedValue != TempBool)
        SubmitCommandBool (PN_IGNORE_PREVIOUS_CLASSIFICATION,
        B_SET_PROPERTY, TempBool);
      break;

    case MSG_PURGE_AGE:
      TempUint32 = strtoul (m_PurgeAgeTextboxPntr->Text (), NULL, 10);
      if (m_PurgeAgeCachedValue != TempUint32)
        SubmitCommandInt32 (PN_PURGE_AGE, B_SET_PROPERTY, TempUint32);
      break;

    case MSG_PURGE_POPULARITY:
      TempUint32 = strtoul (m_PurgePopularityTextboxPntr->Text (), NULL, 10);
      if (m_PurgePopularityCachedValue != TempUint32)
        SubmitCommandInt32 (PN_PURGE_POPULARITY, B_SET_PROPERTY, TempUint32);
      break;

    case MSG_SERVER_MODE:
      TempBool = (m_ServerModeCheckboxPntr->Value() == B_CONTROL_ON);
      if (m_ServerModeCachedValue != TempBool)
        SubmitCommandBool (PN_SERVER_MODE, B_SET_PROPERTY, TempBool);
      break;

    default:
      BView::MessageReceived (MessagePntr);
  }
}


/* Check the server for changes in the state of the database, and if there are
any changes, update the displayed values.  Since this is a read only
examination of the server, we go directly to the application rather than
sending it messages.  Also, when sending messages, we can't find out what it is
doing while it is busy with a batch of spam additions (all the spam add
commands will be in the queue ahead of our requests for info).  Instead, we
lock the BApplication (so it isn't changing things while we're looking) and
retrieve our values. */

void ControlsView::PollServerForChanges ()
{
  ABSApp     *MyAppPntr;
  BMenuItem  *TempMenuItemPntr;
  char        TempString [PATH_MAX];
  BWindow    *WindowPntr;

  /* We need a pointer to our window, for changing the title etc. */

  WindowPntr = Window ();
  if (WindowPntr == NULL)
    return; /* No window, no point in updating the display! */

  /* Check the server mode flag.  If the mode is off, then the window has to be
  minimized.  Similarly, if it gets turned on, maximize the window.  Note that
  the user can maximize the window manually, even while still in server mode.
  */

  if (g_ServerMode != m_ServerModeCachedValue &&
  m_ServerModeCheckboxPntr != NULL)
  {
    m_ServerModeCachedValue = g_ServerMode;
    m_ServerModeCheckboxPntr->SetValue (
      m_ServerModeCachedValue ? B_CONTROL_ON : B_CONTROL_OFF);
    WindowPntr->Minimize (m_ServerModeCachedValue);
  }

  if (WindowPntr->IsMinimized ())
    return; /* Window isn't visible, don't waste time updating it. */

  /* So that people don't stare at a blank screen, request a database load if
  nothing is there.  But only do it once, so the user doesn't get a lot of
  invalid database messages if one doesn't exist yet.  In server mode, we never
  get this far so it is only loaded when the user wants to see something. */

  if (!m_DatabaseLoadDone)
  {
    m_DatabaseLoadDone = true;
    /* Counting the number of words will load the database. */
    SubmitCommandString (PN_DATABASE_FILE, B_COUNT_PROPERTIES, "");
  }

  /* Check various read only values, which can be read from the BApplication
  without having to lock it.  This is useful for displaying the number of words
  as it is changing.  First up is the purge age setting. */

  MyAppPntr = dynamic_cast<ABSApp *> (be_app);
  if (MyAppPntr == NULL)
    return; /* Doesn't exist or is the wrong class.  Not likely! */

  if (MyAppPntr->m_PurgeAge != m_PurgeAgeCachedValue &&
  m_PurgeAgeTextboxPntr != NULL)
  {
    m_PurgeAgeCachedValue = MyAppPntr->m_PurgeAge;
    sprintf (TempString, "%lu", m_PurgeAgeCachedValue);
    m_PurgeAgeTextboxPntr->SetText (TempString);
  }

  /* Check the purge popularity. */

  if (MyAppPntr->m_PurgePopularity != m_PurgePopularityCachedValue &&
  m_PurgePopularityTextboxPntr != NULL)
  {
    m_PurgePopularityCachedValue = MyAppPntr->m_PurgePopularity;
    sprintf (TempString, "%lu", m_PurgePopularityCachedValue);
    m_PurgePopularityTextboxPntr->SetText (TempString);
  }

  /* Check the Ignore Previous Classification flag. */

  if (MyAppPntr->m_IgnorePreviousClassification !=
  m_IgnorePreviousClassCachedValue &&
  m_IgnorePreviousClassCheckboxPntr != NULL)
  {
    m_IgnorePreviousClassCachedValue =
      MyAppPntr->m_IgnorePreviousClassification;
    m_IgnorePreviousClassCheckboxPntr->SetValue (
      m_IgnorePreviousClassCachedValue ? B_CONTROL_ON : B_CONTROL_OFF);
  }

  /* Update the genuine count. */

  if (MyAppPntr->m_TotalGenuineMessages != m_GenuineCountCachedValue &&
  m_GenuineCountTextboxPntr != NULL)
  {
    m_GenuineCountCachedValue = MyAppPntr->m_TotalGenuineMessages;
    sprintf (TempString, "%lu", m_GenuineCountCachedValue);
    m_GenuineCountTextboxPntr->SetText (TempString);
  }

  /* Update the spam count. */

  if (MyAppPntr->m_TotalSpamMessages != m_SpamCountCachedValue &&
  m_SpamCountTextboxPntr != NULL)
  {
    m_SpamCountCachedValue = MyAppPntr->m_TotalSpamMessages;
    sprintf (TempString, "%lu", m_SpamCountCachedValue);
    m_SpamCountTextboxPntr->SetText (TempString);
  }

  /* Update the word count. */

  if (MyAppPntr->m_WordCount != m_WordCountCachedValue &&
  m_WordCountTextboxPntr != NULL)
  {
    m_WordCountCachedValue = MyAppPntr->m_WordCount;
    sprintf (TempString, "%lu", m_WordCountCachedValue);
    m_WordCountTextboxPntr->SetText (TempString);
  }

  /* Update the tokenize mode pop-up menu. */

  if (MyAppPntr->m_TokenizeMode != m_TokenizeModeCachedValue &&
  m_TokenizeModePopUpMenuPntr != NULL)
  {
    m_TokenizeModeCachedValue = MyAppPntr->m_TokenizeMode;
    TempMenuItemPntr =
      m_TokenizeModePopUpMenuPntr->ItemAt ((int) m_TokenizeModeCachedValue);
    if (TempMenuItemPntr != NULL)
      TempMenuItemPntr->SetMarked (true);
  }

  /* Update the scoring mode pop-up menu. */

  if (MyAppPntr->m_ScoringMode != m_ScoringModeCachedValue &&
  m_ScoringModePopUpMenuPntr != NULL)
  {
    m_ScoringModeCachedValue = MyAppPntr->m_ScoringMode;
    TempMenuItemPntr =
      m_ScoringModePopUpMenuPntr->ItemAt ((int) m_ScoringModeCachedValue);
    if (TempMenuItemPntr != NULL)
      TempMenuItemPntr->SetMarked (true);
  }

  /* Lock the application.  This will stop it from processing any further
  messages until we are done.  Or if it is busy, the lock will fail. */

  if (MyAppPntr->LockWithTimeout (100000) != B_OK)
    return; /* It's probably busy doing something. */

  /* See if the database file name has changed. */

  if (strcmp (MyAppPntr->m_DatabaseFileName.String (),
  m_DatabaseFileNameCachedValue) != 0 &&
  m_DatabaseFileNameTextboxPntr != NULL)
  {
    strcpy (m_DatabaseFileNameCachedValue,
      MyAppPntr->m_DatabaseFileName.String ());
    m_DatabaseFileNameTextboxPntr->SetText (m_DatabaseFileNameCachedValue);
    WindowPntr->SetTitle (m_DatabaseFileNameCachedValue);
  }

  /* Done.  Let the BApplication continue processing messages. */

  MyAppPntr->Unlock ();
}


void ControlsView::Pulse ()
{
  if (system_time () > m_TimeOfLastPoll + 200000)
  {
    PollServerForChanges ();
    m_TimeOfLastPoll = system_time ();
  }
}



/******************************************************************************
 * Implementation of the DatabaseWindow class, constructor, destructor and the
 * rest of the member functions in mostly alphabetical order.
 */

DatabaseWindow::DatabaseWindow ()
: BWindow (BRect (30, 30, 620, 400),
    "Haiku spam filter server",
    B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
  BRect TempRect;

  /* Add the controls view. */

  m_ControlsViewPntr = new ControlsView (Bounds ());
  if (m_ControlsViewPntr == NULL)
    goto ErrorExit;
  AddChild (m_ControlsViewPntr);

  /* Add the word view in the remaining space under the controls view. */


  TempRect = Bounds ();
  TempRect.top = m_ControlsViewPntr->Frame().bottom + 1;
  m_WordsViewPntr = new WordsView (TempRect);
  if (m_WordsViewPntr == NULL)
    goto ErrorExit;
  AddChild (m_WordsViewPntr);
 
 /* Minimize the window if we are starting up in server mode.  This is done 	 
	before the window is open so it doesn't flash onto the screen, and possibly 	 
	steal a keystroke or two.  The ControlsView will further update the minimize 	 
	mode when it detects changes in the server mode. */ 
  Minimize (g_ServerMode);

  return;

ErrorExit:
  DisplayErrorMessage ("Unable to initialise the window contents.");
}


void DatabaseWindow::MessageReceived (BMessage *MessagePntr)
{
  if (MessagePntr->what == B_MOUSE_WHEEL_CHANGED)
  {
    /* Pass the mouse wheel stuff down to the words view, since that's the only
    one which does scrolling so we don't need to worry about whether it has
    focus or not. */

    if (m_WordsViewPntr != NULL)
      m_WordsViewPntr->MessageReceived (MessagePntr);
  }
  else
    BWindow::MessageReceived (MessagePntr);
}


bool DatabaseWindow::QuitRequested ()
{
  be_app->PostMessage (B_QUIT_REQUESTED);
  return true;
}



/******************************************************************************
 * Implementation of the word display view.
 */

WordsView::WordsView (BRect NewBounds)
: BView (NewBounds, "WordsView", B_FOLLOW_ALL_SIDES,
    B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE | B_PULSE_NEEDED),
  m_ArrowLineDownPntr (NULL),
  m_ArrowLineUpPntr (NULL),
  m_ArrowPageDownPntr (NULL),
  m_ArrowPageUpPntr (NULL),
  m_LastTimeAKeyWasPressed (0)
{
  font_height TempFontHeight;

  GetFont (&m_TextFont); /* Modify the default font to be our own. */
  m_TextFont.SetSize (ceilf (m_TextFont.Size() * 1.1));
  m_TextFont.GetHeight (&TempFontHeight);
  SetFont (&m_TextFont);

  m_LineHeight = ceilf (TempFontHeight.ascent +
    TempFontHeight.descent + TempFontHeight.leading);
  m_AscentHeight = ceilf (TempFontHeight.ascent);
  m_TextHeight = ceilf (TempFontHeight.ascent +
    TempFontHeight.descent);

  m_FocusedColour.red = 255;
  m_FocusedColour.green = 255;
  m_FocusedColour.blue = 255;
  m_FocusedColour.alpha = 255;

  m_UnfocusedColour.red = 245;
  m_UnfocusedColour.green = 245;
  m_UnfocusedColour.blue = 255;
  m_UnfocusedColour.alpha = 255;

  m_BackgroundColour = m_UnfocusedColour;
  SetViewColor (m_BackgroundColour);
  SetLowColor (m_BackgroundColour);
  SetHighColor (0, 0, 0);

  strcpy (m_FirstDisplayedWord, "a");
}


void WordsView::AttachedToWindow ()
{
  BPolygon        DownLinePolygon (g_DownLinePoints,
                    sizeof (g_DownLinePoints) /
                    sizeof (g_DownLinePoints[0]));

  BPolygon        DownPagePolygon (g_DownPagePoints,
                    sizeof (g_DownPagePoints) /
                    sizeof (g_DownPagePoints[0]));

  BPolygon        UpLinePolygon (g_UpLinePoints,
                    sizeof (g_UpLinePoints) /
                    sizeof (g_UpLinePoints[0]));

  BPolygon        UpPagePolygon (g_UpPagePoints,
                    sizeof (g_UpPagePoints) /
                    sizeof (g_UpPagePoints[0]));

  BPicture        TempOffPicture;
  BPicture        TempOnPicture;
  BRect           TempRect;

  /* Make the buttons and associated polygon images for the forward and
  backwards a word or a page of words buttons.  They're the width of the scroll
  bar area on the right, but twice as tall as usual, since there is no scroll
  bar and that will make it easier to use them.  First the up a line button. */

  SetHighColor (0, 0, 0);
  BeginPicture (&TempOffPicture);
  FillPolygon (&UpLinePolygon);
  SetHighColor (180, 180, 180);
  StrokePolygon (&UpLinePolygon);
  EndPicture ();

  SetHighColor (128, 128, 128);
  BeginPicture (&TempOnPicture);
  FillPolygon (&UpLinePolygon);
  EndPicture ();

  TempRect = Bounds ();
  TempRect.bottom = TempRect.top + 2 * B_H_SCROLL_BAR_HEIGHT;
  TempRect.left = TempRect.right - B_V_SCROLL_BAR_WIDTH;
  m_ArrowLineUpPntr = new BPictureButton (TempRect, "Up Line",
    &TempOffPicture, &TempOnPicture,
    new BMessage (MSG_LINE_UP), B_ONE_STATE_BUTTON,
    B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
  if (m_ArrowLineUpPntr == NULL) goto ErrorExit;
  AddChild (m_ArrowLineUpPntr);
  m_ArrowLineUpPntr->SetTarget (this);

  /* Up a page button. */

  SetHighColor (0, 0, 0);
  BeginPicture (&TempOffPicture);
  FillPolygon (&UpPagePolygon);
  SetHighColor (180, 180, 180);
  StrokePolygon (&UpPagePolygon);
  EndPicture ();

  SetHighColor (128, 128, 128);
  BeginPicture (&TempOnPicture);
  FillPolygon (&UpPagePolygon);
  EndPicture ();

  TempRect = Bounds ();
  TempRect.top += 2 * B_H_SCROLL_BAR_HEIGHT + 1;
  TempRect.bottom = TempRect.top + 2 * B_H_SCROLL_BAR_HEIGHT;
  TempRect.left = TempRect.right - B_V_SCROLL_BAR_WIDTH;
  m_ArrowPageUpPntr = new BPictureButton (TempRect, "Up Page",
    &TempOffPicture, &TempOnPicture,
    new BMessage (MSG_PAGE_UP), B_ONE_STATE_BUTTON,
    B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
  if (m_ArrowPageUpPntr == NULL) goto ErrorExit;
  AddChild (m_ArrowPageUpPntr);
  m_ArrowPageUpPntr->SetTarget (this);

  /* Down a page button. */

  SetHighColor (0, 0, 0);
  BeginPicture (&TempOffPicture);
  FillPolygon (&DownPagePolygon);
  SetHighColor (180, 180, 180);
  StrokePolygon (&DownPagePolygon);
  EndPicture ();

  SetHighColor (128, 128, 128);
  BeginPicture (&TempOnPicture);
  FillPolygon (&DownPagePolygon);
  EndPicture ();

  TempRect = Bounds ();
  TempRect.bottom -= 3 * B_H_SCROLL_BAR_HEIGHT + 1;
  TempRect.top = TempRect.bottom - 2 * B_H_SCROLL_BAR_HEIGHT;
  TempRect.left = TempRect.right - B_V_SCROLL_BAR_WIDTH;
  m_ArrowPageDownPntr = new BPictureButton (TempRect, "Down Page",
    &TempOffPicture, &TempOnPicture,
    new BMessage (MSG_PAGE_DOWN), B_ONE_STATE_BUTTON,
    B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
  if (m_ArrowPageDownPntr == NULL) goto ErrorExit;
  AddChild (m_ArrowPageDownPntr);
  m_ArrowPageDownPntr->SetTarget (this);

  /* Down a line button. */

  SetHighColor (0, 0, 0);
  BeginPicture (&TempOffPicture);
  FillPolygon (&DownLinePolygon);
  SetHighColor (180, 180, 180);
  StrokePolygon (&DownLinePolygon);
  EndPicture ();

  SetHighColor (128, 128, 128);
  BeginPicture (&TempOnPicture);
  FillPolygon (&DownLinePolygon);
  EndPicture ();

  TempRect = Bounds ();
  TempRect.bottom -= B_H_SCROLL_BAR_HEIGHT;
  TempRect.top = TempRect.bottom - 2 * B_H_SCROLL_BAR_HEIGHT;
  TempRect.left = TempRect.right - B_V_SCROLL_BAR_WIDTH;
  m_ArrowLineDownPntr = new BPictureButton (TempRect, "Down Line",
    &TempOffPicture, &TempOnPicture,
    new BMessage (MSG_LINE_DOWN), B_ONE_STATE_BUTTON,
    B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
  if (m_ArrowLineDownPntr == NULL) goto ErrorExit;
  AddChild (m_ArrowLineDownPntr);
  m_ArrowLineDownPntr->SetTarget (this);

  return;

ErrorExit:
  DisplayErrorMessage ("Problems while making view displaying the words.");
}


/* Draw the words starting with the one at or after m_FirstDisplayedWord.  This
requires looking at the database in the BApplication, which may or may not be
available (if it isn't, don't draw, a redraw will usually be requested by the
Pulse member function when it keeps on noticing that the stuff on the display
doesn't match the database). */

void WordsView::Draw (BRect UpdateRect)
{
  float                   AgeDifference;
  float                   AgeProportion;
  float                   CenterX;
  float                   ColumnLeftCenterX;
  float                   ColumnMiddleCenterX;
  float                   ColumnRightCenterX;
  float                   CompensatedRatio;
  StatisticsMap::iterator DataIter;
  StatisticsMap::iterator EndIter;
  rgb_color               FillColour;
  float                   GenuineProportion;
  uint32                  GenuineSpamSum;
  float                   HeightPixels;
  float                   HeightProportion;
  float                   LeftBounds;
  ABSApp                 *MyAppPntr;
  uint32                  NewestAge;
  uint32                  OldestAge;
  float                   OneFifthTotalGenuine;
  float                   OneFifthTotalSpam;
  double                  RawProbabilityRatio;
  float                   RightBounds;
  float                   SpamProportion;
  StatisticsPointer       StatisticsPntr;
  BRect                   TempRect;
  char                    TempString [PATH_MAX];
  float                   TotalGenuineMessages = 1.0; /* Avoid divide by 0. */
  float                   TotalSpamMessages = 1.0;
  float                   Width;
  float                   Y;

  /* Lock the application.  This will stop it from processing any further
  messages until we are done.  Or if it is busy, the lock will fail. */

  MyAppPntr = dynamic_cast<ABSApp *> (be_app);
  if (MyAppPntr == NULL || MyAppPntr->LockWithTimeout (100000) != B_OK)
    return; /* It's probably busy doing something. */

  /* Set up various loop invariant variables. */

  if (MyAppPntr->m_TotalGenuineMessages > 0)
    TotalGenuineMessages = MyAppPntr->m_TotalGenuineMessages;
  OneFifthTotalGenuine = TotalGenuineMessages / 5;

  if (MyAppPntr->m_TotalSpamMessages > 0)
    TotalSpamMessages = MyAppPntr->m_TotalSpamMessages;
  OneFifthTotalSpam = TotalSpamMessages / 5;

  EndIter = MyAppPntr->m_WordMap.end ();

  OldestAge = MyAppPntr->m_OldestAge;
  NewestAge = /* actually newest age plus one */
    MyAppPntr->m_TotalGenuineMessages + MyAppPntr->m_TotalSpamMessages;

  if (NewestAge == 0)
    goto NormalExit; /* No words to display, or something is badly wrong. */

  NewestAge--; /* The newest message has age NewestAge. */
  AgeDifference = NewestAge - OldestAge; /* Can be zero if just one message. */

  LeftBounds = Bounds().left;
  RightBounds = Bounds().right - B_V_SCROLL_BAR_WIDTH;
  Width = RightBounds - LeftBounds;
  FillColour.alpha = 255;

  CenterX = ceilf (LeftBounds + Width * 0.5);
  ColumnLeftCenterX = ceilf (LeftBounds + Width * 0.05);
  ColumnMiddleCenterX = CenterX;
  ColumnRightCenterX = ceilf (LeftBounds + Width * 0.95);

  for (DataIter = MyAppPntr->m_WordMap.lower_bound (m_FirstDisplayedWord),
  Y = Bounds().top;
  DataIter != EndIter && Y < UpdateRect.bottom;
  DataIter++, Y += m_LineHeight)
  {
    if (Y + m_LineHeight < UpdateRect.top)
      continue; /* Not in the visible area yet, don't actually draw. */

    /* Draw the colour bar behind the word.  It reflects the spamness or
    genuineness of that particular word, plus the importance of the word and
    the age of the word.

    First calculate the compensated spam ratio (described elsewhere).  It is
    close to 0.0 for genuine words and close to 1.0 for pure spam.  It is drawn
    as a blue bar to the left of center if it is less than 0.5, and a red bar
    on the right of center if it is greater than 0.5.  At exactly 0.5 nothing
    is drawn; the word is worthless as an indicator.

    The height of the bar corresponds to the number of messages the word was
    found in.  Make the height proportional to the total of spam and genuine
    messages for the word divided by the sum of the most extreme spam and
    genuine counts in the database.

    The staturation of the colour corresponds to the age of the word, with old
    words being almost white rather than solid blue or red. */

    StatisticsPntr = &DataIter->second;

    SpamProportion = StatisticsPntr->spamCount / TotalSpamMessages;
    GenuineProportion = StatisticsPntr->genuineCount / TotalGenuineMessages;
    if (SpamProportion + GenuineProportion > 0.0f)
      RawProbabilityRatio =
      SpamProportion / (SpamProportion + GenuineProportion);
    else
      RawProbabilityRatio = g_RobinsonX;

    /* The compensated ratio leans towards 0.5 (RobinsonX) more for fewer
    data points, with a weight of 0.45 (RobinsonS). */

    GenuineSpamSum =
      StatisticsPntr->spamCount + StatisticsPntr->genuineCount;
    CompensatedRatio =
      (g_RobinsonS * g_RobinsonX + GenuineSpamSum * RawProbabilityRatio) /
      (g_RobinsonS + GenuineSpamSum);

    /* Used to use the height based on the most frequent word, but some words,
    like "From", show up in all messages which made most other words just
    appear as a thin line.  I did a histogram plot of the sizes in my test
    database, and figured that you get better coverage of 90% of the messages
    if you use 1/5 of the total number as the count which gives you 100%
    height.  The other 10% get a full height bar, but most people wouldn't care
    that they're super frequently used. */

    HeightProportion = 0.5f * (StatisticsPntr->genuineCount /
      OneFifthTotalGenuine + StatisticsPntr->spamCount / OneFifthTotalSpam);

    if (HeightProportion > 1.0f)
      HeightProportion = 1.0f;
    HeightPixels = ceilf (HeightProportion * m_TextHeight);

    if (AgeDifference <= 0.0f)
      AgeProportion = 1.0; /* New is 1.0, old is 0.0 */
    else
      AgeProportion = (StatisticsPntr->age - OldestAge) / AgeDifference;

    TempRect.top = ceilf (Y + m_TextHeight / 2 - HeightPixels / 2);
    TempRect.bottom = TempRect.top + HeightPixels;

    if (CompensatedRatio < 0.5f)
    {
      TempRect.left = ceilf (
        CenterX - 1.6f * (0.5f - CompensatedRatio) * (CenterX - LeftBounds));
      TempRect.right = CenterX;
      FillColour.red = 230 - (int) (AgeProportion * 230.0f);
      FillColour.green = FillColour.red;
      FillColour.blue = 255;
    }
    else /* Ratio >= 0.5, red spam block. */
    {
      TempRect.left = CenterX;
      TempRect.right = ceilf (
        CenterX + 1.6f * (CompensatedRatio - 0.5f) * (RightBounds - CenterX));
      FillColour.blue = 230 - (int) (AgeProportion * 230.0f);
      FillColour.green = FillColour.blue;
      FillColour.red = 255;
    }
    SetHighColor (FillColour);
    SetDrawingMode (B_OP_COPY);
    FillRect (TempRect);

    /* Print the text centered in columns of various widths.  The number of
    genuine messages in the left 10% of the width, the word in the middle 80%,
    and the number of spam messages using the word in the right 10%. */

    SetHighColor (0, 0, 0);
    SetDrawingMode (B_OP_OVER); /* So that antialiased text mixes better. */

    sprintf (TempString, "%lu", StatisticsPntr->genuineCount);
    Width = m_TextFont.StringWidth (TempString);
    MovePenTo (ceilf (ColumnLeftCenterX - Width / 2), Y + m_AscentHeight);
    DrawString (TempString);

    strcpy (TempString, DataIter->first.c_str ());
    Width = m_TextFont.StringWidth (TempString);
    MovePenTo (ceilf (ColumnMiddleCenterX - Width / 2), Y + m_AscentHeight);
    DrawString (TempString);

    sprintf (TempString, "%lu", StatisticsPntr->spamCount);
    Width = m_TextFont.StringWidth (TempString);
    MovePenTo (ceilf (ColumnRightCenterX - Width / 2), Y + m_AscentHeight);
    DrawString (TempString);
  }

  /* Draw the first word (the one which the user types in to select the first
  displayed word) on the right, in the scroll bar margin, rotated 90 degrees to
  fit between the page up and page down buttons. */

  Width = m_TextFont.StringWidth (m_FirstDisplayedWord);
  if (Width > 0)
  {
    TempRect = Bounds ();
    TempRect.top += 4 * B_H_SCROLL_BAR_HEIGHT + 1;
    TempRect.bottom -= 5 * B_H_SCROLL_BAR_HEIGHT + 1;

    MovePenTo (TempRect.right - m_TextHeight + m_AscentHeight - 1,
      ceilf ((TempRect.bottom + TempRect.top) / 2 + Width / 2));
    m_TextFont.SetRotation (90);
    SetFont (&m_TextFont, B_FONT_ROTATION);
    DrawString (m_FirstDisplayedWord);
    m_TextFont.SetRotation (0);
    SetFont (&m_TextFont, B_FONT_ROTATION);
  }

NormalExit:

  /* Successfully finished drawing.  Update the cached values to match what we
  have drawn. */
  m_CachedTotalGenuineMessages = MyAppPntr->m_TotalGenuineMessages;
  m_CachedTotalSpamMessages = MyAppPntr->m_TotalSpamMessages;
  m_CachedWordCount = MyAppPntr->m_WordCount;

  /* Done.  Let the BApplication continue processing messages. */
  MyAppPntr->Unlock ();
}


/* When the user presses keys, they select the first word to be displayed in
the view (it's the word at or lexicographically after the word typed in).  The
keys are appended to the starting word, until the user stops typing for a
while, then the next key will be the first letter of a new starting word. */

void WordsView::KeyDown (const char *BufferPntr, int32 NumBytes)
{
  int32          CharLength;
  bigtime_t      CurrentTime;
  char           TempString [40];

  CurrentTime = system_time ();

  if (NumBytes < (int32) sizeof (TempString))
  {
    memcpy (TempString, BufferPntr, NumBytes);
    TempString [NumBytes] = 0;
    CharLength = strlen (TempString); /* So NUL bytes don't get through. */

    /* Check for arrow keys, which move the view up and down. */

    if (CharLength == 1 &&
    (TempString[0] == B_UP_ARROW ||
    TempString[0] == B_DOWN_ARROW ||
    TempString[0] == B_PAGE_UP ||
    TempString[0] == B_PAGE_DOWN))
    {
      MoveTextUpOrDown ((TempString[0] == B_UP_ARROW) ? MSG_LINE_UP :
        ((TempString[0] == B_DOWN_ARROW) ? MSG_LINE_DOWN :
        ((TempString[0] == B_PAGE_UP) ? MSG_PAGE_UP : MSG_PAGE_DOWN)));
    }
    else if (CharLength > 1 ||
    (CharLength == 1 && 32 <= (uint8) TempString[0]))
    {
      /* Have a non-control character, or some sort of multibyte char.  Add it
      to the word and mark things for redisplay starting at the resulting word.
      */

      if (CurrentTime - m_LastTimeAKeyWasPressed >= 1000000 /* microseconds */)
        strcpy (m_FirstDisplayedWord, TempString); /* Starting a new word. */
      else if (strlen (m_FirstDisplayedWord) + CharLength <= g_MaxWordLength)
        strcat (m_FirstDisplayedWord, TempString); /* Append to existing. */

      Invalidate ();
    }
  }

  m_LastTimeAKeyWasPressed = CurrentTime;
  BView::KeyDown (BufferPntr, NumBytes);
}


/* Change the background colour to show that we have the focus.  When we have
it, keystrokes will select the word to be displayed at the top of the list. */

void WordsView::MakeFocus (bool Focused)
{
  if (Focused)
    m_BackgroundColour = m_FocusedColour;
  else
    m_BackgroundColour = m_UnfocusedColour;
  SetViewColor (m_BackgroundColour);
  SetLowColor (m_BackgroundColour);

  /* Also need to set the background colour for the scroll buttons, since they
  can't be made transparent. */

  if (m_ArrowLineDownPntr != NULL)
  {
    m_ArrowLineDownPntr->SetViewColor (m_BackgroundColour);
    m_ArrowLineDownPntr->Invalidate ();
  }

  if (m_ArrowLineUpPntr != NULL)
  {
    m_ArrowLineUpPntr->SetViewColor (m_BackgroundColour);
    m_ArrowLineUpPntr->Invalidate ();
  }

  if (m_ArrowPageDownPntr != NULL)
  {
    m_ArrowPageDownPntr->SetViewColor (m_BackgroundColour);
    m_ArrowPageDownPntr->Invalidate ();
  }

  if (m_ArrowPageUpPntr != NULL)
  {
    m_ArrowPageUpPntr->SetViewColor (m_BackgroundColour);
    m_ArrowPageUpPntr->Invalidate ();
  }

  Invalidate ();

  BView::MakeFocus (Focused);
}


void WordsView::MessageReceived (BMessage *MessagePntr)
{
  int32     CountFound;
  float     DeltaY; /* Usually -1.0, 0.0 or +1.0. */
  type_code TypeFound;

  switch (MessagePntr->what)
  {
    case B_MOUSE_WHEEL_CHANGED:
      if (MessagePntr->FindFloat ("be:wheel_delta_y", &DeltaY) != 0) break;
      if (DeltaY < 0)
        MoveTextUpOrDown (MSG_LINE_UP);
      else if (DeltaY > 0)
        MoveTextUpOrDown (MSG_LINE_DOWN);
      break;

    case MSG_LINE_DOWN:
    case MSG_LINE_UP:
    case MSG_PAGE_DOWN:
    case MSG_PAGE_UP:
      MoveTextUpOrDown (MessagePntr->what);
      break;

    case B_SIMPLE_DATA: /* Something has been dropped in our view. */
      if (MessagePntr->GetInfo ("refs", &TypeFound, &CountFound) == B_OK &&
      CountFound > 0 && TypeFound == B_REF_TYPE)
      {
        RefsDroppedHere (MessagePntr);
        break;
      }
      /* Else fall through to the default case, in case it is something else
      dropped that the system knows about. */

    default:
      BView::MessageReceived (MessagePntr);
  }
}


/* If the user clicks on our view, take over the focus. */

void WordsView::MouseDown (BPoint)
{
  if (!IsFocus ())
    MakeFocus (true);
}


void WordsView::MoveTextUpOrDown (uint32 MovementType)
{
  StatisticsMap::iterator  DataIter;
  int                      i;
  ABSApp                  *MyAppPntr;
  int                      PageSize;

  /* Lock the application.  This will stop it from processing any further
  messages until we are done (we need to look at the word list directly).  Or
  if it is busy, the lock will fail. */

  MyAppPntr = dynamic_cast<ABSApp *> (be_app);
  if (MyAppPntr == NULL || MyAppPntr->LockWithTimeout (2000000) != B_OK)
    return; /* It's probably busy doing something. */

  PageSize = (int) (Bounds().Height() / m_LineHeight - 1);
  if (PageSize < 1)
    PageSize = 1;

  DataIter = MyAppPntr->m_WordMap.lower_bound (m_FirstDisplayedWord);

  switch (MovementType)
  {
    case MSG_LINE_UP:
      if (DataIter != MyAppPntr->m_WordMap.begin ())
        DataIter--;
      break;

    case MSG_LINE_DOWN:
      if (DataIter != MyAppPntr->m_WordMap.end ())
        DataIter++;
      break;

    case MSG_PAGE_UP:
      for (i = 0; i < PageSize; i++)
      {
        if (DataIter == MyAppPntr->m_WordMap.begin ())
          break;
        DataIter--;
      }
      break;

    case MSG_PAGE_DOWN:
      for (i = 0; i < PageSize; i++)
      {
        if (DataIter == MyAppPntr->m_WordMap.end ())
          break;
        DataIter++;
      }
      break;
  }

  if (DataIter != MyAppPntr->m_WordMap.end ())
    strcpy (m_FirstDisplayedWord, DataIter->first.c_str ());

  Invalidate ();

  MyAppPntr->Unlock ();
}


/* This function periodically polls the BApplication to see if anything has
changed.  If the word list is different or the display has changed in some
other way, it will then try to refresh the display, repeating the attempt until
it gets successfully drawn. */

void WordsView::Pulse ()
{
  ABSApp *MyAppPntr;

  /* Probe the BApplication to see if it has changed. */

  MyAppPntr = dynamic_cast<ABSApp *> (be_app);
  if (MyAppPntr == NULL)
    return; /* Something is wrong, give up. */

  if (MyAppPntr->m_TotalGenuineMessages != m_CachedTotalGenuineMessages ||
  MyAppPntr->m_TotalSpamMessages != m_CachedTotalSpamMessages ||
  MyAppPntr->m_WordCount != m_CachedWordCount)
    Invalidate ();
}


/* The user has dragged and dropped some file references on the words view.  If
it is in the left third, add the file(s) as examples of genuine messages, right
third for spam messages and if it is in the middle third then evaluate the
file(s) for spaminess. */

void WordsView::RefsDroppedHere (BMessage *MessagePntr)
{
  float  Left;
  bool   SpamExample = true; /* TRUE if example is of spam, FALSE genuine. */
  float  Third;
  BPoint WhereDropped;

  /* Find out which third of the view it was dropped into. */

  if (MessagePntr->FindPoint ("_drop_point_", &WhereDropped) != B_OK)
    return;  /* Need to know where it was dropped. */
  ConvertFromScreen (&WhereDropped);
  Third = Bounds().Width() / 3;
  Left = Bounds().left;
  if (WhereDropped.x < Left + Third)
    SpamExample = false;
  else if (WhereDropped.x < Left + 2 * Third)
  {
    /* In the middle third, evaluate all files for spaminess. */
    EstimateRefFilesAndDisplay (MessagePntr);
    return;
  }

  if (g_CommanderLooperPntr != NULL)
    g_CommanderLooperPntr->CommandReferences (
    MessagePntr, true /* BulkMode */, SpamExample ? CL_SPAM : CL_GENUINE);
}



/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char**)
{
  g_CommandLineMode = (argc > 1);
  if (!g_CommandLineMode)
    cout << PrintUsage; /* In case no arguments specified. */

  g_CommanderLooperPntr = new CommanderLooper;
  if (g_CommanderLooperPntr != NULL)
  {
    g_CommanderMessenger = new BMessenger (NULL, g_CommanderLooperPntr);
    g_CommanderLooperPntr->Run ();
  }

  ABSApp MyApp;

  if (MyApp.InitCheck () == 0)
  {
    MyApp.LoadSaveSettings (true /* DoLoad */);
    MyApp.Run ();
  }

  if (g_CommanderLooperPntr != NULL)
  {
    g_CommanderLooperPntr->PostMessage (B_QUIT_REQUESTED);
    snooze (100000); /* Let the CommanderLooper thread run so it quits. */
  }

  cerr << "SpamDBM shutting down..." << endl;
  return 0; /* And implicitly destroys MyApp, which writes out the database. */
}
