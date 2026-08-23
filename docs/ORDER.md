# Compilation-unit order

MSVC emits functions in source order and the linker concatenates objects in
command-line order. Use `src/map` and `make sort`; uncertain core boundaries
remain provisional.

```text
0x00401000  game core and Win32 support
0x004426A0  ix_log_printf
0x00442750  ix audio objects
0x004492E0  static debug CRT (not reconstructed)
```

## Proven `ix` order

| Range | Source |
| --- | --- |
| `0x00442750`–`0x00443DA5` | `src/ix/streamer.cpp` |
| `0x00443DA6`–`0x0044490F` | `src/ix/thread.cpp` |
| `0x00444910`–`0x004451B4` | `src/ix/dsp.cpp` |
| `0x004451B5`–`0x00445F5F` | `src/ix/dsps.cpp` |
| `0x00445F60`–`0x004467C4` | `src/ix/mixer.cpp` |
| `0x004467C5`–`0x004471FF` | `src/ix/dspv.cpp` |
| `0x00447200`–`0x00447CD7` | `src/ix/system.cpp` |
| `0x00447CD8`–`0x0044879B` | `src/ix/sound.cpp` |
| `0x0044879C`–`0x004492DF` | `src/ix/sample.cpp` |

`src/ix/ixlog.cpp` contains `ix_log_printf` immediately before this sequence;
its full original object extent is not proven.

## Core placement windows

| Source | Range |
| --- | --- |
| `src/winmain.c` | `0x401000`–`0x402DFF` |
| `src/sysinput.c` | `0x402E00`–`0x4030FF` |
| `src/cdrom.c` | `0x403100`–`0x4034FF` |
| `src/mono.c` | `0x403500`–`0x403E4F` |
| `src/auto.c` | `0x403E50`–`0x40460F` |
| `src/cmpgn.c` | `0x404610`–`0x40609F` |
| `src/brains.c` | `0x4060A0`–`0x40CFFF` |
| `src/nav.c` | `0x40D000`–`0x40FFFF` |
| `src/spc.c` | `0x410000`–`0x412FFF` |
| `src/cockpt.c` | `0x413000`–`0x417FFF` |
| `src/geom.c` | `0x418000`–`0x41AD4F` |
| `src/barracks.c` | `0x41ADA0`–`0x41C75F` |
| `src/debug.cpp` | `0x41C760`–`0x41D0BF` |
| `src/mathutil.c` | `0x41D000`–`0x41D24F` |
| `src/disk.c` | `0x41D250`–`0x41EFFF` |
| `src/ship.c` | `0x41F000`–`0x420FFF` |
| `src/logic.c` | `0x421000`–`0x424FFF` |
| `src/pilot.cpp` | `0x425000`–`0x426FFF` |
| `src/system.c` | `0x427000`–`0x4274DF` |
| `src/main.c` | `0x4274E0`–`0x427FFF` |
| `src/hudmsg.c` | `0x428000`–`0x42AFFF` |
| `src/pload.c` | `0x42B000`–`0x42B3FF` |
| `src/sound.c` | `0x42B400`–`0x42CFFF` |
| `src/music.c` | `0x42D000`–`0x42EFFF` |
| `src/screen.c` | `0x42F000`–`0x431FFF` |
| `src/dib.c` | `0x432000`–`0x43390F` |
| `src/text.c` | `0x433910`–`0x433ABF` |
| `src/smart.c` | `0x433AC0`–`0x434CCF` |
| `src/mathfp.c` | `0x434CD0`–`0x4353FF` |
| `src/strdos.c` | `0x435400`–`0x4355EF` |
| `src/eventmgr.c` | `0x4355F0`–`0x436FFF` |
| `src/screens.c` | `0x437000`–`0x43F5FF` |
| `src/killbrd.c` | `0x43F600`–`0x440BFF` |
| `src/gr.c` | `0x440C00`–`0x44269F` |

`src/globals.c` is temporary storage, not an original unit. Move a global only
when ownership and declaration order are proven. When a boundary changes,
update this table and `SRCS_ORDERED_CORE` together, then run `make order` and
`make verify`.
