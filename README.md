Minecraft++
===========

Modular, multi-threaded, event-driven Minecraft server written in C++.

Very early development.

Dependencies
------------

- [cURL](http://curl.haxx.se/)
- [MySQL](http://dev.mysql.com/doc/refman/5.6/en/c-api.html)
- [OpenSSL](http://www.openssl.org/)
- [RLeahyLib](https://github.com/RobertLeahy/RLeahyLib)
- [zlib](http://zlib.net/)

Operating System Support
------------------------

As all the above libraries are available for Windows and Linux (zlib has to be custom-built from source for Windows x64), the server will support Linux and Windows equally.

Compiler Support
----------------

GCC 4.8.0 is the compiler currently used to build the server.

RLeahyLib is hardcoded not to build on anything except GCC 4.8.0 or higher, but this restriction is so the type system can depend on certain GCC-specific macros, and can likely be quickly adapted to suit Clang.

Neither Minecraft++ or RLeahyLib will build on anything that doesn't support C++11.

It definitely does not build on VC++.