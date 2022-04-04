#
# Copyright 2020 Haiku, Inc. All rights reserved.
# Distributed under the terms of the MIT License.
#
# Authors:
#  Kyle Ambroff-Kao, kyle@ambroffkao.com
#

"""
Transparent HTTP proxy.
"""

import http.client
import http.server
import optparse
import socket
import sys
import urllib.parse


class RequestHandler(http.server.BaseHTTPRequestHandler):
    """
    Implement the basic requirements for a transparent HTTP proxy as defined
    by RFC 7230. Enough of the functionality is implemented to support the
    integration tests in HttpTest that use the HTTP proxy feature.

    There are many error conditions and failure modes which are not handled.
    Those cases can be added as the test suite expands to handle more error
    cases.
    """
    def __init__(self, *args, **kwargs):
        # This is used to hold on to persistent connections to the downstream
        # servers. This maps downstream_host:port => HTTPConnection
        #
        # This implementation is not thread safe, but that's OK we only have
        # a single thread anyway.
        self._connections = {}

        super(RequestHandler, self).__init__(*args, **kwargs)

    def _proxy_request(self):
        # Extract the downstream server from the request path.
        #
        # Note that no attempt is made to prevent message forwarding loops
        # here. This doesn't need to be a complete proxy implementation, just
        # enough of one for integration tests. RFC 7230 section 5.7 says if
        # this were a complete implementation, it would have to make sure that
        # the target system was not this process to avoid a loop.
        target = urllib.parse.urlparse(self.path)

        # If Connection: close wasn't used, then we may still have a connection
        # to this downstream server handy.
        conn = self._connections.get(target.netloc, None)
        if conn is None:
            conn = http.client.HTTPConnection(target.netloc)

        # Collect headers from client which will be sent to the downstream
        # server.
        client_headers = {}
        for header_name in self.headers:
            if header_name in ('Host', 'Content-Length'):
                continue
            for header_value in self.headers.get_all(header_name):
                client_headers[header_name] = header_value

        # Compute X-Forwarded-For header
        client_address = '{}:{}'.format(*self.client_address)
        x_forwarded_for_header = self.headers.get('X-Forwarded-For', None)
        if x_forwarded_for_header is None:
            client_headers['X-Forwarded-For'] = client_address
        else:
            client_headers['X-Forwarded-For'] = \
                x_forwarded_for_header + ', ' + client_address

        # Read the request body from client.
        request_body_length = int(self.headers.get('Content-Length', '0'))
        request_body = self.rfile.read(request_body_length)

        # Send the request to the downstream server
        if target.query:
            target_path = target.path + '?' + target.query
        else:
            target_path = target.path
        conn.request(self.command, target_path, request_body, client_headers)
        response = conn.getresponse()

        # Echo the response to the client.
        self.send_response_only(response.status, response.reason)
        for header_name, header_value in response.headers.items():
            self.send_header(header_name, header_value)
        self.end_headers()

        # Read the response body from upstream and write it to downstream, if
        # there is a response body at all.
        response_content_length = \
            int(response.headers.get('Content-Length', '0'))
        if response_content_length > 0:
            self.wfile.write(response.read(response_content_length))

        # Cleanup, possibly hang on to persistent connection to target
        # server.
        connection_header_value = self.headers.get('Connection', None)
        if response.will_close or connection_header_value == 'close':
            conn.close()
            self.close_connection = True
        else:
            # Hang on to this connection for future requests. This isn't
            # really bulletproof but it's good enough for integration tests.
            self._connections[target.netloc] = conn

        self.log_message(
            'Proxied request from %s to %s',
            client_address,
            self.path)

    def do_GET(self):
        self._proxy_request()

    def do_HEAD(self):
        self._proxy_request()

    def do_POST(self):
        self._proxy_request()

    def do_PUT(self):
        self._proxy_request()

    def do_DELETE(self):
        self._proxy_request()

    def do_PATCH(self):
        self._proxy_request()

    def do_OPTIONS(self):
        self._proxy_request()


def main():
    options = parse_args(sys.argv)

    bind_addr = (
        options.bind_addr,
        0 if options.port is None else options.port)

    server = http.server.HTTPServer(
        bind_addr,
        RequestHandler,
        bind_and_activate=False)
    if options.port is None:
        server.server_port = server.socket.getsockname()[1]
    else:
        server.server_port = options.port

    if options.server_socket_fd:
        server.socket = socket.fromfd(
            options.server_socket_fd,
            socket.AF_INET,
            socket.SOCK_STREAM)
    else:
        server.server_bind()
        server.server_activate()

    print(
        'Transparent HTTP proxy listening on port',
        server.server_port,
        file=sys.stderr)
    try:
        server.serve_forever(0.01)
    except KeyboardInterrupt:
        server.server_close()


def parse_args(argv):
    parser = optparse.OptionParser(
        usage='Usage: %prog [OPTIONS]',
        description=__doc__)
    parser.add_option(
        '--bind-addr',
        default='127.0.0.1',
        dest='bind_addr',
        help='By default only bind to loopback')
    parser.add_option(
        '--port',
        dest='port',
        default=None,
        type='int',
        help='If not specified a random port will be used.')
    parser.add_option(
        "--fd",
        dest='server_socket_fd',
        default=None,
        type='int',
        help='A socket FD to use for accept() instead of binding a new one.')
    options, args = parser.parse_args(argv)
    if len(args) > 1:
        parser.error('Unexpected arguments: {}'.format(', '.join(args[1:])))
    return options


if __name__ == '__main__':
    main()
