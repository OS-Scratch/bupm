# bupm changelog
**Current Version: 1.3.0**

## 1.3.0-git:..
### New Features
- removing packages: support implemented
- added cleaner dependency support
### Changes
**remove.cc**, **remove.h**:
- Added removing capability
**main.cc**:
- implemented remove.cc / remove.h accessibility
**install.cc**:
- fix dependency support
- fix critical archive error where symlinks would not work (1.2.2 fix, didn't work as expected), packages zlib and zlib-ng

## 1.2.3-git:3e8e0f4..d5a9d62
### New Features
- bupm repository now has toml files instead of yaml
### Changes
**fetch.cc**:
- Add toml support
**meson.build**:
- Update dependencies (remove yaml-cpp, include tomlplusplus)

## 1.2.2-git:857a8b0..3e8e0f4
## **Security Patch**
### Fixes
**install**:
- tar: Fix extraction security logic (thanks to an user to reporting it!). Now, bupm ignores symlinks and .. sequences, so a file is not overwritten accidentally.
- Key Verification: added in install to check integrity of packages. (Suggested by an other user using Gemini). 

## 1.2.1-git:d8dc3c9..4b6b350
### Fixes
**install**: Added missing line breaks.
**repo**:
- **Repository::help()**: fixed call to repo.
- **repo.h header**: Fixed inclusion of Repository::help()
