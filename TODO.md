# TODO

Ranked by severity based on runtime impact and data corruption risk.

## Critical
- None (cleared on 2026-02-17).

## High
- None (cleared on 2026-02-17).

## Medium
- None (cleared on 2026-02-17).

## Low
- Mixed logging/printf with inconsistent verbosity control. `src/midi.c`, `src/log.c`, `src/debug.h`
- Manual-only test in `test/test.c`; no automated tests. `test/test.c`
- `console2udp/` duplicates the codebase and increases maintenance overhead.
