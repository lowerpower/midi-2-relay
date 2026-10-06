# Changelog

## 2026-10-05
### Added
- Max note on-time, `-m <ms>` (default 1000, 0 = no limit). A relay turned on by a note is turned off after this long even if no note-off arrives, and stays off until a note turns it on again (a repeated note-on restarts the timer). The main loop wakes in time for the next expiry, and relays expiring together go out in one `set`. Also bounds the damage from a lost note-off. `src/midi.c`, `src/midi.h`
- The on-time starts when the relay's on state is sent, not when the note is parsed, so the gather window never eats into it; gathering is cut short at the next relay deadline and expiries are rechecked after it; note timing uses a monotonic clock. `src/midi.c`, `src/midi.h`

## 2026-10-02
### Changed
- Batch note changes into one relay update. Note on/off now only updates the bitmask; the main loop sends a single `set` after each serial read, after gathering more bytes for up to `-g` ms (default 3). A chord is one `set` instead of one per note, which stops `usb-relay` falling behind under dense MIDI sequences. `src/midi.c`, `src/midi.h`

### Added
- `-g <ms>` gather window for batching (0 = batch per serial read only, max 50). `src/midi.c`

## 2026-02-17
### Fixed
- Enabled `-t` parsing to allow setting target host/port. `src/midi.c`
- Guarded relay index updates to prevent bitmask corruption and skip invalid mappings. `src/midi.c`
- Implemented UDP reads in `process_udp_in()` by calling `recvfrom()`. `src/midi.c`
- Tightened map parsing to reject relay `0` and out-of-range values. `src/load_map.c`
- Updated usage text to match the default target port. `src/midi.c`
- Eliminated `load_map()` name conflict by renaming the config reader to `read_config()`. `src/file_config.c`, `src/file_config.h`
- Fixed bitmask change detection to avoid redundant skips. `src/midi.c`
- Made map reload detection deterministic without `difftime` edge cases. `src/load_map.c`
- Stored relay mappings as `U16` to avoid signed overflow. `src/midi.h`
- Made bitmask encoding include leading zeros for a stable wire format. `src/midi.c`
- Avoided binding a UDP socket when listen port is disabled. `src/midi.c`
- Standardized runtime logging in MIDI processing around one verbosity-controlled path. `src/midi.c`
- Optimized bitmask-to-hex encoding and relay command formatting in the MIDI hot path. `src/midi.c`
- Reduced event-loop timing overhead by reusing a single `second_count()` value per loop iteration. `src/midi.c`
- Removed unconditional per-iteration `fflush(stdout)` to reduce loop I/O overhead. `src/midi.c`
- Replaced Linux `second_count()` `ftime` path with `gettimeofday` to reduce deprecated/legacy overhead. `src/arch.c`
- Throttled high-verbosity idle marker output (`.` and `,`) to reduce stdout churn in the event loop. `src/midi.c`
- Switched serial input from single-byte reads to buffered reads in the event loop. `src/midi.c`
- Cached UDP target socket address and reused it for `sendto()` calls. `src/midi.c`, `src/midi.h`
- Added automatic header dependency tracking to automated test builds. `tests/Makefile`
- Reduced warning noise by fixing unused-parameter and fixed-array string checks. `src/midi.c`, `src/log.c`

### Added
- Serial device override via `-s <device>` CLI option. `src/midi.c`, `src/midi.h`
- Optional UDP listen port via `-u <port>`. `src/midi.c`, `src/midi.h`
- Automated unit test suite and test runner (`43` assertions). `tests/Makefile`, `tests/test_runner.c`, `tests/test_basic.c`, `tests/test_parse.c`, `tests/test_strings.c`
