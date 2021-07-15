# PineConnect
Companion daemon and app for the PineTime watch

## Build

### Install Dependencies

Debian and Ubuntu:
```
sudo apt-get install build-essential cmake git libbluetooth-dev libreadline-dev`
```

Arch:
```
sudo pacman -S git cmake make pkgconfig`
```

### Clone the Repository

Clone the repo and load the [gattlib](https://github.com/labapart/gattlib) submodule:
```
git clone https://github.com/timaios/PineConnect.git
cd PineConnect
git submodule update --init
```

### Build [gattlib](https://github.com/labapart/gattlib) Submodule

```
cd lib/gattlib
mkdir build
cd build
cmake -DGATTLIB_BUILD_DOCS=OFF ..
make
cd ../../..
```

### Build PineConnect

```
make clean
make
```

### Test-run the Daemon Process

```
cd build/daemon
LD_LIBRARY_PATH=../../lib/gattlib/build/dbus ./pc-daemon
```

