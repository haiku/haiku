#!/usr/bin/env python3
#
# Usage: bootstrap_client.py <address>[:port] <command> ...
#
# <command> and the following arguments are concatenated (separated by a space)
# and passed to a shell on the server.

import os
import select
import socket
import sys


port = 4242
bufferSize = 4 * 1024

# interpret command line args
if len(sys.argv) < 3:
	sys.exit('Usage: ' + sys.argv[0] + ' <address>[:<port>] <command>')

address = sys.argv[1]
portIndex = address.find(':')
if portIndex >= 0:
	port = int(address[portIndex + 1:])
	address = address[:portIndex]

commandToRun = " ".join(sys.argv[2:])

# create sockets and connect to server
controlConnection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
stdioConnection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
stderrConnection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
	controlConnection.connect((address, port))
	stdioConnection.connect((address, port))
	stderrConnection.connect((address, port))
except socket.error as msg:
	sys.exit('Failed to connect to %s port %d: %s' % (address, port, msg))

# send command length and command
controlConnection.send(b"%08d" % len(commandToRun.encode()))
controlConnection.send(commandToRun.encode())

# I/O loop. We quit when all sockets have been closed.
exitCode = b''
connections = [controlConnection, stdioConnection, stderrConnection, sys.stdin]

while connections and (len(connections) > 1 or not sys.stdin in connections):
	(readable, writable, exceptions) = select.select(connections, [],
		connections)

	if sys.stdin in readable:
		data = sys.stdin.readline(bufferSize)
		if data:
			stdioConnection.send(data.encode())
		else:
			connections.remove(sys.stdin)
			stdioConnection.shutdown(socket.SHUT_WR)

	if stdioConnection in readable:
		data = stdioConnection.recv(bufferSize)
		if data:
			sys.stdout.buffer.write(data)
			sys.stdout.buffer.flush()
		else:
			connections.remove(stdioConnection)
		
	if stderrConnection in readable:
		data = stderrConnection.recv(bufferSize)
		if data:
			sys.stderr.buffer.write(data)
			sys.stderr.buffer.flush()
		else:
			connections.remove(stderrConnection)
		
	if controlConnection in readable:
		data = controlConnection.recv(bufferSize)
		if data:
			exitCode += data
		else:
			connections.remove(controlConnection)

# either an exit code has been sent or we consider this an error
if exitCode:
	sys.exit(int(exitCode))
sys.exit(1)
