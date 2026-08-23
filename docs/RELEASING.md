# Releases

The Release workflow publishes only on a pushed annotated
`vMAJOR.MINOR.PATCH` tag. A manual workflow run builds artifacts without
creating a release.

```sh
git tag -a v0.1.0 -m "v0.1.0"
git push origin v0.1.0
```

The workflow builds the MSVC 4.20 Win32 reference executable plus SDL2 archives
for Windows x86-64, Linux x86-64, and macOS x86-64/arm64. It generates
`SHA256SUMS` and release notes. No game data is included. SDL2 release builds
use `MODERN_RELEASE=1` and are checked for sanitizer runtimes.
