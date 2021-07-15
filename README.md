# PineConnect
Companion daemon and app for the PineTime watch

## Build

### Install Dependencies

Debian and Ubuntu:
`sudo apt-get install build-essential cmake git libbluetooth-dev libreadline-dev`

Arch:
`sudo pacman -S git cmake make pkgconfig`

### Clone the Repository

Clone the repo and load the [gattlib](https://github.com/labapart/gattlib) submodule:
```
git clone https://github.com/timaios/PineConnect.git
cd PineConnect
git submodule update --init
```

### Build gattlib Submodule

```
cd lib/gattlib
mkdir build
cd build
cmake -DGATTLIB_BUILD_DOCS=OFF ..
make
cd ../../..
```

### Build the PineConnect Daemon

```
mkdir build
g++ -g -Wall -Wpedantic -Ilib/gattlib/include -Llib/gattlib/build/dbus -o build/daemon src/daemon/main.cc -lgattlib
```

### Run the Daemon Process

```
cd build
export LD_LIBRARY_PATH=$(pwd)/../lib/gattlib/build/dbus
./daemon
```

