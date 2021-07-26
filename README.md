# PineConnect

Companion daemon and app for the PineTime watch

The goal of this project is to provide companion software for the PineTime (running the
InfiniTime firmware) which uses as few third party libraries as possible to allow for
easy compilation on the different Linux flavors used on the PinePhone.

## State of Affairs

This project is at an early stage. The daemon is currently being developed, but no work has
been done on the app so far.

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

