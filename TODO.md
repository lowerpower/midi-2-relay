# TODO

Ranked by severity based on runtime impact and data corruption risk.

## Critical
- None (cleared on 2026-02-17).

## High
- None (cleared on 2026-02-17).

## Medium
- `Send_Bitmask_2_relay()` uses a static `bit_string` and compares incorrectly; change detection is flawed after first send. `src/midi.c`
- `load_map_if_new()` uses `difftime(old, new)` as a boolean and can reload on negative deltas. `src/load_map.c`
- `midi->map` uses `char` for relay IDs; sign/overflow possible. `src/midi.h`, `src/load_map.c`
- `Bitmask_2_String()` omits leading zeros; may break controller expectations. `src/midi.c`
- `udp_listener(0, ...)` binds an ephemeral port for a socket used only to send, which is confusing and possibly unintended. `src/net.c`, `src/midi.c`

## Low
- Mixed logging/printf with inconsistent verbosity control. `src/midi.c`, `src/log.c`, `src/debug.h`
- Manual-only test in `test/test.c`; no automated tests. `test/test.c`
- `console2udp/` duplicates the codebase and increases maintenance overhead.
