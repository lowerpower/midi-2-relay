# Architecture Overview: midi-2-relay

**Document Level:** Medium  
**Scope:** Core runtime, modules, data flow, interfaces, build, and key risks.  
**Primary Runtime:** `src/midi.c` -> `midi2relay`

## System Summary
midi-2-relay is a single-process, event-driven C application that reads raw MIDI bytes from a serial port and translates note on/off events into UDP commands for a relay controller. It maintains a relay bitmask and sends `set <hex>` commands over UDP. It periodically reloads the MIDI-to-relay map file and sends a `ping` keepalive.

## Goals and Non-Goals
**Goals**
- Translate MIDI note on/off events into relay state changes.
- Send relay state updates over UDP with minimal latency.
- Allow dynamic updates of note/channel-to-relay mapping.
- Run continuously and optionally as a daemon on Unix.

**Non-Goals**
- Full MIDI protocol support (only note on/off and limited system handling).
- Reliable transport or ordering guarantees (UDP only).
- Rich configuration or management UI.

## Runtime Data Flow
1. Initialize defaults and state.
2. Load map file into `midi->map`.
3. Open and configure serial port (`/dev/ttyAMA0`) for MIDI input.
4. Create UDP socket, set non-blocking, register in select loop.
5. Enter select loop:
   - On serial read: parse MIDI stream into messages.
   - On note on/off: update relay bitmask and send `set <hex>` if changed.
   - Periodically reload map file if modified.
   - Send `ping` keepalive on idle timeout.

## Core Components
**Entry and Orchestration**
- `src/midi.c`
  - Main loop, command-line parsing, MIDI byte parsing.
  - Relay bitmask manipulation and UDP send.
  - Signal handling (Unix) and console control handler (Windows).

**Mapping**
- `src/load_map.c`
  - Parses mapping file lines: `<note> <channel> <relay>`.
  - Ignores comments (`#`) and whitespace.
  - Supports reload on mtime change.

**Networking**
- `src/net.c`, `src/net.h`
  - UDP socket utilities, non-blocking setup, timeouts.
  - `udp_listener` used to create a socket for sending.

**Select/IO Multiplexing**
- `src/yselect.c`, `src/yselect.h`
  - Wrapper around `select()` with internal FD sets.

**Platform Utilities**
- `src/arch.c`, `src/arch.h`
  - Timing, sleep, file utilities, string helpers.

**Daemonization (Unix)**
- `src/daemonize.c`, `src/daemonize.h`
  - Fork/setsid, pidfile, chroot, setuid/setgid.

**Logging/Debug**
- `src/log.c`, `src/log.h`, `src/debug.h`
  - Lightweight logging and debug printing (often compiled out).

## Primary Data Structures
**`MIDI` (src/midi.h)**
- Network target: `target_host`, `target_ip`, `target_port`, `soc`.
- Relay state: `bitmask[32]` (256 relays), `bit_string`.
- Map table: `map[128][16]` for note/channel to relay.
- Runtime state: `status`, `data1`, `data2`, `counter`.
- Files: `map_file`, `map_file_info`, `pidfile`.

## External Interfaces
**MIDI Input**
- Serial: `/dev/ttyAMA0` (38400 8N1 raw).
- Parses MIDI status/data bytes; supports note on/off and minimal system events.

**UDP Output**
- Commands: `set <hex>` and `ping`.
- Destination: configured by `target_host`/`target_port` (defaults in code).

**Mapping File**
- Default path: `/var/www/html/uploads/map.txt`.
- Format: `note channel relay` (integers).
- Notes: 0-127, channels: 0-15, relays: 0-127.

## Build and Artifacts
**Linux/Unix**
- Build via `src/makefile`.
- Target: `midi2relay`.

**Windows**
- Visual Studio solution/project in `win32/`.

## Operational Characteristics
**Concurrency**
- Single-threaded event loop using `select()`.

**Performance**
- Lightweight byte-by-byte parsing.
- Sends UDP only on state change (bitmask diff).

**Resilience**
- Restarts not managed by the app; suitable for external supervisor.
- Map file reload uses mtime check every ~60 seconds.

## Known Issues and Risks
- CLI `-t` is documented but not parsed due to missing `t` in getopt string.
- Default target port in code is `1027`, while usage text claims `5998`.
- `src/file_config.c` defines `load_map` but is not compiled; if added, it will conflict with `src/load_map.c`.
- `process_udp_in()` does not call `recvfrom`, so inbound UDP is effectively ignored.

## Extension Points
- Add support for more MIDI message types in `process_midi_command`.
- Improve map file schema (e.g., velocity ranges, multiple relays).
- Add config file parsing or unify with map file.
- Implement inbound UDP commands for state sync or control.

## Diagrams and Reference Docs
- `docs/midi.pdf`
- `docs/midi-flow.jpg`
- `docs/midi-processing.jpg`
