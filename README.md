# PineConnect
Companion daemon and app for the PineTime watch

## Build

### Install Dependencies

You will need a basic development environment with `g++` and `make` installed.
For the libraries, `pkg-config` and the `libdbus` header files are needed.

Debian and Ubuntu:
```
sudo apt-get install build-essential pkgconf git libdbus-1-dev`
```

### Clone the Repository

Clone the repo:
```
git clone https://github.com/timaios/PineConnect.git
```

### Build PineConnect

```
make clean
make
```

### Test-run the Daemon Process

```
cd build/daemon
./pineconnectd
```

