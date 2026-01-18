# Changelog

## [v1.35.3] - 2026-01-18
### Added
- **Automated CI/CD**: Added GitHub Actions workflow (`release.yml`) for zero-touch releases.
- **Dynamic Attribution**: Build script now dynamically updates `update.json` and `module.prop` URLs based on the hosting repository (e.g., `kelexine` vs `rexackermann`).
- **Media Keys**: Full 14-key Consumer HID deck implemented in TUI with reliable 2-byte reports.
- **Mouse Hold**: Added `[HL]`, `[HM]`, `[HR]` toggles for drag-and-drop operations.
- **Radar Mouse**: Virtual analog stick control with aspect ratio correction and center-click.

### Changed
- **TUI Rewrite**: Complete rewrite of the TUI in C using `termbox2` (v1.30+).
- **Consolidated Versioning**: Jumped to v1.35.x to mark the stability milestone.
- **Repo Cleanup**: Removed experimental `portable/` mode to focus on Magisk module stability.
- **Authorship**: Updated credits to correctly attribute `kelexine` (Original) and `rexackermann` (Contributor).

### Fixed
- **HID Consumer**: Fixed issue where media keys were interpreted as text input strings.
- **Update JSON**: Fixed inconsistency between `zipUrl` naming and actual build artifacts.

## [v1.25.1] - 2026-01-18
### Fixed
- **TUI Exit**: Fixed issue where `Ctrl+Alt+Q` was not being captured correctly in some terminals.
- **Input Mode**: Enabled `TB_INPUT_ALT` for reliable modifier detection.
- **Fail-safe**: Added `Ctrl+C` and `Ctrl+Q` (with Alt) as fallback exit sequences.

## [v1.25.0] - 2026-01-18
- **Auto-Recovery Logic**: Added automatic recovery to all wrapper scripts. If HID operation fails, the script will automatically attempt to run `hid-setup` and `setprop` via `su` before retrying.
- **Robustness**: Improved reliability on Android systems with flaky USB gadget states.

## v1.23.7 (2026-01-17)
- **C-based TUI Rewrite**: Rewrote the `hid-tui` interface in C using `termbox2`.
- **Zero Dependencies**: Removed Python requirement for the terminal keyboard.
- **Improved Performance**: Faster rendering and zero cold-start delay.
- **Universal Compatibility**: Works on any rooted Android/Linux system without pre-installed interpreters.

## v1.23.6 (2026-01-17)
- **New TUI Keyboard Interface**: Added `hid-tui`, a laptop-style terminal keyboard with mouse support and sticky modifiers.
- **Improved WIN Key Logic**: Toggles GUI modifier on first press, sends standalone WIN key on double-press.
- **Enhanced Portability**: `hid-tui` is fully self-contained and available in the `portable/` directory.

## v1.23.5 (2026-01-17)
- **Fixed Keyboard Modifier Logic**: Rewrote `parse_modifiers` to correctly handle single modifiers and composite strings like `CTRL-C`.
- **Universal Static Binaries**: All binaries are now statically linked with `musl` using Zig CC, ensuring zero library dependencies on Android (Bionic) or Linux (glibc).
- **Architecture Blobs**: Included pre-compiled binaries for `arm64`, `arm`, `x86_64`, and `x86` in the flashable ZIP.
- **Robust Wrapper Scripts**: Updated all shell scripts to use relative path resolution (`dirname $0`) for better reliability.
- **Standalone Portable Mode**: Added a `portable/` directory with architecture-aware scripts for testing without installing the module.
- **Improved Aliases**: Added `WIN`, `META`, `SUPER`, and `CONTROL` as standard aliases.
- **Key List Documentation**: Comprehensive list of supported keys added to README.

## v1.18.3 (2025-08-12 19:52)
- Automated build: v1.18.3 (code: 11803)

## v1.18.2 (2025-08-12 19:51)
- Automated build: v1.18.2 (code: 11802)

## v1.18.2 (2025-08-12 19:45)
- Automated build: v1.18.2 (code: 11802)

## v1.18.1 (2025-08-12 19:40)
- Automated build: v1.18.1 (code: 11801)

## v1.18.0 (2025-08-12 19:37)
- Automated build: v1.18.0 (code: 11800)

## v1.17.0 (2025-08-12 19:32)
- Automated build: v1.17.0 (code: 11700)

## v1.17.0 (2025-04-22 09:35)
- Version: v1.17.0 (Code: 11700)
- Build Timestamp: 2025-04-22 09:35
