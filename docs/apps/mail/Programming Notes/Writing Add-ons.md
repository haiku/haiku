# About this Document (Discouragement for Cheaters)

You need to read the whole thing. If you are writing a protocol, you have
to understand filters, chains, callbacks, the works. Filter authors can skip
the "Writing a Protocol" section, I suppose, but other than that, read it all.
Trust me, it's worth it. Also, if something doesn't make sense, or at any point
something becomes difficult that you feel probably shouldn't be, drop us a
line at zoidberg@bug-br.org.br – we're glad to help.

## General Architecture - Introduction to Chains

When you create an account in E-mail preferences, it creates two chains:
one inbound, one outbound. Each chain consists of a number of stored references
to mail filters, the generic type of Mail Daemon add-on, and their settings,
in the form of a flattened `BMessage`. The chain also stores global chain meta
data, also in a flattened `BMessage`, and various auxiliary information like
the chain name, and whether it is an outbound or inbound chain. The
distinction is important only to (1) set the color of the status bar when the
chain is run and (2) identify to the daemon which chains to run when it is
asked to fetch or to send mail. Each chain is identified by a unique unsigned
32-bit integer, the chain ID.

## Introduction to Filters

Every MDR add-on is conceptually a filter, and, programmatically, derived
from the `Mail::Filter` class (which is to be found in `MailAddon.h`). An MDR
filter is not the standard sort of e-mail filter (a sorter, etc.), but is
defined to be any sort of entity that modifies, parses, or otherwise cares
about an e-mail message in the process of it being sent (or received). Into
this very broad definition, it is possible to fit all the add-ons it is
possible (for us, at any rate, with our inadequate minds) to imagine:
protocols, standard e-mail filters, notification windows, message saving,
etc. In fact, the vast, vast majority of what happens when a message is
transferred happens not in the daemon itself, but in one of its many add-ons.

## Introduction to ChainRunner and Callbacks

The `Mail::ChainRunner` class exists, as the name would imply, to run chains.
It is of great use to you, the MDR add-on author. It publishes a variety of
useful public routines (like `ShowError()`) that will be described in their
appropriate sections, and does a number of other things that will also be
described later. But it does do one thing that is of general importance and
interest, and as near in importance for you to understand as filters:
callbacks.

Callbacks are called at the completion (successful or otherwise) of some
aspect of chain execution.

At the moment of completion, the callback's Callback() routine is called
with the error code that completed whatever it is the callback was waiting
for (`B_OK` or one of the `B_MAIL_*` family in MailAddon.h generally indicate
successful completion), and the callback is then destroyed. Callbacks come
in three kinds: message, process, and chain, and are registered by the
Register*Callback() routines of ChainRunner. These types of callbacks are
called, respectively, at the termination of a message transfer, a block of
message transfers (e.g. after all new messages are fetched off the server),
or the chain (just before all the add-ons are to be destroyed). These are
useful for a whole variety of tasks, and are used, for example, in such
things as deleting messages after they are fetched in POP3.

## How to Write a Filter

The `Mail::Filter` class has two important hooks (actually, it only has two
hooks, but they are quite important one): `InitCheck()` and `ProcessMailMessage()`.
`InitCheck()` corresponds to the standard Be API `InitCheck()` function:
after construction of your filter (which, for things like protocols,
may involve complicated things like connecting to a server), `InitCheck()`
is called. If something is wrong (say, you couldn't connect to the server),
return an appropriate error code, and, if out_message does not equal `NULL`,
set it to an appropriate human readable error message. If it does equal
`NULL`, it is suggested that you call ChainRunner's `ShowError()` routine
(see Error Reporting for more information). If `InitCheck()` returns an error,
construction of the chain stops, all filters are deleted, and `ChainRunner`
packs up and goes home.

After successful construction of all the filters in the chain,
`ProcessMailMessage()` is called for each message that passes through it.
It takes what looks, at first glance, like a bewildering array of arguments,
but they generally make sense and most filter applications don't need to use
them all anyway.

 * `BPositionIO** io_message`: This is where the message is to be written to
   (or read from). Astute observers will note that it is a pointer to a
   pointer, and will question either our sanity or my typing, depending
   on their frames of mind and personalities, among other things. But
   this aspect allows you to modify the argument in unexpected (and,
   naturally, very useful) ways. IMAP and POP3 replace the argument with
   their own reader that retrieves data from the server as it is requested.
   The Outbox filter swaps the argument for a BFile pointing to the message
   to be fetched. Note that if you do swap it, you become responsible for
   the deletion of the old argument. If you don't, there will be memory
   leaks and other untold havoc. (Further information on replacing
   `io_message` is available under How to Write a Protocol)

 * `BEntry *io_entry`: This tells you where, on disk, the contents of the
   message are kept. Useful for debugging purposes and for moving it about,
   although this last is not reccomended. For information on why not, see
   `io_headers` and `io_folder` (the next two, for the lazy).

 * `BMessage *io_headers`: This contains a list of various kinds of random
   junk in addition to a list of the headers of the message (after it's been
   through the Parser filter, which means it's blank for protocols, and full
   of yummy data for everyone else). The headers are stored as strings, with
   the key the header tag in whatever case it was in the message header (the
   subject, for instance, can be found with `FindString("Subject")`). If you
   modify these entries, they are written to disk in whatever form you leave
   them. In addition, there are several MDR-added entries (the previously
   mentioned "random junk"). These are the THREAD, NAME, SIZE, and DESTINATION
   fields. THREAD is the message thread (the subject with, Re:, Fwd:, etc.
   removed), NAME is the name of the sender (as displayed in Tracker in the
   "Name" attribute), SIZE (stored as a size_t) is the complete message size
   (in bytes), and DESTINATION, which may or may not have been added, is an
   override value for where, on disk, the message should be stored. You can
   add this to have the message be placed somewhere other than the user's
   defined inbox.

 * `BPath *io_folder`: This defines the subfolder of the user's inbox to
   which the message will be added, expressed relative to the inbox. IMAP
   uses it for the folder structure (it sets it to the name of the IMAP folder),
   and POP3 leaves it blank. If left blank, it will not be placed in a
   subfolder.

 * `const char *io_uid`: This is the unique id of the message, in some form
   that makes sense to the protocol. Usually of no concern to any filter.

After processing the message, `ProcessMailMessage()` returns either `B_OK`,
a descriptive error code, or one of the constants at the top of `MailAddon.h`
(`B_MAIL_DISCARD`, `B_MAIL_END_FETCH`, or `B_MAIL_END_CHAIN`). `B_OK` causes
the message to continue down the chain, `B_MAIL_DISCARD` causes it to be
deleted from disk and from the server and terminates the processing of the
message, error codes terminates the processing of the message as well,
`B_MAIL_END_FETCH` terminates the fetching of all remaining messages in this
fetch block, and `B_MAIL_END_CHAIN` indicates a catastrophic error has
occurred that requires the chain to be destroyed and the connection closed.

## Instantiating and Configuring the Filter

MDR uses three symbols in a filter, two of which are optional. They are
described below:

`instantiate_mailfilter`: This is called to instantiate a new copy of your
filter. It is passed a copy of the filter's settings and a pointer to the
calling `ChainRunner`.

`instantiate_config_panel`: This is passed a copy of your filter's settings
and the chain meta data. From it, you should return a BView with configuration
options. E-mail prefs will call `ResizeToPreferred()` on it after it is
instantiated. To save, the prefs app will call `Archive()`. The passed
`BMessage *` becomes your settings.

`descriptive_name`: This is passed the settings of the filter, and a
`char * buffer`. If this routine returns `B_OK`, the contents of the buffer
will replace the name of the add-on in E-mail prefs.

## How to Write a Protocol

While it is possible to write a protocol using nothing but the
`Mail::Filter` hooks, this is the Bad Way™ to do it. Instead of forcing you
through that, we've created the spectacularly useful `Mail::Protocol` class
(found, unsurprisingly, in `MailProtocol.h`). `Mail::Protocol` has two hooks,
`GetMessage()` and `DeleteMessage()`, a few member items, and a number of
important conventions. The MDR side of a mail protocol is fairly simple and
easy to understand; the network side of things may not be, and the best we can
do there is wish you luck. But you (hopefully) won't be cursing MDR.

### Part I: Starting the Connection (or, what to do in your constructor)

When your protocol is instantiated by `instantiate_mailfilter()`, you are
expected to initiate the connection. Information on this is contained in your
settings in a standard format, and can be written to your settings in that
format by `Mail::ProtocolConfigView`. The existance of this class makes your
life easy (you can return one from instantiate_config_panel and not worry
about configuration any further). The format is described at the end of this
section. After successfully establishing the connection, you are expected to
add the unique ids of every message on the server to the protected data member
unique_ids. This is a StringList, a special class we've created just for MDR.
It uses simple operators like `+=`, and shouldn't require too much work to
understand. The header is StringList.h, in the support subdirectory.
After adding all the unique ids, you need to tell ChainRunner to get the new
messages. You do this as follows:

```
StringList to_dl;
manifest->NotHere(*unique_ids, &to_dl);
runner->GetMessages(&to_dl, maildrop_size);
```

where maildrop_size is the combined total length (in bytes) of all the
messages on the server. If you don't know this, or determining it would be
complicated, slow, awkward, or just plain annoying, you can pass `-1`, in
which case the status bar will advance by message count instead of transferred
bytes.

### Part II: Protocol Settings Format

```
server (string): The IP address or hostname of the server

port (int32): The port on the server to connect to, if the user has specified one

flavor (int32): The 0-based index of the protocol flavor the user has chosen. If you didn't give the
		user a choice of flavors in ProtocolConfigView, you can ignore this with impunity.

username (string): The user name entered in config.

password & cpasswd (string): These give you the password, which may or may not have been stored
		encrypted. Use this code to get the password in plain text (stored in the password variable):

	const char *password = settings->FindString("password");
	char *passwd = get_passwd(settings, "cpasswd");
	if (passwd)
		password = passwd;

auth_method (int32): The 0-based index of the authentication method the user has chosen. If you
		didn't give the user a choice of methods in ProtocolConfigView, you can ignore this with
		impunity.
```

### Part III: Fetching Messages (or, what to do in GetMessage())

In your protocol's `GetMessage()` routine, you fetch the message indicated by
uid, into out_file. If your protocol is of the type that has multiple folders,
you can indicate that to future filters by setting out_folder_location to the
name of the folder in which the message is found. That's all you need to do.

Things get more complicated (you knew they would) if you want to support
partial message downloading. To do this, you need to replace out_file with
some sort of `BPositionIO` derivative that reads the message as required. Every
byte read from your `BPositionIO` derivative must also be written in the on-disk
representation of the message, that is, the old out_file argument. Second,
when your sub-class is deleted, you must delete the old out_file. Third, when
anyone does a Seek() operation referenced from SEEK_END, you must download the
whole message. You also need to add to out_headers an int32 named SIZE
containing the size, in bytes, of the complete message.

### Part IV: Deleting Messages

This is really simple. When `DeleteMessage()` is called, you delete the
message indicated by uid. You also need to modify the unique_ids list. To do
this, just do `(*unique_ids) -= uid;`.

### Part V: The Rest of It

As far as MDR is concerned, there is no rest of it. Everything else on the
BeOS side of things is taken care of by Mail::Protocol. Then there's the
network.... we'll leave you to that, and bother you no further, except to
ask you to read the next two sections:

### RemoteStorageProtocol

For IMAP-like protocols (that is, remotely stored mail systems with multiple
mailboxes), we provide you with the RemoteStorageProtocol class. It handles
most everything on the BeOS side. You need simply to implement RSP's six hook
functions: GetMessage(), AddMessage(), DeleteMessage(), CopyMessage(),
CreateMailbox(), and DeleteMailbox(). These should be fairly self-explanatory.
A couple of notes are in order, however.

First, unique ids MUST NOT contain a '/' character. If they do, everything
will go to hell.

Second, when you fill the unique_ids structure, use the following format:
`mailbox/id`. Mailbox names can contain / characters, and foo/bar will be
interpreted as a nested directory.

Second, you needn't remove anything from unique_ids in DeleteMessage().
That's handled for you.

Third, hooks like CopyMessage() and AddMessage() are passed the unique id
in a BString pointer. Fill this with the unique id the copy/uploaded message
receives once it's on the server.

### Progress Reporting

When your protocol receives a message, or a part of it, it is important
(from the user's point of view) that you display that fact. ChainRunner
provides a very simple way to do this, in its ReportProgress() function.
ReportProgress() takes three arguments: the number of bytes received since the
last call, the number of messages received since the last call, and an update
message. If you just want to inform the user of something happening
("Logging in", for instance), you can leave the bytes and messages argument
blank.

### Error Reporting

To report an error to the user, you need to call ChainRunner's `ShowError()`
method with some human-readable string describing the error condition. In
addition, you should take whatever action is necessary to report the error to
MDR in machine-understandable form, such as returning an error code, or calling
ChainRunner's Stop() method. Note that Stop() adds a message to the end of the
queue – you need to return a fatal error from ProcessMailMessage() to interrupt
a mail fetch in progress.
