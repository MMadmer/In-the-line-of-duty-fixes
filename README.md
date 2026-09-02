# In the Line of Duty Fixes

Non-destructive fixes for S.T.A.L.K.E.R.: Shadow of Chernobyl and the mod
«По долгу службы». Original game/mod files and saves must remain unchanged.

Current source version: 0.9.0. This is a development candidate, not a verified
release. The source contains an in-memory console fix and an in-game update dialog based on existing SoC UI
classes. Update end-to-end QA remains blocked by the execution tool's policy.

Read [PROJECT_RULES.md](PROJECT_RULES.md) before working on the project.

## Build

Requires Windows, Visual Studio C++ x86 tools, CMake, and .NET Framework's C# compiler.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

`tools/package/Build-Package.ps1` produces a complete candidate payload and audits
destination collisions. Packaging does not mean the runtime QA gates have passed.

At the end of each implementation batch, the complete runtime payload is deployed
to the main game, never selected DLLs. Incomplete QA is disclosed explicitly; it
does not replace or silently cancel that mandatory candidate deployment. Original
game/mod files and saves remain untouched.
