# midi-2-relay

![Language](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![License](https://img.shields.io/badge/license-Unknown-inactive)

Translate MIDI note events into UDP relay commands.

## What It Does
- Reads raw MIDI bytes from a serial port.
- Maps MIDI note/channel to relay outputs using a map file.
- Sends `set <hex>` commands over UDP to a relay controller.
- Periodically reloads the map file and sends `ping` keepalives.

## Build
Linux/Unix:
```sh
cd src
make
```

Windows:
- Open `win32/midi.sln` in Visual Studio and build the `midi` project.

## Run
```sh
./midi2relay -v
```

Common options:
- `-v` increase verbosity (repeatable).
- `-d <pidfile>` run as a daemon (Unix).
- `-f <config_file>` reserved (not currently used).
- `-t <ip:port>` target relay host/port (see notes below).
- `-s <device>` serial device path (default `/dev/ttyAMA0`).
- `-u <port>` UDP listen port (default `0`, disabled).

## Map File
Default path in code:
- `/var/www/html/uploads/map.txt`

Format (one mapping per line):
```
<note> <channel> <relay>
```

Valid ranges:
- `note`: 0-127
- `channel`: 0-15
 - `relay`: 0-127

Lines starting with `#` are comments.

## Default Behavior
- Serial port: `/dev/ttyAMA0` at 38400 8N1 raw.
- Target UDP host: `127.0.0.1`
- Target UDP port: `1027`

## Known Issues
- None currently tracked. See `TODO.md` for open work.

## Documentation
- Architecture: `docs/architecture.md`
- Diagrams: `docs/midi.pdf`, `docs/midi-flow.jpg`, `docs/midi-processing.jpg`

## Project Layout
- `src/` core application and utilities
- `test/` manual serial read test
- `win32/` Visual Studio solution
- `docs/` supporting documents

## Install
Linux/Unix (from source):
1. Build: `cd src && make`
2. Move the binary into your PATH, for example:
   - `sudo cp midi2relay /usr/local/bin/`

Windows:
1. Open `win32/midi.sln` in Visual Studio.
2. Build the `midi` project to produce the executable.

## Troubleshooting
- Serial port open fails:
  - Confirm the device exists: `/dev/ttyAMA0` (or update in code).
  - Check permissions and that no other process is using the device.
- No relay updates:
  - Verify the map file path and contents.
  - Increase verbosity with `-v` and look for load/parse output.
  - Ensure the relay target UDP host/port matches your controller.
- Map file changes not taking effect:
  - The file is only reloaded when mtime changes and checked ~every 60s.
- UDP send appears silent:
  - There is no inbound UDP handling; only outbound `set` and `ping` are used.
