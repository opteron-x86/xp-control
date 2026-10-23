# Contributing to XP Control

## Development

Fork the repository, create a branch, and make a focused change. Use the pinned dependencies and the build commands in [README.md](README.md). The `windows-release` preset produces the distributable DLL; `native-tests` runs the portable test suite.

The project uses C++23. Keep curve calculations and settings logic independent of game types so they can be tested on Windows and Linux. Use CommonLib's runtime-aware accessors for engine data and the game task queue for runtime changes.

## Tests

Add tests for behavior changes. Relevant cases include:

- XP below, equal to, and above the current and proposed thresholds.
- Pending level-ups and accumulated XP spanning multiple levels.
- Valid, malformed, and boundary-value configuration files.
- Settings persistence and file-operation failures.
- Save loading, character creation, menu transitions, and session cancellation.

For runtime changes, record the Skyrim version, SKSE version, relevant mods, reproduction steps, and observed results. Exercise configuration changes, XP awards, level-up completion, save/reload, and starting another character within the same game process.

## Pull requests

Describe the behavior, the reason for the change, and its validation. Include screenshots for visible UI changes and log excerpts for runtime behavior. Keep build output and editor files in ignored paths. CI checks must pass for the submitted commit.

## Issues

Include the mod version or commit, Skyrim and SKSE versions, configuration, reproduction steps, and the relevant `XPControl.log` excerpt. For a crash, include a crash report and the actions immediately preceding it.

## Dependency updates

Update dependency pins and license attribution together. Validate the universal SE/AE/VR build, the SKSE metadata audit, and both test platforms.

## Licensing

Contributions are distributed under the project's license and exceptions. Preserve applicable third-party attribution when adding or updating dependencies.
