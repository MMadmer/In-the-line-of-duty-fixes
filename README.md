# In the Line of Duty Fixes

Non-destructive fixes for S.T.A.L.K.E.R.: Shadow of Chernobyl and the mod
«По долгу службы». Original game/mod files and saves must remain unchanged.

Current local version: 0.9.0, intentionally retained for update testing against
GitHub 0.9.1. The fix pack repairs console editing, incorrect door logic assigned
to a decorative table, knife replacement through inventory, and the in-game
updater layout. Original assets are changed only in private memory or through
runtime hooks, never on disk.

See [gameplay and UI QA](docs/GAMEPLAY_AND_UI.md) for the tested cases and limits.

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
