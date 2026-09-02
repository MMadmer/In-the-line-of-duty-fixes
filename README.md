# In the Line of Duty Fixes

Non-destructive fixes for S.T.A.L.K.E.R.: Shadow of Chernobyl and the mod
«По долгу службы». Original game/mod files and saves must remain unchanged.

Current version: 0.9.1. The project contains in-memory console fixes and an
in-game update dialog based on existing Shadow of Chernobyl UI classes.

Read [PROJECT_RULES.md](PROJECT_RULES.md) before working on the project.

## Build

Requires Windows, Visual Studio C++ x86 tools, CMake, and .NET Framework's C# compiler.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

`tools/package/Build-Package.ps1` produces the complete runtime archive and audits
destination collisions. Validation details are documented separately.

At the end of each implementation batch, the complete runtime payload is deployed
to the main game, never selected DLLs. Incomplete QA is disclosed explicitly; it
does not replace or silently cancel that mandatory candidate deployment. Original
game/mod files and saves remain untouched.
