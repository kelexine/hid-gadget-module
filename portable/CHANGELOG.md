# Changelog

## v1.23.3 (2026-01-17)

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

## v1.17.0 (2025-08-12 19:31)

- Automated build: v1.17.0 (code: 11700)

# Changelog

## v1.17.0 (2025-04-22 09:35)

* **Automated Build Details:**
    * Version: v1.17.0 (Code: 11700)
    * Build Timestamp: 2025-04-22 09:35
    * Commit: c7a5986
    * Commit Subject: Automated build: v1.17.0

* **Technical Changes Summary (from Git diff):**
```
 CHANGELOG.md | 34 +++++++++++++---------------------
 1 file changed, 13 insertions(+), 21 deletions(-)
```


## v1.17.0 (2025-04-22 09:35)

* **Automated Build Details:**
    * Version: v1.17.0 (Code: 11700)
    * Build Timestamp: 2025-04-22 09:35
    * Commit: f409fce
    * Commit Subject: Automated build: v1.17.0

* **Technical Changes Summary (from Git diff):**
```
 CHANGELOG.md | 30 +++++++++++++++---------------
 1 file changed, 15 insertions(+), 15 deletions(-)
```


## v1.17.0 (2025-04-22 09:12)

* **Automated Build Details:**
    * Version: v1.17.0 (Code: 11700)
    * Build Timestamp: 2025-04-22 09:12
    * Commit: ca1d180
    * Commit Subject: Automated build: v1.17.0

* **Technical Changes Summary (from Git diff):**
```
 CHANGELOG.md | 30 +++++++++++++++---------------
 1 file changed, 15 insertions(+), 15 deletions(-)
```
