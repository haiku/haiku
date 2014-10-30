#!/usr/bin/env python

import socket
import subprocess
import sys


address = '0.0.0.0'
port = 4242


def receiveExactly(connection, size):
	data = '';
	while size > 0:
		dataReceived = connection.recv(size)
		if not dataReceived:
			raise EOFError()
		data += dataReceived
		size -= len(dataReceived)
	return data


def handleConnection(listenerSocket):
	(controlConnection, controlAddress) = listenerSocket.accept()
	(stdioConnection, stdioAddress) = listenerSocket.accept()
	(stderrConnection, stderrAddress) = listenerSocket.accept()

	print 'accepted client connections'

	try:
		commandLength = receiveExactly(controlConnection, 8)
		commandToRun = receiveExactly(controlConnection, int(commandLength))

		print 'received command: ' + commandToRun

		exitCode = subprocess.call(commandToRun, stdin=stdioConnection,
			stdout=stdioConnection, stderr=stderrConnection, shell=True)

		controlConnection.send(str(exitCode))
	finally:
		controlConnection.close()
		stdioConnection.close()
		stderrConnection.close()

	print 'client connections closed'


listenerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listenerSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

try:
	listenerSocket.bind((address, port))
except socket.error, msg:
	sys.exit('Failed to bind to %s port %d: %s' % (address, port, msg[1]))

listenerSocket.listen(3)

print 'started listening on adddress %s port %s' % (address, port)

while True:
	handleConnection(listenerSocket)
