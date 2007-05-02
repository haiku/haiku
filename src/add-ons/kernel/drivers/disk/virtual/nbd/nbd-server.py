#!/usr/bin/python
# from http://lists.canonical.org/pipermail/kragen-hacks/2004-May/000397.html
import struct, socket, sys
# network block device server, substitute for nbd-server.  Probably slower.
# But it works!  And it's probably a lot easier to improve the
# performance of this Python version than of the C version.  This
# Python version is 14% of the size and perhaps 20% of the features of
# the C version.  Hmm, that's not so great after all...
# Working:
# - nbd protocol
# - read/write serving up files
# - error handling
# - file size detection
# - in theory, large file support... not really
# - so_reuseaddr
# - nonforking
# Missing:
# - reporting errors to client (in particular writing and reading past end)
# - multiple clients (this probably requires copy-on-write or read-only)
# - copy on write
# - read-only
# - permission tracking
# - idle timeouts
# - running from inetd
# - filename substitution
# - partial file exports
# - exports of large files (bigger than 1/4 of RAM)
# - manual exportsize specification
# - so_keepalive
# - that "split an export file into multiple files" thing that sticks the .0
#   on the end of your filename
# - backgrounding
# - daemonizing

class Error(Exception): pass

class buffsock:
    "Buffered socket wrapper; always returns the amount of data you want."
    def __init__(self, sock): self.sock = sock
    def recv(self, nbytes):
        rv = ''
        while len(rv) < nbytes:
            more = self.sock.recv(nbytes - len(rv))
            if more == '': raise Error(nbytes)
            rv += more
        return rv
    def send(self, astring): self.sock.send(astring)
    def close(self): self.sock.close()
            

class debugsock:
    "Debugging socket wrapper."
    def __init__(self, sock): self.sock = sock
    def recv(self, nbytes):
        print "recv(%d) =" % nbytes,
        rv = self.sock.recv(nbytes)
        print `rv`
        return rv
    def send(self, astring):
        print "send(%r) =" % astring,
        rv = self.sock.send(astring)
        print `rv`
        return rv
    def close(self):
        print "close()"
        self.sock.close()

def negotiation(exportsize):
    "Returns initial NBD negotiation sequence for exportsize in bytes."
    return ('NBDMAGIC' + '\x00\x00\x42\x02\x81\x86\x12\x53' +
        struct.pack('>Q', exportsize) + '\0' * 128);

def nbd_reply(error=0, handle=1, data=''):
    "Construct an NBD reply."
    assert type(handle) is type('') and len(handle) == 8
    return ('\x67\x44\x66\x98' + struct.pack('>L', error) + handle + data)
     
# possible request types
read_request = 0
write_request = 1
disconnect_request = 2

class nbd_request:
    "Decodes an NBD request off the TCP socket."
    def __init__(self, conn):
        conn = buffsock(conn)
        template = '>LL8sQL'
        header = conn.recv(struct.calcsize(template))
        (self.magic, self.type, self.handle, self.offset, 
            self.len) = struct.unpack(template, header)
        if self.magic != 0x25609513: raise Error(self.magic)
        if self.type == write_request: 
            self.data = conn.recv(self.len)
            assert len(self.data) == self.len
    def reply(self, error, data=''):
        return nbd_reply(error=error, handle=self.handle, data=data)
    def range(self):
        return slice(self.offset, self.offset + self.len)

def serveclient(asock, afile):
    "Serves a single client until it exits."
    afile.seek(0)
    abuf = list(afile.read())
    asock.send(negotiation(len(abuf)))
    while 1:
        req = nbd_request(asock)
        if req.type == read_request:
            asock.send(req.reply(error=0, 
                data=''.join(abuf[req.range()])))
        elif req.type == write_request:
            abuf[req.range()] = req.data
            afile.seek(req.offset)
            afile.write(req.data)
            afile.flush()
            asock.send(req.reply(error=0))
        elif req.type == disconnect_request:
            asock.close()
            return

def mainloop(listensock, afile):
    "Serves clients forever."
    while 1:
        (sock, addr) = listensock.accept()
        print "got conn on", addr
        serveclient(sock, afile)

def main(argv):
    "Given a port and a filename, serves up the file."
    afile = file(argv[2], 'rb+')
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', int(argv[1])))
    sock.listen(5)
    mainloop(sock, afile)

if __name__ == '__main__': main(sys.argv)