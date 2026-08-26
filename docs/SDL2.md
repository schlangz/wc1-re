# SDL2 port

The native target reuses the reconstructed game and `ix` audio library.
`src/sdl/` owns host windows, input, timing, paths, audio, and presentation;
native objects stay under `out-modern/` and never enter the MSVC comparison
build.

The game still renders a 320x200 indexed frame. Both backends present it in a
centered 4:3 viewport and use that viewport for pointer mapping. `--enhanced`
selects the optional OpenGL 3.2 space-object renderer; the original software
path remains the default and fallback.

Kilrathi Saga data is fully supported. DOS support includes compressed packet
resources, `MUSIC.MID`, `WINGLDR.TIM`, AdLib music, and synthesized OriginFX
effects. In-flight sampled and synthesized effects use camera-relative stereo
panning; synthesized effects retain distance falloff.

Kilrathi Saga wave effects receive 6 dB of mix headroom so full-scale and
overlapping weapon sounds do not mask the streamed music. The DOS OriginFX
path retains its separate synthesized music-and-effects mix.

Development builds use ASan and UBSan. Release builds select
`MODERN_RELEASE=1`.

## Joystick input

`--joystick-mode=original` preserves the original two-axis, two-button layout.
The `4button-2axis` and `4button-4axis` modes add WCAT-style controls. The
four-axis mode accepts:

| `--joystick-axes=` | Extra-axis behavior |
| --- | --- |
| `twin-stick-roll` | Extra X rolls; extra Y is relative throttle |
| `twin-stick-yaw` | Extra X yaws; extra Y is relative throttle |
| `hotas-yaw` | Primary X rolls; rudder yaws; final axis is throttle |
| `hotas-roll` | Primary X yaws; rudder rolls; final axis is throttle |
| `linear-throttle` | Third axis is throttle; keyboard controls roll |
| `rudder-yaw` | Third axis yaws |
| `rudder-roll` | Third axis rolls |

Use `--joystick-debug` for event diagnostics and `--joystick-rumble` for
supported feedback. The README documents the common controller bindings.

## Commands

```sh
make modern
make modern-test
make run-modern
make run-modern-dos
make run-modern-mission SERIES=1 MISSION=0
```

`modern-test` is a data-free SDL2 initialization smoke check.
