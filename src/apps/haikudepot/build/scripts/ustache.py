"""
ustache module.

This module alone implements the entire ustache library, including its
minimal command line interface.

The main functions are considered to be :func:`cli`, :func:`render` and
:func:`stream`, and they expose different approaches to template rendering:
command line interface, buffered and streaming rendering API, respectively.

Other functionality will be considered **advanced**, exposing some
implementation-specific complexity and potentially non-standard behavior
which could reduce compatibility with other mustache implentations and
future major ustache versions.

"""
"""
ustache, Mustache for Python
============================

See `README.md`_, `project documentation`_ and `project repository`_.

.. _README.md: https://ustache.readthedocs.io/en/latest/README.html
.. _project documentation: https://ustache.readthedocs.io
.. _project repository: https://gitlab.com/ergoithz/ustache


License
-------

Copyright (c) 2021, Felipe A Hernandez.

MIT License (see `LICENSE`_).

.. _LICENSE: https://gitlab.com/ergoithz/ustache/-/blob/master/LICENSE

"""

import codecs
import collections
import collections.abc
import functools
import sys
import types
import typing

try:
    import xxhash  # type: ignore
    _cache_hash = next(filter(callable, (
        getattr(xxhash, name, None)
        for name in (
        'xxh3_64_intdigest',
        'xxh64_intdigest',
    )
    )))
    """
    Generate template hash using the fastest algorithm available:

    1. :py:func:`xxhash.xxh3_64_intdigest` from ``python-xxhash>=2.0.0``.
    2. :py:func:`xxhash.xxh64_intdigest` from ``python-xxhash>=1.2.0``.

    """
except (ImportError, StopIteration):
    def _cache_hash(template: bytes) -> bytes:
        """Get template itself as hash fallback."""
        return template

__author__ = 'Felipe A Hernandez'
__email__ = 'ergoithz@gmail.com'
__license__ = 'MIT'
__version__ = '0.1.5'
__all__ = (
    # api
    'tokenize',
    'stream',
    'render',
    'cli',
    # exceptions
    'TokenException',
    'ClosingTokenException',
    'UnclosedTokenException',
    'DelimiterTokenException',
    # defaults
    'default_resolver',
    'default_getter',
    'default_stringify',
    'default_escape',
    'default_lambda_render',
    'default_tags',
    'default_cache',
    'default_virtuals',
    # types
    'TString',
    'PartialResolver',
    'PropertyGetter',
    'StringifyFunction',
    'EscapeFunction',
    'LambdaRenderFunctionFactory',
    'LambdaRenderFunctionConstructor',
    'CompiledTemplate',
    'CompiledToken',
    'CompiledTemplateCache',
    'TagsTuple',
)

if sys.version_info > (3, 9):
    _abc = collections.abc
    _TagsTuple = tuple[typing.AnyStr, typing.AnyStr]
    _TagsByteTuple = tuple[bytes, bytes]
    _CompiledToken = tuple[bool, bool, bool, slice, slice, int]
    _CompiledTemplate = tuple[_CompiledToken, ...]
    _CacheKey = tuple[typing.Union[bytes, int], bytes, bytes, bool]
    _TokenizeStack = list[tuple[slice, int, int, int]]
    _TokenizeRecording = list[_CompiledToken]
    _SiblingIterator = typing.Optional[_abc.Iterator]
    _ProcessStack = list[tuple[_SiblingIterator, bool, bool]]
else:
    _abc = typing
    _TagsTuple = typing.Tuple[typing.AnyStr, typing.AnyStr]
    _TagsByteTuple = typing.Tuple[bytes, bytes]
    _CompiledToken = typing.Tuple[bool, bool, bool, slice, slice, int]
    _CompiledTemplate = typing.Tuple[_CompiledToken, ...]
    _CacheKey = typing.Tuple[typing.Union[bytes, int], bytes, bytes, bool]
    _TokenizeStack = typing.List[typing.Tuple[slice, int, int, int]]
    _TokenizeRecording = typing.List[_CompiledToken]
    _SiblingIterator = typing.Optional[_abc.Iterator]
    _ProcessStack = typing.List[typing.Tuple[_SiblingIterator, bool, bool]]

T = typing.TypeVar('T')
"""Generic."""

D = typing.TypeVar('D')
"""Generic."""

TString = typing.TypeVar('TString', str, bytes)
"""String/bytes generic."""

PartialResolver = _abc.Callable[[typing.AnyStr], typing.AnyStr]
"""Template partial tag resolver function interface."""

PropertyGetter = _abc.Callable[
    [typing.Any, _abc.Sequence[typing.Any], typing.AnyStr, typing.Any],
    typing.Any,
]
"""Template property getter function interface."""

StringifyFunction = _abc.Callable[[bytes, bool], bytes]
"""Template variable general stringification function interface."""

EscapeFunction = _abc.Callable[[bytes], bytes]
"""Template variable value escape function interface."""

LambdaRenderFunctionConstructor = _abc.Callable[
    ...,
    _abc.Callable[..., typing.AnyStr],
]
"""Lambda render function constructor interface."""

LambdaRenderFunctionFactory = LambdaRenderFunctionConstructor
"""
.. deprecated:: 0.1.3
    Use :attr:`LambdaRenderFunctionConstructor` instead.

"""

VirtualPropertyFunction = _abc.Callable[[typing.Any], typing.Any]
"""Virtual property implementation callable interface."""

VirtualPropertyMapping = _abc.Mapping[str, VirtualPropertyFunction]
"""Virtual property mapping interface."""

TagsTuple = _TagsTuple
"""Mustache tag tuple interface."""

CompiledToken = _CompiledToken
"""
Compiled template token.

Tokens are tuples containing a renderer decission path, key, content and flags.

``type: bool``
    Decission for rendering path node `a`.

``type: bool``
    Decission for rendering path node `b`.

``type: bool``
    Decission for rendering path node `c`

``key: Optional[slice]``
    Template slice for token scope key, if any.

``content: Optional[slice]``
    Template slice for token content data, if any.

``flags: int``
    Token flags.

    - Unused: ``-1`` (default)
    - Variable flags:
        - ``0`` - escaped
        - ``1`` - unescaped
    - Block start flags:
        - ``0`` - falsy
        - ``1`` - truthy
    - Block end value: block content index.

"""

CompiledTemplate = _CompiledTemplate
"""
Compiled template interface.

.. seealso::

    :py:attr:`ustache.CompiledToken`
        Item type.

    :py:attr:`ustache.CompiledTemplateCache`
        Interface exposing this type.

"""

CompiledTemplateCache = _abc.MutableMapping[_CacheKey, CompiledTemplate]
"""
Cache mapping interface.

.. seealso::

    :py:attr:`ustache.CompiledTemplateCache`
        Item type.

"""


class LRUCache(collections.OrderedDict, typing.Generic[T]):
    """Capped mapping discarding least recently used elements."""

    def __init__(self, maxsize: int, *args, **kwargs) -> None:
        """
        Initialize.

        :param maxsize: maximum number of elements will be kept

        Any parameter excess will be passed straight to dict constructor.

        """
        self.maxsize = maxsize
        super().__init__(*args, **kwargs)

    def get(self, key: _abc.Hashable, default: D = None) -> typing.Union[T, D]:
        """
        Get value for given key or default if not present.

        :param key: hashable
        :param default: value will be returned if key is not present
        :returns: value if key is present, default if not

        """
        try:
            return self[key]
        except KeyError:
            return default  # type: ignore

    def __getitem__(self, key: _abc.Hashable) -> T:
        """
        Get value for given key.

        :param key: hashable
        :returns: value if key is present
        :raises KeyError: if key is not present

        """
        self.move_to_end(key)
        return super().__getitem__(key)

    def __setitem__(self, key: _abc.Hashable, value: T) -> None:
        """
        Set value for given key.

        :param key: hashable will be used to retrieve values later on
        :param value: value for given key

        """
        super().__setitem__(key, value)
        try:
            self.move_to_end(key)
            while len(self) > self.maxsize:
                self.popitem(last=False)
        except KeyError:  # race condition
            pass


default_tags = (b'{{', b'}}')
"""Tuple of default mustache tags (in bytes)."""

default_cache: CompiledTemplateCache = LRUCache(1024)
"""
Default template cache mapping, keeping the 1024 most recently used
compiled templates (LRU expiration).

If `xxhash`_ is available, template data won't be included in cache.

.. _xxhash: https://pypi.org/project/xxhash/

"""


def virtual_length(ref: typing.Any) -> int:
    """
    Resolve virtual length property.

    :param ref: any non-mapping object implementing `__len__`
    :returns: number of items
    :raises TypeError: if ref is mapping or has no `__len__` method

    """
    if isinstance(ref, collections.abc.Mapping):
        raise TypeError
    return len(ref)


default_virtuals: VirtualPropertyMapping = types.MappingProxyType({
    'length': virtual_length,
})
"""
Immutable mapping with default virtual properties.

The following virtual properties are implemented:

- **length**, for non-mapping sized objects, returning ``len(ref)``.

"""

TOKEN_TYPES = [(True, False, True, False)] * 0x100
TOKEN_TYPES[0x21] = False, False, False, True  # '!'
TOKEN_TYPES[0x23] = False, True, True, False  # '#'
TOKEN_TYPES[0x26] = True, True, True, True  # '&'
TOKEN_TYPES[0x2F] = False, True, False, False  # '/'
TOKEN_TYPES[0x3D] = False, False, False, False  # '='
TOKEN_TYPES[0x3E] = False, False, True, False  # '>'
TOKEN_TYPES[0x5E] = False, True, True, True  # '^'
TOKEN_TYPES[0x7B] = True, True, False, True  # '{'
"""ASCII-indexed tokenizer decission matrix."""

FALSY_PRIMITIVES = None, False, 0, float('nan'), float('-nan')
EMPTY = slice(0)
EVERYTHING = slice(None)


class TokenException(SyntaxError):
    """Invalid token found during tokenization."""

    message = 'Invalid tag {tag} at line {row} column {column}'

    @classmethod
    def from_template(
            cls,
            template: bytes,
            start: int,
            end: int,
    ) -> 'TokenException':
        """
        Create exception instance from parsing data.

        :param template: template bytes
        :param start: character position where the offending tag starts at
        :param end: character position where the offending tag ends at
        :returns: exception instance

        """
        tag = template[start:end].decode()
        row = 1 + template[:start].count(b'\n')
        column = 1 + start - max(0, template.rfind(b'\n', 0, start))
        return cls(cls.message.format(tag=tag, row=row, column=column))


class ClosingTokenException(TokenException):
    """Non-matching closing token found during tokenization."""

    message = 'Non-matching tag {tag} at line {row} column {column}'


class UnclosedTokenException(ClosingTokenException):
    """Unclosed token found during tokenization."""

    message = 'Unclosed tag {tag} at line {row} column {column}'


class DelimiterTokenException(TokenException):
    """
    Invalid delimiters token found during tokenization.

    .. versionadded:: 0.1.1

    """

    message = 'Invalid delimiters {tag} at line {row} column {column}'


def default_stringify(
        data: typing.Any,
        text: bool = False,
) -> bytes:
    """
    Convert arbitrary data to bytes.

    :param data: value will be serialized
    :param text: whether running in text mode or not (bytes mode)
    :returns: template bytes

    """
    return (
        data
        if isinstance(data, bytes) and not text else
        data.encode()
        if isinstance(data, str) else
        f'{data}'.encode()
    )


def replay(
        recording: typing.Sequence[CompiledToken],
        start: int = 0,
) -> _abc.Generator[CompiledToken, int, None]:
    """
    Yield template tokenization from cached data.

    This generator accepts sending back a token index, which will result on
    rewinding it back and repeat everything from there.

    :param recording: token list
    :param start: starting index
    :returns: token tuple generator

    """
    size = len(recording)
    while True:
        for item in range(start, size):
            start = yield recording[item]
            if start is not None:
                break
        else:
            break


def default_escape(data: bytes) -> bytes:
    """
    Convert bytes conflicting with HTML to their escape sequences.

    :param data: bytes containing text
    :returns: escaped text bytes

    """
    return (
        data
        .replace(b'&', b'&amp;')
        .replace(b'<', b'&lt;')
        .replace(b'>', b'&gt;')
        .replace(b'"', b'&quot;')
        .replace(b'\'', b'&#x60;')
        .replace(b'`', b'&#x3D;')
    )


def default_resolver(name: typing.AnyStr) -> bytes:
    """
    Mustache partial resolver function (stub).

    :param name: partial template name
    :returns: empty bytes

    """
    return b''


def default_getter(
        scope: typing.Any,
        scopes: _abc.Sequence[typing.Any],
        key: typing.AnyStr,
        default: typing.Any = None,
        *,
        virtuals: VirtualPropertyMapping = default_virtuals,
) -> typing.Any:
    """
    Extract property value from scope hierarchy.

    :param scope: uppermost scope (corresponding to key ``'.'``)
    :param scopes: parent scope sequence
    :param key: property key
    :param default: value will be used as default when missing
    :param virtuals: mapping of virtual property callables
    :return: value from scope or default

    .. versionadded:: 0.1.3
       *virtuals* parameter.

    Both :class:`AttributeError` and :class:`TypeError` exceptions
    raised by virtual property implementations will be handled as if
    that property doesn't exist, which can be useful to filter out
    incompatible types.

    """
    if key in (b'.', '.'):
        return scope

    binary_mode = not isinstance(key, str)
    components = key.split(b'.' if binary_mode else '.')  # type: ignore
    for ref in (*scopes, scope)[::-1]:
        for name in components:
            if binary_mode:
                try:
                    ref = ref[name]
                    continue
                except (KeyError, TypeError, AttributeError):
                    pass
                try:
                    name = name.decode()  # type: ignore
                except UnicodeDecodeError:
                    break
            try:
                ref = ref[name]
                continue
            except (KeyError, TypeError, AttributeError):
                pass
            try:
                ref = (
                    ref[int(name)]
                    if name.isdigit() else
                    getattr(ref, name)  # type: ignore
                )
                continue
            except (KeyError, TypeError, AttributeError, IndexError):
                pass
            try:
                ref = virtuals[name](ref)  # type: ignore
                continue
            except (KeyError, TypeError, AttributeError):
                pass
            break
        else:
            return ref
    return default


def default_lambda_render(
        scope: typing.Any,
        **kwargs,
) -> _abc.Callable[[TString], TString]:
    r"""
    Generate a template-only render function with fixed parameters.

    :param scope: current scope
    :param \**kwargs: parameters forwarded to :func:`render`
    :returns: template render function

    """

    def lambda_render(template: TString) -> TString:
        """
        Render given template to string/bytes.

        :param template: template text
        :returns: rendered string or bytes (depending on template type)

        """
        return render(template, scope, **kwargs)

    return lambda_render


def slicestrip(template: bytes, start: int, end: int) -> slice:
    """
    Strip slice from whitespace on bytes.

    :param template: bytes where whitespace should be stripped
    :param start: substring slice start
    :param end: substring slice end
    :returns: resulting stripped slice

    """
    c = template[start:end]
    return slice(end - len(c.lstrip()), start + len(c.rstrip()))


def tokenize(
        template: bytes,
        *,
        tags: TagsTuple = default_tags,
        comments: bool = False,
        cache: CompiledTemplateCache = default_cache,
) -> _abc.Generator[CompiledToken, int, None]:
    """
    Generate token tuples from mustache template.

    This generator accepts sending back a token index, which will result on
    rewinding it back and repeat everything from there.

    :param template: template as utf-8 encoded bytes
    :param tags: mustache tag tuple (open, close)
    :param comments: whether yield comment tokens or not (ignore comments)
    :param cache: mutable mapping for compiled template cache
    :return: token tuple generator (type, name slice, content slice, option)

    :raises UnclosedTokenException: if token is left unclosed
    :raises ClosingTokenException: if block closing token does not match
    :raises DelimiterTokenException: if delimiter token syntax is invalid

    """
    tokenization_key = _cache_hash(template), *tags, comments  # type: ignore

    cached = cache.get(tokenization_key)
    if cached:  # recordings must contain at least one token
        yield from replay(cached)
        return

    template_find = template.find

    stack: _TokenizeStack = []
    stack_append = stack.append
    stack_pop = stack.pop
    scope_label = EVERYTHING
    scope_head = 0
    scope_start = 0
    scope_index = 0

    empty = EMPTY
    start_tag, end_tag = tags
    end_literal = b'}' + end_tag
    end_switch = b'=' + end_tag
    start_len = len(start_tag)
    end_len = len(end_tag)

    token: CompiledToken
    token_types = TOKEN_TYPES
    t = slice
    s = functools.partial(slicestrip, template)
    recording: _TokenizeRecording = []
    record = recording.append

    text_start, text_end = 0, template_find(start_tag)
    while text_end != -1:
        if text_start < text_end:  # text
            token = False, True, False, empty, t(text_start, text_end), -1
            record(token)
            yield token

        tag_start = text_end + start_len
        try:
            a, b, c, d = token_types[template[tag_start]]
        except IndexError:
            raise UnclosedTokenException.from_template(
                template=template,
                start=text_end,
                end=tag_start,
            ) from None

        if a:  # variables
            tag_start += b

            if c:  # variable
                tag_end = template_find(end_tag, tag_start)
                text_start = tag_end + end_len

            else:  # triple-keyed variable
                tag_end = template_find(end_literal, tag_start)
                text_start = tag_end + end_len + 1

            token = False, True, True, s(tag_start, tag_end), empty, d

        elif b:  # block
            tag_start += 1
            tag_end = template_find(end_tag, tag_start)
            text_start = tag_end + end_len

            if c:  # open
                stack_append((scope_label, text_end, scope_start, scope_index))
                scope_label = s(tag_start, tag_end)
                scope_head = text_end
                scope_start = text_start
                scope_index = len(recording)
                token = True, True, False, scope_label, empty, d

            elif template[scope_label] != template[tag_start:tag_end].strip():
                raise ClosingTokenException.from_template(
                    template=template,
                    start=text_end,
                    end=text_start,
                )

            else:  # close
                token = (
                    True, True, True,
                    scope_label, s(scope_start, text_end), scope_index,
                )
                scope_label, scope_head, scope_start, scope_index = stack_pop()

        elif c:  # partial
            tag_start += 1
            tag_end = template_find(end_tag, tag_start)
            text_start = tag_end + end_len
            token = True, False, True, s(tag_start, tag_end), empty, -1

        elif d:  # comment
            tag_start += 1
            tag_end = template_find(end_tag, tag_start)
            text_start = tag_end + end_len

            if not comments:
                text_end = template_find(start_tag, text_start)
                continue

            token = True, False, False, empty, s(tag_start, tag_end), -1

        else:  # tags
            tag_start += 1
            tag_end = template_find(end_switch, tag_start)
            text_start = tag_end + end_len + 1

            try:
                start_tag, end_tag = template[tag_start:tag_end].split(b' ')
                if not (start_tag and end_tag):
                    raise ValueError

            except ValueError:
                raise DelimiterTokenException.from_template(
                    template=template,
                    start=text_end,
                    end=text_start,
                ) from None

            end_literal = b'}' + end_tag
            end_switch = b'=' + end_tag
            start_len = len(start_tag)
            end_len = len(end_tag)
            start_end = tag_start + start_len
            end_start = tag_end - end_len
            token = (
                False, False, True,
                s(tag_start, start_end), s(end_start, tag_end), -1,
            )

        if tag_end < 0:
            raise UnclosedTokenException.from_template(
                template=template,
                start=tag_start,
                end=tag_end,
            )

        record(token)
        rewind = yield token
        if rewind is not None:
            yield from replay(recording, rewind)

        text_end = template_find(start_tag, text_start)

    if stack:
        raise UnclosedTokenException.from_template(
            template=template,
            start=scope_head,
            end=scope_start,
        )

    # end
    token = False, False, False, empty, t(text_start, None), -1
    cache[tokenization_key] = (*recording, token)
    yield token


def process(
        template: TString,
        scope: typing.Any,
        *,
        scopes: _abc.Iterable[typing.Any] = (),
        resolver: PartialResolver = default_resolver,
        getter: PropertyGetter = default_getter,
        stringify: StringifyFunction = default_stringify,
        escape: EscapeFunction = default_escape,
        lambda_render: LambdaRenderFunctionConstructor = default_lambda_render,
        tags: TagsTuple = default_tags,
        cache: CompiledTemplateCache = default_cache,
) -> _abc.Generator[bytes, int, None]:
    """
    Generate rendered mustache template byte chunks.

    :param template: mustache template string
    :param scope: root object used as root mustache scope
    :param scopes: iterable of parent scopes
    :param resolver: callable will be used to resolve partials (bytes)
    :param getter: callable will be used to pick variables from scope
    :param stringify: callable will be used to render python types (bytes)
    :param escape: callable will be used to escape template (bytes)
    :param lambda_render: lambda render function constructor
    :param tags: mustache tag tuple (open, close)
    :param cache: mutable mapping for compiled template cache
    :return: byte chunk generator

    :raises UnclosedTokenException: if token is left unclosed
    :raises ClosingTokenException: if block closing token does not match
    :raises DelimiterTokenException: if delimiter token syntax is invalid

    """
    text_mode = isinstance(template, str)

    # encoding
    data: bytes = (
        template.encode()  # type: ignore
        if text_mode else
        template
    )
    current_tags: _TagsByteTuple = tuple(  # type: ignore
        tag.encode() if isinstance(tag, str) else tag
        for tag in tags[:2]
    )

    # current context
    siblings: _SiblingIterator = None
    callback = False
    silent = False

    # context stack
    stack: _ProcessStack = []
    stack_append = stack.append
    stack_pop = stack.pop

    # scope stack
    scopes = list(scopes)
    scopes_append = scopes.append
    scopes_pop = scopes.pop

    # locals
    read = data.__getitem__
    decode = (
        (lambda x: data[x].decode())  # type: ignore
        if text_mode else
        read
    )
    missing = object()
    falsy_primitives = FALSY_PRIMITIVES
    TIterable = collections.abc.Iterable
    TNonLooping = (
        str,
        bytes,
        collections.abc.Mapping,
    )
    TSequence = collections.abc.Sequence

    tokens = tokenize(data, tags=current_tags, cache=cache)
    rewind = tokens.send
    for a, b, c, token_name, token_content, token_option in tokens:
        if silent:
            if a and b:
                if c:  # closing/loop
                    closing_scope = scope
                    closing_callback = callback

                    scope = scopes_pop()
                    siblings, callback, silent = stack_pop()

                    if closing_callback and not silent:
                        yield stringify(
                            closing_scope(
                                decode(token_content),
                                lambda_render(
                                    scope=scope,
                                    scopes=scopes,
                                    resolver=resolver,
                                    escape=escape,
                                    tags=current_tags,
                                    cache=cache,
                                ),
                            ),
                            text_mode,
                        )

                else:  # block
                    scopes_append(scope)
                    stack_append((siblings, callback, silent))
        elif a:
            if b:
                if c:  # closing/loop
                    if siblings:
                        try:
                            scope = next(siblings)
                            callback = callable(scope)
                            silent = callback
                            rewind(token_option)
                        except StopIteration:
                            scope = scopes_pop()
                            siblings, callback, silent = stack_pop()
                    else:
                        scope = scopes_pop()
                        siblings, callback, silent = stack_pop()

                else:  # block
                    scopes_append(scope)
                    stack_append((siblings, callback, silent))

                    scope = getter(scope, scopes, decode(token_name), None)
                    falsy = (  # emulate JS falseness
                            scope in falsy_primitives
                            or isinstance(scope, TSequence) and not scope
                    )
                    if token_option:  # falsy block
                        siblings = None
                        callback = False
                        silent = not falsy
                    elif falsy:  # truthy block with falsy value
                        siblings = None
                        callback = False
                        silent = True
                    elif (
                            isinstance(scope, TIterable)
                            and not isinstance(scope, TNonLooping)
                    ):  # loop block
                        try:
                            siblings = iter(scope)
                            scope = next(siblings)
                            callback = callable(scope)
                            silent = callback
                        except StopIteration:
                            siblings = None
                            scope = None
                            callback = False
                            silent = True
                    else:  # truthy block with truthy value
                        siblings = None
                        callback = callable(scope)
                        silent = callback

            elif c:  # partial
                value = resolver(decode(token_name))
                if value:
                    yield from process(
                        template=value,
                        scope=scope,
                        scopes=scopes,
                        resolver=resolver,
                        escape=escape,
                        tags=current_tags,
                        cache=cache,
                    )
            # else: comment
        elif b:
            if c:  # variable
                value = getter(scope, scopes, decode(token_name), missing)
                if value is not missing:
                    yield (
                        stringify(value, text_mode)
                        if token_option else
                        escape(stringify(value, text_mode))
                    )

            else:  # text
                yield read(token_content)

        elif c:  # tags
            current_tags = read(token_name), read(token_content)

        else:  # end
            yield read(token_content)


def stream(
        template: TString,
        scope: typing.Any,
        *,
        scopes: _abc.Iterable[typing.Any] = (),
        resolver: PartialResolver = default_resolver,
        getter: PropertyGetter = default_getter,
        stringify: StringifyFunction = default_stringify,
        escape: EscapeFunction = default_escape,
        lambda_render: LambdaRenderFunctionConstructor = default_lambda_render,
        tags: TagsTuple = default_tags,
        cache: CompiledTemplateCache = default_cache,
) -> _abc.Generator[TString, None, None]:
    """
    Generate rendered mustache template chunks.

    :param template: mustache template (str or bytes)
    :param scope: current rendering scope (data object)
    :param scopes: list of precedent scopes
    :param resolver: callable will be used to resolve partials (bytes)
    :param getter: callable will be used to pick variables from scope
    :param stringify: callable will be used to render python types (bytes)
    :param escape: callable will be used to escape template (bytes)
    :param lambda_render: lambda render function constructor
    :param tags: tuple (start, end) specifying the initial mustache delimiters
    :param cache: mutable mapping for compiled template cache
    :returns: generator of bytes/str chunks (same type as template)

    :raises UnclosedTokenException: if token is left unclosed
    :raises ClosingTokenException: if block closing token does not match
    :raises DelimiterTokenException: if delimiter token syntax is invalid

    """
    chunks = process(
        template=template,
        scope=scope,
        scopes=scopes,
        resolver=resolver,
        getter=getter,
        stringify=stringify,
        escape=escape,
        lambda_render=lambda_render,
        tags=tags,
        cache=cache,
    )
    yield from (
        codecs.iterdecode(chunks, 'utf8')
        if isinstance(template, str) else
        chunks
    )


def render(
        template: TString,
        scope: typing.Any,
        *,
        scopes: _abc.Iterable[typing.Any] = (),
        resolver: PartialResolver = default_resolver,
        getter: PropertyGetter = default_getter,
        stringify: StringifyFunction = default_stringify,
        escape: EscapeFunction = default_escape,
        lambda_render: LambdaRenderFunctionConstructor = default_lambda_render,
        tags: TagsTuple = default_tags,
        cache: CompiledTemplateCache = default_cache,
) -> TString:
    """
    Render mustache template.

    :param template: mustache template
    :param scope: current rendering scope (data object)
    :param scopes: list of precedent scopes
    :param resolver: callable will be used to resolve partials (bytes)
    :param getter: callable will be used to pick variables from scope
    :param stringify: callable will be used to render python types (bytes)
    :param escape: callable will be used to escape template (bytes)
    :param lambda_render: lambda render function constructor
    :param tags: tuple (start, end) specifying the initial mustache delimiters
    :param cache: mutable mapping for compiled template cache
    :returns: rendered bytes/str (type depends on template)

    :raises UnclosedTokenException: if token is left unclosed
    :raises ClosingTokenException: if block closing token does not match
    :raises DelimiterTokenException: if delimiter token syntax is invalid

    """
    data = b''.join(process(
        template=template,
        scope=scope,
        scopes=scopes,
        resolver=resolver,
        getter=getter,
        stringify=stringify,
        escape=escape,
        lambda_render=lambda_render,
        tags=tags,
        cache=cache,
    ))
    return data.decode() if isinstance(template, str) else data


def cli(argv: typing.Optional[_abc.Sequence[str]] = None) -> None:
    """
    Render template from command line.

    Use `python -m ustache --help` to check available options.

    :param argv: command line arguments, :attr:`sys.argv` when None

    """
    import argparse
    import json
    import sys

    arguments = argparse.ArgumentParser(
        description='Render mustache template.',
    )
    arguments.add_argument(
        'template',
        metavar='PATH',
        type=argparse.FileType('r'),
        help='template file',
    )
    arguments.add_argument(
        '-j', '--json',
        metavar='PATH',
        type=argparse.FileType('r'),
        default=sys.stdin,
        help='JSON file, default: stdin',
    )
    arguments.add_argument(
        '-o', '--output',
        metavar='PATH',
        type=argparse.FileType('w'),
        default=sys.stdout,
        help='output file, default: stdout',
    )
    args = arguments.parse_args(argv)
    try:
        args.output.write(render(args.template.read(), json.load(args.json)))
    finally:
        args.template.close()
        if args.json is not sys.stdin:
            args.json.close()
        if args.output is not sys.stdout:
            args.output.close()


if __name__ == '__main__':
    cli()
