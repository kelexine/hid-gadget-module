# Changelog

## v1.3.0 (2025-04-20 05:36)

- Automated entry for v1.3.0 build

Commit Message: Automated build: v1.3.0\n\nChanges:\n\n

Changes Summary:
```
 CHANGELOG.md                           |  15 ++++
 module.prop                            |   4 +-
 sepolicy.rule                          |  91 +++++++++-----------
 sepolicy.te (gone)                     |  73 ----------------
 system/bin/hid-setup                   | 153 +++++++++++++++++++++------------
 system/etc/hid/consumer-desc.bin (new) | Bin 0 -> 317 bytes
 system/etc/hid/keyboard-desc.bin (new) | Bin 0 -> 63 bytes
 system/etc/hid/mouse-desc.bin (new)    | Bin 0 -> 56 bytes
 update.json                            |   6 +-
 9 files changed, 160 insertions(+), 182 deletions(-)
```


## v1.3.0 (2025-04-20 05:36)

- Automated entry for v1.3.0 build

Commit Message: Automated build: v1.2.1\n\nChanges:\n\n

Changes Summary:
```
 CHANGELOG.md                      | 13 +++++++
 sepolicy.rule (new)               | 73 +++++++++++++++++++++++++++++++++++++++
 system/etc/init/init.hidgadget.rc |  2 +-
 3 files changed, 87 insertions(+), 1 deletion(-)
```


## v1.2.1 (2025-04-19 21:21)

- Automated entry for v1.2.1 build

Commit Message: Automated build: v1.2.1\n\nChanges:\n\n

Changes Summary:
```
 CHANGELOG.md | 79 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 79 insertions(+)
```


## v1.2.1 (2025-04-19 21:17)

- Automated entry for v1.2.1 build

Commit Message: Automated build: v1.2.1\n\nChanges Summary:\n\n CHANGELOG.md | 26 ++++++++++++++++++++++++++  module.prop  |  4 ++--  update.json  |  6 +++---  3 files changed, 31 insertions(+), 5 deletions(-)
  diff --git a/CHANGELOG.md b/CHANGELOG.md
  index bfeb595..080823f 100644
  --- a/CHANGELOG.md
  +++ b/CHANGELOG.md
  @@ -1,5 +1,31 @@
   # Changelog
  
  +## v1.2.1 (2025-04-19 21:09)
  +
  +- Automated entry for v1.2.1 build
  +
  +Commit Message: Automated build: v1.2
  +
  +Changes Summary:
  +```
  + CHANGELOG.md | 16 ++++++++++++++++
  + 1 file changed, 16 insertions(+)
  +```
  +
  +
  +## v1.2.0 (2025-04-19 21:06)
  +
  +- Automated entry for v1.2.0 build
  +
  +Commit Message: Automated build: v1.2
  +
  +Changes Summary:
  +```
  + CHANGELOG.md | 16 ++++++++++++++++
  + 1 file changed, 16 insertions(+)
  +```
  +
  +
   ## v1.2 (2025-04-19 20:17)
  
   - Automated entry for v1.2 build
  diff --git a/module.prop b/module.prop
  index 98fdd9d..93df396 100644
  --- a/module.prop
  +++ b/module.prop
  @@ -1,7 +1,7 @@
   id=hid-gadget
   name=HID Gadget
  -version=v1.2
  -versionCode=102
  +version=v1.2.1
  +versionCode=10201
   author=@kelexine GitHub
   description=USB HID Gadget emulation for keyboard, mouse, and consumer control
   updateJson=https://raw.githubusercontent.com/kelexine/hid-gadget-module/main/update.json
  diff --git a/update.json b/update.json
  index 25c8842..6eda9ea 100644
  --- a/update.json
  +++ b/update.json
  @@ -1,6 +1,6 @@
   {
  -  "version": "v1.2",
  -  "versionCode": 102,
  -  "zipUrl": "https://github.com/kelexine/hid-gadget-module/releases/download/v1.2/hid-gadget-module-v1.2.zip",
  +  "version": "v1.2.1",
  +  "versionCode": 10201,
  +  "zipUrl": "https://github.com/kelexine/hid-gadget-module/releases/download/v1.2.1/hid-gadget-module-v1.2.1.zip",
     "changelog": "https://github.com/kelexine/hid-gadget-module/blob/main/CHANGELOG.md"
   }\n

Changes Summary:
```
 CHANGELOG.md | 26 ++++++++++++++++++++++++++
 module.prop  |  4 ++--
 update.json  |  6 +++---
 3 files changed, 31 insertions(+), 5 deletions(-)
```


## v1.2.1 (2025-04-19 21:09)

- Automated entry for v1.2.1 build

Commit Message: Automated build: v1.2

Changes Summary:
```
 CHANGELOG.md | 16 ++++++++++++++++
 1 file changed, 16 insertions(+)
```


## v1.2.0 (2025-04-19 21:06)

- Automated entry for v1.2.0 build

Commit Message: Automated build: v1.2

Changes Summary:
```
 CHANGELOG.md | 16 ++++++++++++++++
 1 file changed, 16 insertions(+)
```


## v1.2 (2025-04-19 20:17)

- Automated entry for v1.2 build

Commit Message: Automated build: v1.2

Changes Summary:
```
 CHANGELOG.md                      |  14 +++++
 customize.sh                      | 122 +++++++++++++++++++++++++++-----------
 sepolicy.te (new)                 |  73 +++++++++++++++++++++++
 system/etc/init/init.hidgadget.rc |   4 ++
 4 files changed, 178 insertions(+), 35 deletions(-)
```


## v1.2 (2025-04-19 19:46)

- Automated entry for v1.2 build

Commit Message: Automated build: v1.2

Changes Summary:
```
 CHANGELOG.md         | 15 +++++++++++++++
 system/bin/hid-setup | 48 +++++++++++++++++++++++++++++++++---------------
 2 files changed, 48 insertions(+), 15 deletions(-)
```


## v1.2 (2025-04-19 17:58)

- Automated entry for v1.2 build

Commit Message: Automated build: v1.2

Changes Summary:
```
 CHANGELOG.md | 43 +++++++++++++++----------------------------
 module.prop  |  4 ++--
 update.json  |  6 +++---
 3 files changed, 20 insertions(+), 33 deletions(-)
```


## v1.2 (2025-04-19 16:08)

- Automated entry for v1.2 build

Commit Message: Automated build: v1.1

Changes Summary:
```
 CHANGELOG.md | 4 ++++
 1 file changed, 4 insertions(+)
```


## v1.1 (2025-04-19)

- Version v1.1 release

## v1.1 (2025-04-19)

- Version v1.1 release

## v1.1 (2025-04-19)

- Version v1.1 release

## v1.1 (2025-04-19)

- Version v1.1 release

## v1.1 (2025-04-19)
