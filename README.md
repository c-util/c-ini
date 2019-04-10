c-ini
=====

Ini-File Handling

The c-ini project implements APIs to deal with ini-files. Different formats can
be supported, but all share common ini-file properties, mainly that they are
human-readable, grouped key-value pairs. For API documentation, see the public
header files, as well as the docbook comments for each function.

### Project

 * **Website**: <https://c-util.github.io/c-ini>
 * **Bug Tracker**: <https://github.com/c-util/c-init/issues>

### Requirements

The requirements for this project are:

 * `libc` (e.g., `glibc >= 2.16`)

At build-time, the following software is required:

 * `meson >= 0.41`
 * `pkg-config >= 0.29`

### Build

The meson build-system is used for this project. Contact upstream
documentation for detailed help. In most situations the following
commands are sufficient to build and install from source:

```sh
mkdir build
cd build
meson setup ..
ninja
meson test
ninja install
```

No custom configuration options are available.

### Repository:

 - **web**:   <https://github.com/c-util/c-ini>
 - **https**: `https://github.com/c-util/c-ini.git`
 - **ssh**:   `git@github.com:c-util/c-ini.git`

### License:

 - **Apache-2.0** OR **LGPL-2.1-or-later**
 - See AUTHORS file for details.
