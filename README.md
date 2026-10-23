# XP Control

XP Control is an SKSE plugin for configuring Skyrim's character XP requirements. It provides an INI configuration and an SKSE Menu Framework panel with presets, a curve preview, and a level-by-level comparison table.

## XP curve

The XP required for the next level is:

```text
BaseXP + (current level × XPPerLevel)
```

The default **Standard** preset uses a base of **375 XP** and an increase of **15 XP per level**. The panel also includes Vanilla, Moderate, Balanced, Gentle, Flat, and Slower Flat presets.

Settings apply to every character using the configuration. **Apply and save** persists both values and refreshes the active XP requirement. When a change would affect pending-level eligibility, the current threshold remains active until the next level-up. Subsequent levels use the configured curve, and accumulated XP is preserved.

## Requirements

- Skyrim Special Edition, Anniversary Edition, or VR, with the matching SKSE release.
- Address Library for the installed Skyrim runtime; VR uses VR Address Library.
- SKSE Menu Framework 3 for the in-game panel.

The universal DLL uses [alandtse CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG) with SE, AE, and VR support enabled. Runtime-aware accessors cover the 1.6.629 and 1.7.99 layout changes and the VR player-data layout. XP awards from Experience use the same character XP system.

## Installation

1. Download the build artifact from [GitHub Actions](https://github.com/opteron-x86/xp-control/actions).
2. Install the `XP-Control-<version>-<commit>.zip` archive with a mod manager.
3. Start Skyrim through SKSE.

Open **SKSE Menu Framework → XP Control → XP curve** to configure the curve. The framework's key binding controls access to the panel.

The configuration file is `Data/SKSE/Plugins/XPControl.ini`:

```ini
[Curve]
BaseXP=375
XPPerLevel=15
```

`BaseXP` accepts values from 1 to 100,000. `XPPerLevel` accepts values from 0 to 10,000. INI configuration is available independently of the panel. The runtime log is `XPControl.log` in the SKSE log directory.

## Build

Dependencies are pinned for reproducible builds. CommonLibSSE-NG **13.1.0** is compiled from commit `12d61f3c39f7d4ba1b1ab543df2bca0e613dc747`; `vcpkg.json` defines the dependency baseline.

### Windows

Install Visual Studio C++ build tools, clang-cl, CMake 3.28 or newer, Ninja, Python, and vcpkg. Set `VCPKG_ROOT` to a bootstrapped vcpkg checkout. Run these commands from an x64 developer shell:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --test-dir build/windows-release --verbose
python tools/package.py --build build/windows-release --output dist
```

The build produces `XPControl.dll` and `XPControl.pdb`. Packaging produces the installable archive, debug-symbol archive, build metadata, and SHA-256 checksums in `dist/`.

### Linux tests

Install CMake 3.28 or newer, Ninja, and GCC 14 or newer, or a recent Clang:

```sh
cmake --preset native-tests
cmake --build --preset native-tests
ctest --preset native-tests --verbose
```

This preset enables AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover XP boundaries, configuration parsing and persistence, and session task cancellation.

### Continuous integration

Pushes to `master`, pull requests, and manual workflow runs execute Linux sanitizer tests, compile the Windows DLL, run Windows tests, audit the SKSE exports and compatibility metadata, and upload packaged artifacts. Each artifact records its source commit.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development, testing, and pull-request guidance. Issues and contributions are welcome through the repository's GitHub pages.

## License

XP Control is distributed under GPL-3.0-or-later with the permissions in [EXCEPTIONS.md](EXCEPTIONS.md). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency attribution and licenses.
