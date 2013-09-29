Minecraft++
===========

Modular, multi-threaded, event-driven Minecraft server written in C++.

Very early development.

Screenshots
-----------

Terrain generation:

![Mountain](http://i.imgur.com/YeUTAql.jpg)

![Mountaintop](http://i.imgur.com/hs3sG2r.jpg)

![Distant mountains](http://i.imgur.com/qQxgz3z.jpg)

Interactive (i.e. non-service/-daemon) front-end:

![Interactive front-end](http://i.imgur.com/9WHZwFB.png)

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

GCC 4.8.0 is used to build the server on Windows, GCC 4.8.1 on Linux.

RLeahyLib is hardcoded not to build on anything but GCC 4.8.0 or higher, but this is so the type system can depend on certain GCC-specific macros, and can likely be quickly adapted to suit a different compiler.

Neither Minecraft++ or RLeahyLib will build on anything that doesn't support C++11.

They definitely do not build on VC++.