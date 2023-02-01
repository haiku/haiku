#
# Copyright 2020 Haiku, Inc. All rights reserved.
# Distributed under the terms of the MIT License.
#
# Authors:
#  Kyle Ambroff-Kao, kyle@ambroffkao.com
#

"""
HTTP(S) server used for integration testing of ServicesKit.

This service receives HTTP requests and just echos them back in the response.

This is intentionally not using any fancy frameworks or libraries so as to not
require any dependencies, and also to allow for adding endpoints to replicate
behavior of other servers in the future.
"""

import abc
import base64
import gzip
import hashlib
import http.server
import io
import optparse
import os
import re
import socket
import ssl
import subprocess
import sys
import tempfile
import zlib


MULTIPART_FORM_BOUNDARY_RE = re.compile(
    r'^multipart/form-data; boundary=(----------------------------\d+)$')
AUTH_PATH_RE = re.compile(
    r'^/auth/(?P<strategy>(basic|digest))'
    '/(?P<username>[a-z0-9]+)/(?P<password>[a-z0-9]+)',
    re.IGNORECASE)


class RequestHandler(http.server.BaseHTTPRequestHandler):
    """
    Any GET or POST request just gets echoed back to the sender. If the path
    ends with a numeric component like "/404" or "/500", then that value will
    be set as the status code in the response.

    Note that this isn't meant to replicate expected functionality exactly.
    Rather than implementing all of these status codes as expected per RFC,
    such as having an empty response body for 201 response, only the
    functionality that is required to handle requests from HttpTests is
    implemented.

    There can also be endpoints here that are intentionally non-compliant in
    order to exercize the HTTP client's behavior when a server is badly
    behaved.
    """
    def do_GET(self, write_response=True):
        authorized, extra_headers = self._authorize()
        if not authorized:
            return

        encoding, response_body = self._build_response_body()

        status_code = extract_desired_status_code_from_path(self.path, 200)
        self.send_response(status_code)
        if status_code >= 300 and status_code < 400:
            self.send_header('Location', '/')

        if status_code == 204:
            write_response = False
        else:
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Content-Length', str(len(response_body)))
            if encoding:
                self.send_header('Content-Encoding', encoding)

        for header_name, header_value in extra_headers:
            self.send_header(header_name, header_value)
        self.end_headers()

        if write_response:
            self.wfile.write(response_body)

    def do_HEAD(self):
        self.do_GET(False)

    def do_POST(self):
        authorized, extra_headers = self._authorize()
        if not authorized:
            return

        encoding, response_body = self._build_response_body()
        self.send_response(
            extract_desired_status_code_from_path(self.path, 200))
        self.send_header('Content-Type', 'text/plain')
        self.send_header('Content-Length', str(len(response_body)))
        if encoding:
            self.send_header('Content-Encoding', encoding)
        for header_name, header_value in extra_headers:
            self.send_header(header_name, header_value)

        self.end_headers()
        self.wfile.write(response_body)

    def do_DELETE(self):
        self._not_supported()

    def do_PATCH(self):
        self._not_supported()

    def do_OPTIONS(self):
        self._not_supported()

    def send_response(self, code, message=None):
        self.log_request(code)
        self.send_response_only(code, message)
        self.send_header('Server', 'Test HTTP Server for Haiku')
        self.send_header('Date', 'Sun, 09 Feb 2020 19:32:42 GMT')

    def _build_response_body(self):
        # The post-body may be multi-part/form-data, in which case the client
        # will have generated some random identifier to identify the boundary.
        # If that's the case, we'll replace it here in order to allow the test
        # client to validate the response data without needing to predict the
        # boundary identifier. This makes the response body deterministic even
        # though the boundary will change with every request, and lets the
        # tests in HttpTests hard-code the entire expected response body for
        # validation.
        boundary_id_value = None

        supported_encodings = [
            e.strip()
            for e in self.headers.get('Accept-Encoding', '').split(',')
            if e.strip()]
        if 'gzip' in supported_encodings:
            encoding = 'gzip'
            output_stream = GzipResponseBodyBuilder()
        elif 'deflate' in supported_encodings:
            encoding = 'deflate'
            output_stream = DeflateResponseBodyBuilder()
        else:
            encoding = None
            output_stream = RawResponseBodyBuilder()

        output_stream.write(
            'Path: {}\r\n\r\n'.format(self.path).encode('utf-8'))
        output_stream.write(b'Headers:\r\n')
        output_stream.write(b'--------\r\n')
        for header in self.headers:
            for header_value in self.headers.get_all(header):
                if header in ('Host', 'Referer', 'X-Forwarded-For'):
                    # The server port can change between runs which will change
                    # the size and contents of the response body. To make tests
                    # that verify the contents of the response body easier the
                    # server port will be stripped from these headers when
                    # echoed to the response body.
                    header_value = re.sub(r':[0-9]+', ':PORT', header_value)

                    # The scheme will also be in this header value, and we want
                    # to return the same reguardless of whether http:// or
                    # https:// was used.
                    header_value = re.sub(
                        r'https?://',
                        'SCHEME://',
                        header_value)
                if header == 'Content-Type':
                    match = MULTIPART_FORM_BOUNDARY_RE.match(
                        self.headers.get('Content-Type', 'text/plain'))
                    if match is not None:
                        boundary_id_value = match.group(1)
                        header_value = header_value.replace(
                            boundary_id_value,
                            '<<BOUNDARY-ID>>')
                output_stream.write(
                    '{}: {}\r\n'.format(header, header_value).encode('utf-8'))

        content_length = int(self.headers.get('Content-Length', 0))
        if content_length > 0:
            output_stream.write(b'\r\n')
            output_stream.write(b'Request body:\r\n')
            output_stream.write(b'-------------\r\n')

            body_bytes = self.rfile.read(content_length).decode('utf-8')
            if boundary_id_value:
                body_bytes = body_bytes.replace(
                    boundary_id_value, '<<BOUNDARY-ID>>')

            output_stream.write(body_bytes.encode('utf-8'))
            output_stream.write(b'\r\n')

        return encoding, output_stream.get_bytes()

    def _not_supported(self):
        self.send_response(405, '{} not supported'.format(self.command))
        self.end_headers()
        self.wfile.write(
            '{} not supported\r\n'.format(self.command).encode('utf-8'))

    def _authorize(self):
        """
        Authorizes the request. If True is returned that means that the
        request was not authorized and the 4xx response has been send to the
        client.
        """
        # We only authorize paths like
        # /auth/<strategy>/<expected-username>/<expected-password>
        match = AUTH_PATH_RE.match(self.path)
        if match is None:
            return True, []

        strategy = match.group('strategy')
        expected_username = match.group('username')
        expected_password = match.group('password')

        if strategy == 'basic':
            return self._handle_basic_auth(
                expected_username,
                expected_password)
        elif strategy == 'digest':
            return self._handle_digest_auth(
                expected_username,
                expected_password)
        else:
            raise NotImplementedError(
                'Unimplemented authorization strategy ' + strategy)

    def _handle_basic_auth(self, expected_username, expected_password):
        authorization = self.headers.get('Authorization', None)
        auth_type = None
        encoded_credentials = None
        username = None
        password = None

        if authorization:
            auth_type, encoded_credentials = authorization.split()

        if encoded_credentials is not None:
            decoded = base64.decodebytes(encoded_credentials.encode('utf-8'))
            username, password = decoded.decode('utf-8').split(':')

        if authorization is None or auth_type != 'Basic' \
                or encoded_credentials is None \
                or username != expected_username \
                or password != expected_password:
            self.send_response(401, 'Not authorized')
            self.send_header('Www-Authenticate', 'Basic realm="Fake Realm"')
            self.end_headers()
            return False, []

        return True, [('Www-Authenticate', 'Basic realm="Fake Realm"')]

    def _handle_digest_auth(self, expected_username, expected_password):
        """
        Implement enough of the digest auth RFC to make tests pass.
        """
        # Note: These values will always be the same because we want the
        # response to be deterministic for testing purposes.
        NONCE = 'f3a95f20879dd891a5544bf96a3e5518'
        OPAQUE = 'f0bb55f1221a51b6d38117c331611799'

        extra_headers = []
        authorization = self.headers.get('Authorization', None)
        credentials = None
        auth_type = None
        if authorization is not None:
            auth_type, fields = authorization.split(maxsplit=1)
            if auth_type == 'Digest':
                credentials = parse_kv_pair_header(fields)

        expected_response_hash = None
        if credentials:
            expected_response_hash = compute_digest_challenge_response_hash(
                self.command,
                self.path,
                '',
                credentials,
                expected_password)

        if authorization is None or credentials is None \
                or auth_type != 'Digest' \
                or expected_response_hash != credentials.get('response'):
            self.send_response(401, 'Not authorized')
            self.send_header(
                'Www-Authenticate',
                'Digest realm="user@shredder",'
                ' nonce="{}",'
                ' qop="auth",'
                ' opaque={},'
                ' algorithm=MD5,'
                ' stale=FALSE'.format(NONCE, OPAQUE))
            self.send_header('Set-Cookie', 'stale_after=never; Path=/')
            self.send_header('Set-Cookie', 'fake=fake_value; Path=/')
            self.end_headers()
            return False, extra_headers

        return True, extra_headers


class ResponseBodyBuilder(object):
    __meta__ = abc.ABCMeta

    @abc.abstractmethod
    def write(self, bytes):
        raise NotImplementedError()

    @abc.abstractmethod
    def get_bytes(self):
        raise NotImplementedError()


class RawResponseBodyBuilder(ResponseBodyBuilder):
    def __init__(self):
        self.buf = io.BytesIO()

    def write(self, bytes):
        self.buf.write(bytes)

    def get_bytes(self):
        return self.buf.getvalue()


class GzipResponseBodyBuilder(ResponseBodyBuilder):
    def __init__(self):
        self.buf = io.BytesIO()
        self.compressor = gzip.GzipFile(
            mode='wb',
            compresslevel=4,
            fileobj=self.buf)

    def write(self, bytes):
        self.compressor.write(bytes)

    def get_bytes(self):
        self.compressor.close()
        return self.buf.getvalue()


class DeflateResponseBodyBuilder(ResponseBodyBuilder):
    def __init__(self):
        self.raw = RawResponseBodyBuilder()

    def write(self, bytes):
        self.raw.write(bytes)

    def get_bytes(self):
        return zlib.compress(self.raw.get_bytes())


def extract_desired_status_code_from_path(path, default=200):
    status_code = default
    path_parts = os.path.split(path)
    try:
        status_code = int(path_parts[-1])
    except ValueError:
        pass
    return status_code


def generate_self_signed_tls_cert(common_name, cert_path, key_path):
    subprocess.check_call([
        'openssl',
        'req',
        '-x509',
        '-nodes',
        '-subj', '/CN={}'.format(common_name),
        '-newkey', 'rsa:4096',
        '-keyout', key_path,
        '-out', cert_path,
        '-days', '1'
    ])


def compute_digest_challenge_response_hash(
        request_method,
        request_uri,
        request_body,
        credentials,
        expected_password):
    """
    Compute hash as defined by RFC2069, although this isn't an attempt to be
    perfect, just enough for basic integration tests in HttpTests to work.

    :param credentials: Map of values parsed from the Authorization header
                        from the client.
    :param expected_password: The known correct password of the user
                              attempting to authenticate.
    :return: None if a hash cannot be produced, otherwise the hash as defined
             by RFC2069.
    """
    algorithm = credentials.get('algorithm')
    if algorithm == 'MD5':
        hashfunc = hashlib.md5
    elif algorithm == 'SHA-256':
        hashfunc = hashlib.sha256
    elif algorithm == 'SHA-512':
        hashfunc = hashlib.sha512
    else:
        return None

    realm = credentials.get('realm')
    username = credentials.get('username')

    ha1 = hashfunc(':'.join([
        username,
        realm,
        expected_password]).encode('utf-8')).hexdigest()

    qop = credentials.get('qop')
    if qop is None or qop == 'auth':
        ha2 = hashfunc(':'.join([
            request_method,
            request_uri]).encode('utf-8')).hexdigest()
    elif qop == 'auth-int':
        ha2 = hashfunc(':'.join([
            request_method,
            request_uri,
            request_body]).encode('utf-8')).hexdigest()
    else:
        ha2 = None

    if ha1 is None or ha2 is None:
        return None

    if qop is None:
        return hashfunc(':'.join([
            ha1,
            credentials.get('nonce', ''),
            ha2]).encode('utf-8')).hexdigest()
    elif qop == 'auth' or qop == 'auth-int':
        hash_components = [
            ha1,
            credentials.get('nonce', ''),
            credentials.get('nc', ''),
            credentials.get('cnonce', ''),
            qop,
            ha2]
        return hashfunc(':'.join(hash_components).encode('utf-8')).hexdigest()


def parse_kv_pair_header(header_value, sep=','):
    d = {}
    for kvpair in header_value.split(sep):
        key, value = kvpair.strip().split('=')
        d[key.strip()] = value.strip().strip('"')
    return d


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

    def run_server():
        if not options.server_socket_fd:
            server.server_bind()
            server.server_activate()
        print(
            'Test server listening on port',
            server.server_port,
            file=sys.stderr)
        server.serve_forever(0.01)

    try:
        if options.use_tls:
            with tempfile.TemporaryDirectory() as temp_cert_dir:
                common_name = options.bind_addr + ':' + str(options.port)
                cert_file = os.path.join(temp_cert_dir, 'cert.pem')
                key_file = os.path.join(temp_cert_dir, 'key.pem')
                generate_self_signed_tls_cert(
                    common_name,
                    cert_file,
                    key_file)
                server.socket = ssl.wrap_socket(
                    server.socket,
                    certfile=cert_file,
                    keyfile=key_file,
                    server_side=True,
                    do_handshake_on_connect=False)
            run_server()
        else:
            run_server()
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
        '--use-tls',
        dest='use_tls',
        default=False,
        action='store_true',
        help='If set, a self-signed TLS certificate, key and CA will be'
        ' generated for testing purposes.')
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
