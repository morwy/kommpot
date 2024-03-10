# kommpot

## Description

### Prerequisites

#### Linux (Ubuntu / Debian)

Install required dependencies:

```bash
sudo apt install cmake ninja-build -y
```

## Dependencies

| Dependency | Version |
| --- | --- |
| `libftdi` | [v1.5](https://www.intra2net.com/en/developer/libftdi/download/libftdi1-1.5.tar.bz2) |
| `libusb-cmake` | [v1.0.27 - 8782527](https://github.com/libusb/libusb-cmake/commit/8782527de86be37e9dd5588e35832d01b411e09a) |

## Credits

* [libftdi](https://www.intra2net.com/en/developer/libftdi/index.php) - non-modified (some build options were commented out to ease building process), originally distributed under LGPL-2.1 license.
* [libusb-cmake](https://github.com/libusb/libusb-cmake) - non-modified, originally distributed under LGPL-2.1 license.
* [Elise Navennec - function export implementation](https://atomheartother.github.io/c++/2018/07/12/CPPDynLib.html) - used as source code, non-modified, originally distributed under MIT license.
