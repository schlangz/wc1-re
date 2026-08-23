# Naming policy

An implemented function takes the first name justified by evidence:

1. The developer's original spelling when stated by diagnostics, symbols,
   paths, imports, or matched recovered source.
2. A specific `<Verb><Object>` description of demonstrated behavior.
3. A name ending in `Hook` only when the original function is empty.

Operational labels such as `ScanTbl...`, `HelperOf...`, and `ReturnConst...`
describe mechanism, not intent, and must not survive implementation. Function
names never contain addresses. Identified globals use a Hungarian name and
retain their address in `/* 0x004xxxxx */` comments on declaration and
definition; unknown globals remain `DAT_<address>`.

Cross-release names are accepted only when the Win32 body, callers, neighboring
functions, and state accesses support them. The Win32 instructions always win.

The debug build's format strings provide primary naming evidence:

```sh
bin/nameOracle.py --top
bin/nameOracle.py
```

Inspect each reported call site because a wrapper may log a callee's name.
Before a rename, verify the address, arguments, return width, callers, strings,
and final `RET`; then update declarations and call sites and run
`make audit-addresses` plus `make compare-func`.
