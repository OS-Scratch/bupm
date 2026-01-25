# bupm changelog
**Current Version: 1.2.2**

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
