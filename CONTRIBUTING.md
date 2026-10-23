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

## Releases

The `project(XPControl VERSION ...)` declaration in `CMakeLists.txt` supplies the package version, DLL metadata, and runtime log version. To publish a release:

1. Update the project version and commit the release changes to `master`.
2. Create and push the matching annotated tag:

   ```sh
   git tag -a v0.1.0 -m "XP Control 0.1.0"
   git push origin v0.1.0
   ```

The workflow validates the tag, runs Linux and Windows tests, builds and audits `XPControl.dll`, packages the outputs, and publishes the assets on GitHub Releases. A suffix such as `v0.1.0-rc.1` marks a prerelease. Version tags must match the CMake version before the suffix.

Publishing uses the workflow's `GITHUB_TOKEN`. Write permission is scoped to the release job. Each release is published after its assets finish uploading. A rerun verifies an existing published release or completes an interrupted draft. The workflow can also be dispatched against an existing version tag.

## Licensing

Contributions are distributed under the project's license and exceptions. Preserve applicable third-party attribution when adding or updating dependencies.
