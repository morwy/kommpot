# kommpot

## Description

**kommpot** is a C++ library providing a small unified interface to multiple transfer protocols.  
Interface consists of very basic functions - **devices()** / **open()** / **close()** / **read()** / **write()** and similar.
A **native_handle()** function is available together with original SDK headers in case of certain fine-tuning required for some specific protocol.

### Prerequisites

#### Linux (Ubuntu / Debian)

Install required dependencies:

```bash
sudo apt install cmake ninja-build libudev-dev -y
```

#### MacOS

Install required dependencies:

```bash
brew install cmake ninja
```

#### Windows

Install required dependencies:

1. Download **Visual Studio 2022 Community** installer and start it.
2. Select **Desktop development with C++** option.
3. Continue with default selection (or anything else needed, e.g. Clang compiler).

## Dependencies

| Dependency | Version |
| --- | --- |
| `libftdi` | [v1.5](https://www.intra2net.com/en/developer/libftdi/download/libftdi1-1.5.tar.bz2) |
| `libusb-cmake` | [v1.0.27 - 8782527](https://github.com/libusb/libusb-cmake/commit/8782527de86be37e9dd5588e35832d01b411e09a) |
| `cpp-httplib` | [v0.35.0 - 85b18a9](https://github.com/yhirose/cpp-httplib/commit/85b18a9c6431877188a706ba3c61679fa9de3e27) |

## Credits

* [googletest](https://github.com/google/googletest) - used as source code, non-modified, originally distributed under BSD-3-Clause license.
* [libftdi](https://www.intra2net.com/en/developer/libftdi/index.php) - used as source code, non-modified (some build options were modified out to ease building process), originally distributed under LGPL-2.1 license.
* [libusb-cmake](https://github.com/libusb/libusb-cmake) - used as source code, non-modified (some build options were modified out to ease building process), originally distributed under LGPL-2.1 license.
* [cpp-httplib](https://github.com/yhirose/cpp-httplib) - used as source code, non-modified, originally distributed under MIT license.
* [Elise Navennec - function export implementation](https://atomheartother.github.io/c++/2018/07/12/CPPDynLib.html) - used as source code, non-modified, originally distributed under MIT license.
