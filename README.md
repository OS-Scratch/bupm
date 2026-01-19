# bupm - the **b**inary **u**pdate and **p**ackage **m**anager
## Installing
bupm uses the meson / ninja build system. 
It depends on yaml-cpp and libcurl (on most package managers, install the package with dev or devel suffix), and also make sure to have a C compiler in $PATH.
If it doesn't find the libraries, try running ```export PKG_CONFIG_PATH=/usr/lib64/pkgconfig```
To install:
```bash
git clone https://github.com/OS-Scratch/bupm.git
cd bupm 
mkdir build
cd build
meson setup ..
ninja
sudo ninja install
```
## Usage
bupm 1.2 introduced install and repository capabilities.
