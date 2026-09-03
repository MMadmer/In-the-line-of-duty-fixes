# In the Line of Duty Fixes — working rules

This file is the single source of truth for project work. Technical notes may
extend it, but they must not weaken or override these rules.

## 1. Scope

- Target game: S.T.A.L.K.E.R.: Shadow of Chernobyl with the mod
  "In the Line of Duty" ("По долгу службы").
- Target platform: the Windows x86 game build used by the supported mod
  installation.
- The project fixes defects without changing the intended story, balance,
  mechanics, content, or save format unless a task explicitly requires it.
- The installed game and mod are behavioral references and read-only inputs.

## 2. Non-destructive installation contract

- Never edit, overwrite, rename, move, delete, or repack an original game or
  mod file. This includes files under `bin`, `gamedata`, `mods`, packed game
  databases, configuration, scripts, assets, executables, and DLLs.
- The fix pack may ship only new files with project-owned, collision-resistant
  names. Installation must be ordinary drag-and-drop into the game root.
- C++ hooks, Lua hooks, DLL proxies, import hooks, and in-memory patches are
  allowed. Runtime state and functions may be intercepted or replaced in
  memory, but the bytes of the installed game and mod files must remain
  unchanged on disk.
- If a required destination path belongs to the game, the original mod, or an
  unknown addon, installation must stop and report the collision. It must never
  replace that file. A verified project-owned file from an earlier fix-pack
  installation may be updated transactionally through its ownership manifest.
- Every installed file belongs to an explicit manifest. Uninstallation removes
  only files from that manifest and restores the exact pre-install state.
- A fix that cannot be implemented without overwriting an existing game or mod
  file is not eligible for this project until a separate non-destructive
  loading mechanism is designed.

## 3. Starting work and evidence

1. Read this file and the relevant current technical notes.
2. Check Git status, the current branch, recent commits, the installed build,
   and existing deployed fix-pack files. Preserve unrelated changes.
3. Check available MCP servers and skills. Prefer IDA or another decompiler for
   native behavior and signatures; use logs and safe runtime inspection for
   runtime facts.
4. Reproduce the defect and record the exact error, log pattern, binary or
   script path, and affected game state before choosing a fix.
5. Fix the cause. Do not hide errors globally, suppress unrelated diagnostics,
   or disable validation merely to make the console quiet.
6. Existing defects discovered during the task are also in scope. Investigate
   and fix reproducible errors instead of leaving them solely because they
   predate the current changes. Preserve story, physics, and save compatibility.

Reference binaries, IDA databases, extracted scripts, dumps, and analysis
trees are strictly read-only. Store generated databases and notes in this
repository, never beside a reference binary when doing so would alter the
reference installation.

## 4. Compatibility and persistence

- Preserve existing saves, profiles, key bindings, `user.ltx`, screenshots,
  logs, and other player data.
- Preserve the public Lua API, engine ABI, serialized identifiers, network
  structures, and save structures.
- Do not add gameplay work to background threads or execute Lua concurrently.
- Runtime hooks must fail closed: if the supported executable or module does
  not match the validated identity, leave the game unmodified and write one
  clear diagnostic instead of patching an unknown address.
- Failing closed is per repair, never for the whole fix pack. A repair that does
  not depend on the executable - anything reached through an import resolved by
  name, any in-memory change to a mod script or config, the Lua payload - must
  keep working on an installation whose binaries are unknown. Only the repairs
  that write to a fixed address, or hand the engine an object it calls back
  through a pinned ABI, may require the exact build, and they disable only
  themselves. One unfamiliar file must never cost the player every other fix.
- Every launch writes `.ild-fixes/runtime/loader-report.txt`: the identity of each
  file the pack checks with its expected and actual hash, and the outcome of each
  repair. It is written before and regardless of any identity check, so an
  installation where nothing applied still explains itself without a console.
- Hooks must be narrowly scoped. Console fixes may change console editing or
  the broken debug-print path, but must not suppress genuine engine, Lua, or
  configuration errors.
- A player must be able to remove the fix pack and launch the untouched game
  and mod with the same saves.

## 5. Code

- Use modern language features and syntactic sugar when they do not harm
  compatibility, clarity, or performance.
- Follow the local style. New text files use UTF-8, a final newline, four-space
  indentation where applicable, and lines no longer than 120 characters.
- Check plain pointers through implicit boolean conversion: `if (pointer)` and
  `if (!pointer)`, not comparisons with `nullptr`.
- In Lua, take and override only Lua-defined methods through a luabind class
  object. Call inherited C++ methods on instances, and call base virtuals through
  the class object only where the engine exports a default implementation;
  otherwise luabind raises "pure virtual function called".
- Add short English comments only where the reason, lifetime, synchronization,
  binary contract, or compatibility constraint would otherwise be unclear.
- Do not ship temporary telemetry, probes, debug commands, verbose per-frame
  logging, dumps, or profiler output. A deliberate support diagnostic is the one
  exception: it must be off until a player turns it on, must not change anything
  the game persists, must cost nothing measurable while off, and must be
  documented in the packaging README.
- Release builds must pass with warnings treated as errors.

## 6. Build and packaging

- Build only from source stored in this repository. Generated binaries and
  packages must not be committed unless the repository explicitly adopts that
  policy later.
- Package layout mirrors the game root so the player can drag and drop it.
- The package must contain the install manifest, removal instructions, version,
  supported binary identities, and no original game or mod file.
- Before packaging, audit every payload path against the supported installation.
  A path collision is a release blocker.
- Run a second incremental build after a successful build and require it to do
  no work.

## 7. Runtime QA

- Build, stage, and test candidates inside this repository. Keep build output,
  the isolated game test root, QA logs, and all temporary artifacts here.
- During development and repository-local QA, treat the configured main game
  installation as a read-only source of the supported binaries, mod data,
  configuration, and newest existing save. End-of-work deployment follows
  section 7.1; it must not be silently omitted because some QA remains incomplete.
- Use a hidden desktop and the newest existing save unless the task requires
  another state. Copy the complete save group into the isolated test app-data
  root; never load the source files in a way that can update them.
- Keep ordinary smoke, UI, and gameplay QA short: 5 seconds after the required
  state is reached. Loading time is separate. Use up to 20 seconds only when a
  specific defect needs observation; do not apply that duration to every test.
- Before installing or testing a candidate, hash the complete save tree. Hash
  it again afterwards and require an exact match unless the test explicitly
  creates a disposable save in an isolated app-data root.
- Snapshot all pre-existing files in the game root that the candidate could
  reach. After QA, verify that their paths, sizes, and SHA-256 values are
  unchanged. New fix-pack files are checked separately against its manifest.
- Launch the supported game executable with an isolated QA log when practical.
  A startup smoke test does not count as a loaded-save test.
- For console work, verify on a loaded save that recurring spam has stopped,
  genuine diagnostics still reach the log, typing survives unrelated log
  output, and Ctrl+C/Ctrl+V copy and paste work as specified.
- Do not move the character or mutate the world unless the test requires it.
  Hidden launch, loading the save, console interaction, log reading, and
  stopping only the process started by QA are allowed.
- After each run, check logs for fatal errors, assertions, access violations,
  Lua errors, and new recurring warnings. Verify that the game, debugger,
  compiler, linker, and helper processes started by the test have exited.
- Keep intentional negative diagnostic tests in separate logs. A clean-run
  claim must be based on a fresh completed log, not filtering or an old marker.
- Do not remove logger output to pass QA. Repair malformed data, incorrect
  API use, or narrowly identified compatibility inputs; real diagnostics stay active.
- Remove temporary QA files and diagnostics after the result is recorded.

### 7.1. Mandatory end-of-work deployment

- At the end of every implementation batch, ALWAYS deploy the complete current
  fix-pack runtime into the configured main game installation. For this
  workspace, the main installation is `D:\Games\S.T.A.L.K.E.R`. A repository-local
  staging copy or a ZIP archive is not a substitute for this deployment.
- This is standing user authorization. Do not ask for the same deployment
  approval again merely because the current build is still a development
  candidate or because runtime/UI/updater QA could not be completed.
- Deploy the whole runtime payload together: native loader and hooks, updater,
  Lua scripts, UI XML, version, and ownership manifests. Never deploy selected
  DLLs or leave other runtime components on an older version.
- Incomplete or blocked QA must be reported explicitly, but it does not cancel
  end-of-work deployment of a successfully built, integrity-checked candidate.
  Deployment does not turn an unverified candidate into a verified release.
- Before copying, confirm that the game and relevant helper processes are
  closed, audit payload paths and foreign-file collisions, hash saves, and keep
  a recoverable snapshot of any project-owned files being replaced. Original
  game/mod files and player data remain outside the writable deployment scope.
- After copying, compare EVERY deployed runtime file with the built payload by
  SHA-256 and recheck save hashes. State the destination and verification result
  in the final response so the user knows the main game contains the new build.
- A failed build, an unresolved foreign-file collision, a running process that
  cannot safely be closed, or a restriction on the deployment operation itself
  is an actual deployment blocker. Report it specifically; never describe an
  undeployed result as completed and never bypass a tool or safety restriction.

## 8. Git and releases

- Keep the working tree limited to the current coherent batch. Do not commit
  failed experiments, logs, dumps, build output, or temporary instrumentation.
- A commit is created after a large successful and verified batch. Before a
  commit, run tests, `git diff --check`, and review the complete diff.
- The commit message describes the result briefly; it does not list files.
- If an attempt is abandoned, return the branch to the last successful state
  rather than keeping fictitious intermediate commits.

### 8.1. Versioning and release assets

- The version follows SemVer and is raised in one place, the CMake project
  version, from which the helper's product version, the package name, the
  ownership manifest and the update manifest are all generated. A release whose
  built helper reports a different product version is not publishable; the
  packaging script refuses it.
- A release carries `In-the-line-of-duty-fixes-VERSION-Setup_Manual.zip`, the
  complete runtime payload for drag-and-drop installation, and, when there is a
  previous release to cut it against, `In-the-line-of-duty-fixes-VERSION-Update_Patch.zip`,
  which carries only what changed since that release. They are alternatives and
  the player needs exactly one. The first release of a major line has no patch.
- Both archives must produce the same installed file set, must contain only
  project-owned paths, and must never contain an original game or mod file, a
  save, or the player's `user.ltx`.
- Package building is a local operation. Push, tag, GitHub Release, and asset
  upload happen only after the user's direct permission.
- Address every GitHub command explicitly to `MMadmer/In-the-line-of-duty-fixes`;
  do not rely on auto-detection.
- **A published release asset is never edited or replaced in place, and a
  published release that clients may already have seen is not deleted.** An
  installed client records the version it holds and asks the release list what
  is newer; replacing bytes under a name a client can already request makes the
  digest it verifies wrong, and it cannot repair itself from that. Supersede
  with a new version instead. A pre-release tag that no client outside the
  developer's own machine ever saw may be removed while that is still true.
- Before publishing, verify a clean HEAD, that the built helper's product
  version matches the tag, the package contents and their hashes, and that a
  test application of the archive leaves original files and the save tree
  untouched.
- After publishing, verify the tag target, the release status, the name and
  SHA-256 of every asset, and the list the updater actually reads:
  `repos/MMadmer/In-the-line-of-duty-fixes/releases?per_page=30`. The client
  takes the highest parseable version on that page, so the GitHub "Latest"
  badge is cosmetic; a tag it cannot parse, a draft, or a pre-release is
  invisible to every installed client.
- Raising the major version is a deliberate act with a user-visible
  consequence: installed clients of a lower major are offered the release as a
  separate installation with a link, not as an automatic update. Do not raise
  it merely to mark a large batch.

### 8.2. Mandatory GitHub Release text

- Keep the release text short and only about the confirmed changes of the
  version being published. No internal task numbers, hashes, file lists,
  unconfirmed promises, or long changelogs.
- Use the same structure for every release: changes first, then a short
  installation instruction. Both languages, RU and EN, in that order of
  sections shown below, because the in-game dialog reads whichever matches the
  player's language and shows nothing when its section is missing.
- In `Changes` list only the user-visible outcome. The baseline is what the
  player already has: for the first release of a line that is the original mod,
  and for every later release it is the previous release. Never mix the two, and
  never describe a change the player cannot observe.
- `Theme` is optional: one short line naming what the release is about. The
  update dialog prints it in bold above the change list and drops the line when
  the section is absent. Keep it to a headline; a bullet list there is ignored.
- In `Installation` state that the archive is extracted into the game root, that
  no original file is replaced, and that saves and the original mod are
  untouched. When a patch archive is also published, state that the two are
  alternatives and that the in-game updater picks the patch by itself.
- Before publishing, run a clean build if one has not been done, and verify the
  version number, the asset names, and the text against the packages actually
  built.

Release body template:

```markdown
## RU

## Тема

[Одна короткая строка о том, чему посвящён релиз, либо раздел не указывать]

## Изменения

* [Краткое описание исправления или улучшения, заметного игроку]
* [Краткое описание исправления или улучшения, заметного игроку]

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не
заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по
списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде.

---

## EN

## Theme

[One short line naming the release, or omit this section entirely]

## Changes

* [Short user-visible fix or improvement]
* [Short user-visible fix or improvement]

## Installation

Extract the archive into the game root. No original game or mod file is
replaced and existing saves keep working. The addon can be removed at any time
using `.ild-fixes/managed-files.txt`, and the mod keeps running as before.
```

## 9. Automatic updates

- Reuse the behavior of Dead Air Refined updates, not its engine UI classes.
  The offer, changes, buttons, and progress display use existing Shadow of
  Chernobyl UI classes inside the game.
- Check the pinned project repository once per game process without blocking
  the main menu. Ignore drafts, prereleases, malformed versions, and assets
  without a non-zero size and SHA-256 digest.
- Offer ordinary updates only within the installed major version. Announce a
  higher major separately without replacing the current installation.
- Read only the matching RU or EN Theme/Changes sections of release notes.
- Verify archive size, archive SHA-256, every manifest entry, path ownership,
  and extracted size before modifying an installed file. Reject traversal,
  Windows path aliases, duplicate paths, reparse points, and foreign files.
- Run the applier from a version-specific cache after the game exits. Keep a
  rollback snapshot until all files have been applied and verified. Restart
  with the original game arguments only after the update completes.
- Full and patch archives must produce the same final managed file set.
  Reject an inapplicable patch before mutation and fall back to a full archive.
- A loopback-only QA API is permitted only behind the explicit QA launch flag.
  Test update selection, downloading, corruption rejection, application,
  rollback, restart, and preservation of original files and saves.
