#!/usr/bin/perl -w

# Part of this code was borrowed from Richard Jones's Net::FTPServer
# http://www.annexia.org/freeware/netftpserver

package FTPServer;

use strict;

use Cwd;
use Socket;
use IO::Socket::INET;
use IO::Seekable;
use POSIX qw(strftime);

my $log        = undef;
my $GOT_SIGURG = 0;

# CONSTANTS

# connection states
my %_connection_states = (
    'NEWCONN'  => 0x01, 
    'WAIT4PWD' => 0x02, 
    'LOGGEDIN' => 0x04,
    'TWOSOCKS' => 0x08,
);

# subset of FTP commands supported by these server and the respective
# connection states in which they are allowed
my %_commands = ( 
    # Standard commands from RFC 959.
    'CWD'  => $_connection_states{LOGGEDIN} |
              $_connection_states{TWOSOCKS}, 
#   'EPRT' => $_connection_states{LOGGEDIN},
#   'EPSV' => $_connection_states{LOGGEDIN}, 
    'LIST' => $_connection_states{TWOSOCKS}, 
#   'LPRT' => $_connection_states{LOGGEDIN},
#   'LPSV' => $_connection_states{LOGGEDIN}, 
    'PASS' => $_connection_states{WAIT4PWD}, 
    'PASV' => $_connection_states{LOGGEDIN}, 
    'PORT' => $_connection_states{LOGGEDIN}, 
    'PWD'  => $_connection_states{LOGGEDIN} |
              $_connection_states{TWOSOCKS}, 
    'QUIT' => $_connection_states{LOGGEDIN} |
              $_connection_states{TWOSOCKS}, 
    'REST' => $_connection_states{TWOSOCKS}, 
    'RETR' => $_connection_states{TWOSOCKS}, 
    'SYST' => $_connection_states{LOGGEDIN},
    'TYPE' => $_connection_states{LOGGEDIN} |
              $_connection_states{TWOSOCKS},
    'USER' => $_connection_states{NEWCONN}, 
    # From ftpexts Internet Draft.
    'SIZE' => $_connection_states{LOGGEDIN} |
              $_connection_states{TWOSOCKS},
);



# COMMAND-HANDLING ROUTINES

sub _CWD_command
{
    my ($conn, $cmd, $path) = @_;

    local $_;
    my $newdir = $conn->{dir};

    # If the path starts with a "/" then it's an absolute path.
    if (substr ($path, 0, 1) eq "/") {
        $newdir = "";
        $path =~ s,^/+,,;
    }

    # Split the path into its component parts and process each separately.
    my @elems = split /\//, $path;

    foreach (@elems) {
        if ($_ eq "" || $_ eq ".") { 
            # Ignore these.
            next;
        } elsif ($_ eq "..") {
            # Go to parent directory.
            if ($newdir eq "") {
                print {$conn->{socket}} "550 Directory not found.\r\n";
                return;
            }
            $newdir = substr ($newdir, 0, rindex ($newdir, "/"));
        } else {
            # Go into subdirectory, if it exists.
            $newdir .= ("/" . $_);
            if (! -d $conn->{rootdir} . $newdir) {
                print {$conn->{socket}} "550 Directory not found.\r\n";
                return;
            }
        }
    }

    $conn->{dir} = $newdir;
}

sub _LIST_command
{
    my ($conn, $cmd, $path) = @_;

    # This is something of a hack. Some clients expect a Unix server
    # to respond to flags on the 'ls command line'. Remove these flags
    # and ignore them. This is particularly an issue with ncftp 2.4.3.
    $path =~ s/^-[a-zA-Z0-9]+\s?//;

    my $dir = $conn->{dir};

    print STDERR "_LIST_command - dir is: $dir\n";

    # Absolute path?
    if (substr ($path, 0, 1) eq "/") {
        $dir = "/";
        $path =~ s,^/+,,;
    }
    
    # Parse the first elements of the path until we find the appropriate
    # working directory.
    my @elems = split /\//, $path;
    my ($wildcard, $filename);
    local $_;

    for (my $i = 0; $i < @elems; ++$i) {
        $_ = $elems[$i];
        my $lastelement = $i == @elems-1;

        if ($_ eq "" || $_ eq ".") { next } # Ignore these.
        elsif ($_ eq "..") {
            # Go to parent directory.
            unless ($dir eq "/") {
                $dir = substr ($dir, 0, rindex ($dir, "/"));
            }
        } else {
            if (!$lastelement) { # These elements can only be directories.
                unless (-d $conn->{rootdir} . $dir . $_) {
                    print {$conn->{socket}} "550 File or directory not found.\r\n";
                    return;
                }
                $dir .= $_;
            } else { # It's the last element: check if it's a file, directory or wildcard.
                if (-f $conn->{rootdir} . $dir . $_) { 
                    # It's a file.
                    $filename = $_;
                } elsif (-d $conn->{rootdir} . $dir . $_) { 
                    # It's a directory.
                    $dir .= $_;
                } elsif (/\*/ || /\?/) {
                    # It is a wildcard.
                    $wildcard = $_;
                } else {
                    print {$conn->{socket}} "550 File or directory not found.\r\n";
                    return;
                }
            }
        }
    }
    
    print STDERR "_LIST_command - dir is: $dir\n" if $log;
    
    print {$conn->{socket}} "150 Opening data connection for file listing.\r\n";

    # Open a path back to the client.
    my $sock = __open_data_connection ($conn);

    unless ($sock) {
        print {$conn->{socket}} "425 Can't open data connection.\r\n";
        return;
    }

    # If the path contains a directory name, extract it so that
    # we can prefix it to every filename listed.
    my $prefix = (($filename || $wildcard) && $path =~ /(.*\/).*/) ? $1 : "";
    
    print STDERR "_LIST_command - prefix is: $prefix\n" if $log;

    # OK, we're either listing a full directory, listing a single
    # file or listing a wildcard.
    if ($filename) {            # Single file.
        __list_file ($sock, $prefix . $filename);
    } else {                    # Wildcard or full directory $dirh.
        unless ($wildcard) {
            # Synthesize (fake) "total" field for directory listing.
            print $sock "total 1 \r\n";
        }

        foreach (__get_file_list ($conn->{rootdir} . $dir, $wildcard)) {
            __list_file ($sock, $prefix . $_);
        }
    }
    
    unless ($sock->close) {
        print {$conn->{socket}} "550 Error closing data connection: $!\r\n";
        return;
    }

    print {$conn->{socket}} "226 Listing complete. Data connection has been closed.\r\n";
}

sub _PASS_command
{
    my ($conn, $cmd, $pass) = @_;

    # TODO: implement authentication?

    print STDERR "switching to LOGGEDIN state\n" if $log;
    $conn->{state} = $_connection_states{LOGGEDIN};
    
    if ($conn->{username} eq "anonymous") {
        print {$conn->{socket}} "202 Anonymous user access is always granted.\r\n";
    } else {
        print {$conn->{socket}} "230 Authentication not implemented yet, access is always granted.\r\n";
    }
}

sub _PASV_command
{
    my ($conn, $cmd, $rest) = @_;
    
    # Open a listening socket - but don't actually accept on it yet.
    "0" =~ /(0)/; # Perl 5.7 / IO::Socket::INET bug workaround.
    my $sock = IO::Socket::INET->new (LocalHost => '127.0.0.1',
                                      LocalPort => '0',
                                      Listen => 1,
                                      Reuse => 1,
                                      Proto => 'tcp',
                                      Type => SOCK_STREAM);

    unless ($sock) {
        # Return a code 550 here, even though this is not in the RFC. XXX
        print {$conn->{socket}} "550 Can't open a listening socket.\r\n";
        return;
    }

    $conn->{passive} = 1;
    $conn->{passive_socket} = $sock;

    # Get our port number.
    my $sockport = $sock->sockport;

    # Split the port number into high and low components.
    my $p1 = int ($sockport / 256);
    my $p2 = $sockport % 256;

    $conn->{state} = $_connection_states{TWOSOCKS};
    
    # We only accept connections from localhost.
    print {$conn->{socket}} "227 Entering Passive Mode (127,0,0,1,$p1,$p2)\r\n";
}

sub _PORT_command
{
    my ($conn, $cmd, $rest) = @_;

    # The arguments to PORT are a1,a2,a3,a4,p1,p2 where a1 is the
    # most significant part of the address (eg. 127,0,0,1) and
    # p1 is the most significant part of the port.
    unless ($rest =~ /^\s*(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})/) {
        print {$conn->{socket}} "501 Syntax error in PORT command.\r\n";
        return;
    }

    # Check host address.
    unless ($1  > 0 && $1 < 224 &&
            $2 >= 0 && $2 < 256 &&
            $3 >= 0 && $3 < 256 &&
            $4 >= 0 && $4 < 256) {
        print {$conn->{socket}} "501 Invalid host address.\r\n";
        return;
    }

    # Construct host address and port number.
    my $peeraddrstring = "$1.$2.$3.$4";
    my $peerport = $5 * 256 + $6;

    # Check port number.
    unless ($peerport > 0 && $peerport < 65536) {
        print {$conn->{socket}} "501 Invalid port number.\r\n";
    }

    $conn->{peeraddrstring} = $peeraddrstring;
    $conn->{peeraddr} = inet_aton ($peeraddrstring);
    $conn->{peerport} = $peerport;
    $conn->{passive} = 0;

    $conn->{state} = $_connection_states{TWOSOCKS};

    print {$conn->{socket}} "200 PORT command OK.\r\n";
}

sub _PWD_command
{
    my ($conn, $cmd, $rest) = @_;
    
    # See RFC 959 Appendix II and draft-ietf-ftpext-mlst-11.txt section 6.2.1.
    my $pathname = $conn->{dir};
    $pathname =~ s,/+$,, unless $pathname eq "/";
    $pathname =~ tr,/,/,s;

    print {$conn->{socket}} "257 \"$pathname\"\r\n";
}

sub _REST_command
{
    my ($conn, $cmd, $restart_from) = @_;
    
    unless ($restart_from =~ /^([1-9][0-9]*|0)$/) {
        print {$conn->{socket}} "501 REST command needs a numeric argument.\r\n";
        return;
    }

    $conn->{restart} = $1;

    print {$conn->{socket}} "350 Restarting next transfer at $1.\r\n";
}

sub _RETR_command
{
    my ($conn, $cmd, $path) = @_;
    
    my $dir = $conn->{dir};

    # Absolute path?
    if (substr ($path, 0, 1) eq "/") {
        $dir = "/";
        $path =~ s,^/+,,;
        $path = "." if $path eq "";
    }

    # Parse the first elements of path until we find the appropriate
    # working directory.
    my @elems = split /\//, $path;
    my $filename = pop @elems;

    foreach (@elems) {
        if ($_ eq "" || $_ eq ".") { 
            next # Ignore these.
        } elsif ($_ eq "..") {
            # Go to parent directory.
            unless ($dir eq "/") {
                $dir = substr ($dir, 0, rindex ($dir, "/"));
            }
        } else {
            unless (-d $conn->{rootdir} . $dir . $_) {
                print {$conn->{socket}} "550 File or directory not found.\r\n";
                return;
            }
            $dir .= $_;
        }
    }

    unless (defined $filename && length $filename) {
        print {$conn->{socket}} "550 File or directory not found.\r\n";
	    return;
    }

    if ($filename eq "." || $filename eq "..") {
        print {$conn->{socket}} "550 RETR command is not supported on directories.\r\n";
	    return;
    }
    
    my $fullname = $conn->{rootdir} . $dir . $filename;
    unless (-f $fullname) {
        print {$conn->{socket}} "550 RETR command is only supported on plain files.\r\n";
        return;
    }

    # Try to open the file.
    unless (open (FILE, '<', $fullname)) {
        print {$conn->{socket}} "550 File or directory not found.\r\n";
        return;
    }

    print {$conn->{socket}} "150 Opening " .
        ($conn->{type} eq 'A' ? "ASCII mode" : "BINARY mode") .
        " data connection for file $filename.\r\n";

    # Open a path back to the client.
    my $sock = __open_data_connection ($conn);

    unless ($sock) {
        print {$conn->{socket}} "425 Can't open data connection.\r\n";
        return;
    }

    # What mode are we sending this file in?
    unless ($conn->{type} eq 'A') # Binary type.
    {
        my ($r, $buffer, $n, $w);

        # Restart the connection from previous point?
        if ($conn->{restart}) {
            # VFS seek method only required to support relative forward seeks
            #
            # In Perl = 5.00503, SEEK_CUR is exported by IO::Seekable,
            # in Perl >= 5.6, SEEK_CUR is exported by both IO::Seekable
            # and Fcntl. Hence we 'use IO::Seekable' at the top of the
            # file to get this symbol reliably in both cases.
            sysseek (FILE, $conn->{restart}, SEEK_CUR);
            $conn->{restart} = 0;
        }

        # Copy data.
        while ($r = sysread (FILE, $buffer, 65536))
        {
            # Restart alarm clock timer.
            alarm $conn->{idle_timeout};

            for ($n = 0; $n < $r; )
            {
                $w = syswrite ($sock, $buffer, $r - $n, $n);

                # Cleanup and exit if there was an error.
                unless (defined $w) {
                    close $sock;
                    close FILE;
                    print {$conn->{socket}} "426 File retrieval error: $!. Data connection has been closed.\r\n";
                    return;
                }

                $n += $w;
            }

            # Transfer aborted by client?
            if ($GOT_SIGURG) {
                $GOT_SIGURG = 0;
                close $sock;
                close FILE;
                print {$conn->{socket}} "426 Transfer aborted. Data connection closed.\r\n";
                return;
            }
        }

        # Cleanup and exit if there was an error.
        unless (defined $r) {
            close $sock;
            close FILE;
            print {$conn->{socket}} "426 File retrieval error: $!. Data connection has been closed.\r\n";
            return;
        }
    } else { # ASCII type.
        # Restart the connection from previous point?
        if ($conn->{restart}) {
            for (my $i = 0; $i < $conn->{restart}; ++$i) {
                getc FILE;
            }
            $conn->{restart} = 0;
        }

        # Copy data.
        while (defined ($_ = <FILE>)) {
            # Remove any native line endings.
            s/[\n\r]+$//;

            # Restart alarm clock timer.
            alarm $conn->{idle_timeout};

            # Write the line with telnet-format line endings.
            print $sock "$_\r\n";

            # Transfer aborted by client?
            if ($GOT_SIGURG) {
                $GOT_SIGURG = 0;
                close $sock;
                close FILE;
                print {$conn->{socket}} "426 Transfer aborted. Data connection closed.\r\n";
                return;
            }
        }
    }

    unless (close ($sock) && close (FILE)) {
        print {$conn->{socket}} "550 File retrieval error: $!.\r\n";
        return;
    }

    print {$conn->{socket}} "226 File retrieval complete. Data connection has been closed.\r\n";
}

sub _SIZE_command
{
    my ($conn, $cmd, $path) = @_;
    
    my $dir = $conn->{dir};

    # Absolute path?
    if (substr ($path, 0, 1) eq "/") {
        $dir = "/";
        $path =~ s,^/+,,;
        $path = "." if $path eq "";
    }

    # Parse the first elements of path until we find the appropriate
    # working directory.
    my @elems = split /\//, $path;
    my $filename = pop @elems;

    foreach (@elems) {
        if ($_ eq "" || $_ eq ".") { 
            next # Ignore these.
        } elsif ($_ eq "..") {
            # Go to parent directory.
            unless ($dir eq "/") {
                $dir = substr ($dir, 0, rindex ($dir, "/"));
            }
        } else {
            unless (-d $conn->{rootdir} . $dir . $_) {
                print {$conn->{socket}} "550 File or directory not found.\r\n";
                return;
            }
            $dir .= $_;
        }
    }

    unless (defined $filename && length $filename) {
        print {$conn->{socket}} "550 File or directory not found.\r\n";
	    return;
    }

    if ($filename eq "." || $filename eq "..") {
        print {$conn->{socket}} "550 SIZE command is not supported on directories.\r\n";
	    return;
    }

    my $fullname = $conn->{rootdir} . $dir . $filename;
    unless (-f $fullname) {
        print {$conn->{socket}} "550 SIZE command is only supported on plain files.\r\n";
        return;
    }

    my $size = 0;
    if ($conn->{type} eq 'A') {
        # ASCII mode: we have to count the characters by hand.
        unless (open (FILE, '<', $filename)) {
            print {$conn->{socket}} "550 Cannot calculate size of $filename.\r\n";
            return;
        }
        $size++ while (defined (getc(FILE)));
        close FILE;
    } else {
        # BINARY mode: we can use stat
        $size = (stat($filename))[7];
    }

    print {$conn->{socket}} "213 $size\r\n";
}

sub _SYST_command
{
    my ($conn, $cmd, $dummy) = @_;
    
    print {$conn->{socket}} "215 UNIX Type: L8\r\n";
}

sub _TYPE_command
{
    my ($conn, $cmd, $type) = @_;
    
    # See RFC 959 section 5.3.2.
    if ($type =~ /^([AI])$/i) {
        $conn->{type} = 'A';
    } elsif ($type =~ /^([AI])\sN$/i) {
        $conn->{type} = 'A';
    } elsif ($type =~ /^L\s8$/i) {
        $conn->{type} = 'L8';
    } else {
        print {$conn->{socket}} "504 This server does not support TYPE $type.\r\n";
        return;
    }

    print {$conn->{socket}} "200 TYPE changed to $type.\r\n";
}

sub _USER_command
{
    my ($conn, $cmd, $username) = @_;

    print STDERR "username: $username\n" if $log;
    $conn->{username} = $username;

    print STDERR "switching to WAIT4PWD state\n" if $log;
    $conn->{state} = $_connection_states{WAIT4PWD};
    
    if ($conn->{username} eq "anonymous") {
        print {$conn->{socket}} "230 Anonymous user access granted.\r\n";
    } else {
        print {$conn->{socket}} "331 Password required.\r\n";
    }
}


# HELPER ROUTINES

sub __open_data_connection
{
    my $conn = shift;

    my $sock;

    if ($conn->{passive}) {
        # Passive mode - wait for a connection from the client.
        accept ($sock, $conn->{passive_socket}) or return undef;
    } else {
        # Active mode - connect back to the client.
        "0" =~ /(0)/; # Perl 5.7 / IO::Socket::INET bug workaround.
        $sock = IO::Socket::INET->new (LocalAddr => '127.0.0.1',
                                       PeerAddr => $conn->{peeraddrstring},
                                       PeerPort => $conn->{peerport},
                                       Proto => 'tcp',
                                       Type => SOCK_STREAM) or return undef;
    }

    return $sock;
}


sub __list_file
{
    my $sock = shift;
    my $filename = shift;

    # Get the status information.
    my ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size,
        $atime, $mtime, $ctime, $blksize, $blocks)
      = lstat $filename;

    # If the file has been removed since we created this
    # handle, then $dev will be undefined. Return immediately.
    return unless defined $dev;

    # Generate printable user/group.
    my $user = getpwuid ($uid) || "-";
    my $group = getgrgid ($gid) || "-";

    # Permissions from mode.
    my $perms = $mode & 0777;

    # Work out the mode using special "_" operator which causes Perl
    # to use the result of the previous stat call.
    $mode = (-f _ ? 'f' :
             (-d _ ? 'd' :
              (-l _ ? 'l' :
               (-p _ ? 'p' :
                (-S _ ? 's' :
                 (-b _ ? 'b' :
                  (-c _ ? 'c' : '?')))))));

    # Generate printable date (this logic is taken from GNU fileutils:
    # src/ls.c: print_long_format).
    my $time = time;
    my $fmt;
    if ($time > $mtime + 6 * 30 * 24 * 60 * 60 || $time < $mtime - 60 * 60) {
        $fmt = "%b %e  %Y";
    } else {
        $fmt = "%b %e %H:%M";
    }

    my $fmt_time = strftime $fmt, localtime ($mtime);

    # Generate printable permissions.
    my $fmt_perms = join "",
      ($perms & 0400 ? 'r' : '-'),
      ($perms & 0200 ? 'w' : '-'),
      ($perms & 0100 ? 'x' : '-'),
      ($perms & 040 ? 'r' : '-'),
      ($perms & 020 ? 'w' : '-'),
      ($perms & 010 ? 'x' : '-'),
      ($perms & 04 ? 'r' : '-'),
      ($perms & 02 ? 'w' : '-'),
      ($perms & 01 ? 'x' : '-');

    # Printable file type.
    my $fmt_mode = $mode eq 'f' ? '-' : $mode;

    # If it's a symbolic link, display the link.
    my $link;
    if ($mode eq 'l') {
        $link = readlink $filename;
        die "readlink: $!" unless defined $link;
    }
    my $fmt_link = defined $link ? " -> $link" : "";

    # Display the file.
    my $line = sprintf
      ("%s%s%4d %-8s %-8s %8d %s %s%s\r\n",
       $fmt_mode,
       $fmt_perms,
       $nlink,
       $user,
       $group,
       $size,
       $fmt_time,
       $filename,
       $fmt_link);
    $sock->print ($line);
}


sub __get_file_list
{
    my $dir = shift;
    my $wildcard = shift;

    opendir (DIRHANDLE, $dir)
        or die "Cannot open directory!!!";

    my @allfiles = readdir DIRHANDLE;
    my @filenames = ();
    
    if ($wildcard) {
        # Get rid of . and ..
        @allfiles = grep !/^\.{1,2}$/, @allfiles;
        
        # Convert wildcard to a regular expression.
        $wildcard = __wildcard_to_regex ($wildcard);

        @filenames = grep /$wildcard/, @allfiles;
    } else {
        @filenames = @allfiles;
    }

    closedir (DIRHANDLE);

    return sort @filenames;
}


sub __wildcard_to_regex
{
    my $wildcard = shift;

    $wildcard =~ s,([^?*a-zA-Z0-9]),\\$1,g; # Escape punctuation.
    $wildcard =~ s,\*,.*,g; # Turn * into .*
    $wildcard =~ s,\?,.,g;  # Turn ? into .
    $wildcard = "^$wildcard\$"; # Bracket it.

    return $wildcard;
}


###########################################################################
# FTPSERVER CLASS
###########################################################################

{
    my %_attr_data = ( # DEFAULT
        _localAddr  => 'localhost',
        _localPort  => 8021,
        _reuseAddr  => 1,
        _rootDir    => Cwd::getcwd(),
    );
    
    sub _default_for
    {
        my ($self, $attr) = @_;
        $_attr_data{$attr};
    }

    sub _standard_keys 
    {
        keys %_attr_data;
    }
}


sub new {
    my ($caller, %args) = @_;
    my $caller_is_obj = ref($caller);
    my $class = $caller_is_obj || $caller;
    my $self = bless {}, $class;
    foreach my $attrname ($self->_standard_keys()) {
        my ($argname) = ($attrname =~ /^_(.*)/);
        if (exists $args{$argname}) {
            $self->{$attrname} = $args{$argname};
        } elsif ($caller_is_obj) {
            $self->{$attrname} = $caller->{$attrname};
        } else {
            $self->{$attrname} = $self->_default_for($attrname);
        }
    }
    return $self;
}


sub run 
{
    my ($self, $synch_callback) = @_;
    my $initialized = 0;

    # turn buffering off on STDERR
    select((select(STDERR), $|=1)[0]);

    # initialize command table
    my $command_table = {};
    foreach (keys %_commands) {
        my $subname = "_${_}_command";
        $command_table->{$_} = \&$subname;
    }

    my $old_ils = $/;
    $/ = "\r\n";

    # create server socket
    "0" =~ /(0)/; # Perl 5.7 / IO::Socket::INET bug workaround.
    my $server_sock = IO::Socket::INET->new (LocalHost => $self->{_localAddr},
                                             LocalPort => $self->{_localPort},
                                             Listen => 1,
                                             Reuse => $self->{_reuseAddr},
                                             Proto => 'tcp',
                                             Type => SOCK_STREAM) or die "bind: $!";

    if (!$initialized) {
        $synch_callback->();
        $initialized = 1;
    }

    $SIG{CHLD} = sub { wait };

    # the accept loop
    while (my $client_addr = accept (my $socket, $server_sock))
    {    
        # turn buffering off on $socket
        select((select($socket), $|=1)[0]);
        
        # find out who connected    
        my ($client_port, $client_ip) = sockaddr_in ($client_addr);
        my $client_ipnum = inet_ntoa ($client_ip);

        # print who connected
        print STDERR "got a connection from: $client_ipnum\n" if $log;

        # fork off a process to handle this connection.
        my $pid = fork();
        unless (defined $pid) {
            warn "fork: $!";
            sleep 5; # Back off in case system is overloaded.
            next;
        }

        if ($pid == 0) { # Child process.

            # install signals
            $SIG{URG}  = sub { 
                $GOT_SIGURG  = 1; 
            };

            $SIG{PIPE} = sub {
                print STDERR "Client closed connection abruptly.\n";
                exit;
            };

            $SIG{ALRM} = sub {
                print STDERR "Connection idle timeout expired. Closing server.\n";
                exit;
            };
            
            #$SIG{CHLD} = 'IGNORE';


            print STDERR "in child\n" if $log;

            my $conn = { 
                'socket'       => $socket, 
                'state'        => $_connection_states{NEWCONN},
                'dir'          => '/',
                'restart'      => 0,
                'idle_timeout' => 60, # 1 minute timeout
                'rootdir'      => $self->{_rootDir},
            };
        
            print {$conn->{socket}} "220 GNU Wget Testing FTP Server ready.\r\n";

            # command handling loop
            for (;;) {
                print STDERR "waiting for request\n" if $log;

                last unless defined (my $req = <$socket>);

                # Remove trailing CRLF.
                $req =~ s/[\n\r]+$//;

                print STDERR "received request $req\n" if $log;

                # Get the command.
                # See also RFC 2640 section 3.1.
                unless ($req =~ m/^([A-Z]{3,4})\s?(.*)/i) {
                    # badly formed command
                    exit 0;
                }

                # The following strange 'eval' is necessary to work around a
                # very odd bug in Perl 5.6.0. The following assignment to
                # $cmd will fail in some cases unless you use $1 in some sort
                # of an expression beforehand.
                # - RWMJ 2002-07-05.
                eval '$1 eq $1';

                my ($cmd, $rest) = (uc $1, $2);

                # Got a command which matches in the table?
                unless (exists $command_table->{$cmd}) {
                    print {$conn->{socket}} "500 Unrecognized command.\r\n";
                    next;
                }

                # Command requires user to be authenticated?
                unless ($_commands{$cmd} | $conn->{state}) {
                    print {$conn->{socket}} "530 Not logged in.\r\n";
                    next;
                }
                
                # Handle the QUIT command specially.
                if ($cmd eq "QUIT") {
                    print {$conn->{socket}} "221 Goodbye. Service closing connection.\r\n";
                    last;
                }

                # Run the command.
                &{$command_table->{$cmd}} ($conn, $cmd, $rest);
            }
        } else { # Father
            close $socket;
        }
    } 

    $/ = $old_ils;
}

1;

# vim: et ts=4 sw=4

