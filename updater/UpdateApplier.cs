using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace IldFixes.Updater
{
    internal static class UpdateApplier
    {
        internal const string ManifestPath = ".ild-fixes/update-manifest.txt";
        private const string ManagedListPath = ".ild-fixes/managed-files.txt";
        private const string VersionPath = ".ild-fixes/version.txt";
        private const string ResultPath = ".ild-fixes/runtime/apply-result.txt";
        private const int MaximumFiles = 1024;
        private const long MaximumExpandedBytes = 1024L * 1024 * 1024;
        private const int MoveFileReplaceExisting = 0x1;
        private const int MoveFileWriteThrough = 0x8;
        // MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST
        private const uint FailureBox = 0x00000030 | 0x00010000 | 0x00040000;

        private sealed class ManifestFile
        {
            internal string Relative;
            internal string Hash;
            internal long Size;
        }

        private sealed class Manifest
        {
            internal Version Version;
            internal bool Patch;
            internal Version Base;
            internal readonly List<ManifestFile> Files = new List<ManifestFile>();
        }

        // Carries the exit code together with the reason shown to the player and kept for the next session.
        private sealed class Failure : Exception
        {
            internal readonly int Code;

            internal Failure(int code, string reason) : base(reason)
            {
                Code = code;
            }
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool MoveFileEx(string existing, string replacement, int flags);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int MessageBoxW(IntPtr owner, string text, string caption, uint type);

        internal static int Apply(Arguments arguments)
        {
            string game = FullDirectory(arguments.Required("--game-dir"));
            string restartExe = arguments.Required("--restart-exe");
            string restartArgs = arguments.Optional("--restart-args") ?? string.Empty;
            bool russian = arguments.Optional("--lang") == "ru";
            bool quiet = arguments.HasFlag("--quiet");
            try
            {
                ApplyVerified(arguments, game, restartExe, restartArgs);
                return 0;
            }
            catch (Failure failure)
            {
                return Fail(game, failure.Code, failure.Message, failure.Code != 25, restartExe, restartArgs,
                    russian, quiet);
            }
            catch (Exception error)
            {
                // The mutation block converts its own errors into a rollback; anything else left the files consistent.
                return Fail(game, 2, error.Message, true, restartExe, restartArgs, russian, quiet);
            }
        }

        private static void ApplyVerified(Arguments arguments, string game, string restartExe, string restartArgs)
        {
            string archive = Path.GetFullPath(arguments.Required("--archive"));
            Version version = ParseVersion(arguments.Required("--version"));
            string digest = arguments.Required("--digest");
            long size = arguments.RequiredLong("--size");
            int waitPid = arguments.RequiredInt("--wait-pid");
            long gameStart = arguments.OptionalLong("--game-start");

            string expectedCache = Path.Combine(game, ".ild-fixes", "update-cache", version.ToString());
            if (!string.Equals(Path.GetDirectoryName(archive), expectedCache, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(Path.GetFullPath(restartExe), Path.Combine(game, "bin", "XR_3DA.exe"),
                    StringComparison.OrdinalIgnoreCase) || !ValidDigest(digest))
                throw new Failure(19, "The update request does not belong to this installation.");
            EnsureNoReparsePoints(game, archive);

            if (!File.Exists(archive) || new FileInfo(archive).Length != size ||
                UpdateClient.Digest(archive) != UpdateClient.NormalizeDigest(digest))
                throw new Failure(20, "The downloaded archive failed its size or SHA-256 check.");
            if (!WaitForProcess(waitPid, gameStart, TimeSpan.FromMinutes(2)))
                throw new Failure(28, "The game did not exit in time.");

            string cache = Path.GetDirectoryName(archive);
            string stage = Path.Combine(cache, "stage");
            string backup = Path.Combine(cache, "backup");
            RecreateDirectory(stage);
            RecreateDirectory(backup);

            Version installedVersion = ReadInstalledVersion(game);
            Manifest manifest;
            try
            {
                using (ZipArchive zip = ZipFile.OpenRead(archive))
                {
                    Dictionary<string, ZipArchiveEntry> entries = ValidateArchive(zip);
                    manifest = ReadManifest(entries[ManifestPath]);
                    if (manifest.Version != version)
                        throw new Failure(21, "The archive version does not match the offered update.");
                    if (manifest.Patch && (installedVersion == null || manifest.Base != installedVersion))
                        RejectPatch(game, version, "The patch was built for another installed version.");
                    ValidateAndStage(entries, manifest, game, stage, version);
                }
            }
            catch (Failure)
            {
                throw;
            }
            catch (Exception error)
            {
                throw new Failure(21, "The update archive is invalid: " + error.Message);
            }

            HashSet<string> previous = ReadManagedFiles(game);
            HashSet<string> target = PayloadPaths(manifest);
            HashSet<string> scope = new HashSet<string>(previous, StringComparer.OrdinalIgnoreCase);
            scope.UnionWith(target);
            foreach (string relative in target)
            {
                if (File.Exists(Destination(game, relative)) && !previous.Contains(relative))
                    throw new Failure(26, "A foreign file occupies the fix-pack path " + relative + ".");
            }
            VerifyInstalledFiles(game, previous, installedVersion);

            Dictionary<string, bool> existed = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);
            try
            {
                foreach (string relative in scope)
                {
                    if (!OwnedPath(relative))
                        throw new InvalidDataException("The installed managed-file list escaped the fix-pack scope.");
                    string destination = Destination(game, relative);
                    bool present = File.Exists(destination);
                    existed.Add(relative, present);
                    if (present)
                    {
                        string saved = Destination(backup, relative);
                        Directory.CreateDirectory(Path.GetDirectoryName(saved));
                        File.Copy(destination, saved, true);
                    }
                }

                RemoveOrphanTemporaries(game, target);
                foreach (string relative in target)
                {
                    string staged = Destination(stage, relative);
                    if (File.Exists(staged))
                        AtomicCopy(staged, Destination(game, relative));
                }

                foreach (string relative in previous)
                {
                    if (target.Contains(relative))
                        continue;
                    string dropped = Destination(game, relative);
                    if (File.Exists(dropped))
                        File.Delete(dropped);
                }

                foreach (ManifestFile file in manifest.Files)
                {
                    string installed = Destination(game, file.Relative);
                    if (!File.Exists(installed) || new FileInfo(installed).Length != file.Size ||
                        UpdateClient.Digest(installed) != file.Hash)
                        throw new InvalidDataException("The installed payload failed its final verification.");
                }
                if (!File.Exists(Destination(game, ManifestPath)))
                    throw new InvalidDataException("The installed manifest is missing.");
            }
            catch (Exception error)
            {
                bool restored = Rollback(game, backup, scope, existed);
                throw new Failure(restored ? 22 : 25, (restored ? "The update was rolled back: " :
                    "The update could not be rolled back completely: ") + error.Message);
            }

            string rejected = Path.Combine(game, ".ild-fixes", "patch-rejected.txt");
            if (File.Exists(rejected))
                File.Delete(rejected);

            string installedUpdater = Path.Combine(game, "InTheLineOfDutyFixesUpdater.exe");
            if (!File.Exists(installedUpdater))
            {
                bool restored = Rollback(game, backup, scope, existed);
                throw new Failure(restored ? 23 : 25, "The installed updater is missing after the update.");
            }

            long ownStart = 0;
            int ownPid;
            using (Process current = Process.GetCurrentProcess())
            {
                ownPid = current.Id;
                try { ownStart = current.StartTime.ToFileTimeUtc(); }
                catch (InvalidOperationException) { }
                catch (System.ComponentModel.Win32Exception) { }
            }
            string finishArguments = "--finish --game-dir " + Quote(game) + " --cache " + Quote(cache) +
                " --wait-pid " + ownPid.ToString(CultureInfo.InvariantCulture) +
                " --wait-start " + ownStart.ToString(CultureInfo.InvariantCulture) +
                " --restart-exe " + Quote(restartExe) + " --restart-args " + Quote(restartArgs);
            Process.Start(new ProcessStartInfo(installedUpdater, finishArguments)
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = game
            });
        }

        internal static int Finish(Arguments arguments)
        {
            string game = FullDirectory(arguments.Required("--game-dir"));
            string cache = Path.GetFullPath(arguments.Required("--cache"));
            int waitPid = arguments.RequiredInt("--wait-pid");
            long waitStart = arguments.OptionalLong("--wait-start");
            string restartExe = arguments.Required("--restart-exe");
            string restartArgs = arguments.Optional("--restart-args") ?? string.Empty;
            string cacheRoot = FullDirectory(Path.Combine(game, ".ild-fixes", "update-cache"));
            string cacheWithSeparator = cacheRoot.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            if (!cache.StartsWith(cacheWithSeparator, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(Path.GetDirectoryName(cache), cacheRoot, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(Path.GetFullPath(restartExe), Path.Combine(game, "bin", "XR_3DA.exe"),
                    StringComparison.OrdinalIgnoreCase))
                return 30;
            ParseVersion(Path.GetFileName(cache));
            EnsureNoReparsePoints(game, cache);

            WaitForProcess(waitPid, waitStart, TimeSpan.FromMinutes(1));
            // Every cached version is obsolete after a verified install; cleanup must never block the restart.
            try
            {
                if (Directory.Exists(cacheRoot))
                    Directory.Delete(cacheRoot, true);
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
            TryDelete(Path.Combine(game, ResultPath.Replace('/', Path.DirectorySeparatorChar)));
            Restart(restartExe, restartArgs);
            return 0;
        }

        private static int Fail(string game, int code, string reason, bool restartable, string restartExe,
            string restartArgs, bool russian, bool quiet)
        {
            WriteResult(game, code, reason);
            string text = restartable ?
                (russian ? "Не удалось обновить фикспак (код {0}): {1}\n\nПрежняя версия сохранена; игра будет запущена снова." :
                    "The fix pack update failed (code {0}): {1}\n\nThe previous version was kept; the game will start again.") :
                (russian ? "Обновление прервано, и откат выполнен не полностью (код {0}): {1}\n\nПереустанови фикспак вручную перед запуском игры." :
                    "The update failed and could not be rolled back completely (code {0}): {1}\n\nReinstall the fix pack manually before starting the game.");
            if (!quiet)
                MessageBoxW(IntPtr.Zero, string.Format(CultureInfo.InvariantCulture, text, code, reason),
                    "In the Line of Duty Fixes", FailureBox);
            if (restartable)
                Restart(restartExe, restartArgs);
            return code;
        }

        private static void WriteResult(string game, int code, string reason)
        {
            try
            {
                string path = Path.Combine(game, ResultPath.Replace('/', Path.DirectorySeparatorChar));
                WriteTextAtomic(path, "code=" + code.ToString(CultureInfo.InvariantCulture) + "\nreason=" +
                    (reason ?? string.Empty).Replace("\r", " ").Replace("\n", " "));
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }

        private static void Restart(string exe, string arguments)
        {
            try
            {
                Process.Start(new ProcessStartInfo(exe, arguments)
                {
                    UseShellExecute = false,
                    WorkingDirectory = Path.GetDirectoryName(exe)
                });
            }
            catch (System.ComponentModel.Win32Exception) { }
            catch (InvalidOperationException) { }
        }

        private static void RejectPatch(string game, Version version, string reason)
        {
            WriteTextAtomic(Path.Combine(game, ".ild-fixes", "patch-rejected.txt"), version.ToString());
            throw new Failure(24, reason);
        }

        private static HashSet<string> PayloadPaths(Manifest manifest)
        {
            HashSet<string> result = new HashSet<string>(manifest.Files.Select(delegate(ManifestFile file)
            {
                return file.Relative;
            }), StringComparer.OrdinalIgnoreCase);
            result.Add(ManifestPath);
            return result;
        }

        private static Dictionary<string, ZipArchiveEntry> ValidateArchive(ZipArchive zip)
        {
            if (zip.Entries.Count > MaximumFiles * 2)
                throw new InvalidDataException("The update archive contains too many entries.");
            Dictionary<string, ZipArchiveEntry> result =
                new Dictionary<string, ZipArchiveEntry>(StringComparer.OrdinalIgnoreCase);
            long expanded = 0;
            int files = 0;
            foreach (ZipArchiveEntry entry in zip.Entries)
            {
                string rawPath = entry.FullName.Replace('\\', '/');
                bool directory = rawPath.EndsWith("/", StringComparison.Ordinal);
                string relative = Normalize(directory ? rawPath.TrimEnd('/') : rawPath);
                if (directory)
                    continue;
                if (++files > MaximumFiles || entry.Length < 0 || (expanded += entry.Length) > MaximumExpandedBytes)
                    throw new InvalidDataException("The update archive exceeded its safety limits.");
                if (!OwnedPath(relative))
                    throw new InvalidDataException("The update archive contains a non-fix-pack path.");
                if (result.ContainsKey(relative))
                    throw new InvalidDataException("The update archive contains duplicate paths.");
                result.Add(relative, entry);
            }
            if (!result.ContainsKey(ManifestPath))
                throw new InvalidDataException("The update manifest is missing.");
            return result;
        }

        private static Manifest ReadManifest(ZipArchiveEntry entry)
        {
            if (entry.Length > 1024 * 1024)
                throw new InvalidDataException("The update manifest is too large.");
            string text;
            using (StreamReader reader = new StreamReader(entry.Open(), new UTF8Encoding(false, true)))
                text = reader.ReadToEnd();
            return ParseManifest(text);
        }

        private static Manifest ParseManifest(string text)
        {
            string[] lines = text.Replace("\r", string.Empty).Split(new[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
            if (lines.Length < 3)
                throw new InvalidDataException("The update manifest is incomplete.");

            Manifest result = new Manifest();
            if (lines[0] == "schema=ild-fixes.update/1")
                result.Patch = false;
            else if (lines[0] == "schema=ild-fixes.update/2")
                result.Patch = true;
            else
                throw new InvalidDataException("The update manifest schema is not supported.");
            result.Version = ParseVersion(Value(lines[1], "version="));

            int firstFile = 2;
            if (result.Patch)
            {
                if (lines.Length < 5 || lines[2] != "kind=patch")
                    throw new InvalidDataException("The patch manifest is incomplete.");
                result.Base = ParseVersion(Value(lines[3], "base="));
                firstFile = 4;
            }

            HashSet<string> paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            long expanded = 0;
            for (int index = firstFile; index < lines.Length; ++index)
            {
                string[] parts = lines[index].Split('\t');
                long size;
                if (parts.Length != 3 || parts[0].Length != 64 || !parts[0].All(Uri.IsHexDigit) ||
                    !long.TryParse(parts[1], NumberStyles.None, CultureInfo.InvariantCulture, out size) || size < 0)
                    throw new InvalidDataException("The update manifest has an invalid file row.");
                string relative = Normalize(parts[2]);
                if (relative.Equals(ManifestPath, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("The update manifest cannot list itself.");
                if (!OwnedPath(relative) || !paths.Add(relative) || paths.Count > MaximumFiles ||
                    (expanded += size) > MaximumExpandedBytes)
                    throw new InvalidDataException("The update manifest contains an invalid path.");
                result.Files.Add(new ManifestFile
                {
                    Relative = relative,
                    Hash = parts[0].ToLowerInvariant(),
                    Size = size
                });
            }
            if (result.Files.Count == 0)
                throw new InvalidDataException("The update manifest declares no files.");
            string[] required = { "bin/dinput8.dll", "InTheLineOfDutyFixesUpdater.exe", VersionPath, ManagedListPath };
            if (required.Any(delegate(string path) { return !paths.Contains(path); }))
                throw new InvalidDataException("The update manifest omits a required fix-pack file.");
            return result;
        }

        private static void ValidateAndStage(
            Dictionary<string, ZipArchiveEntry> entries,
            Manifest manifest,
            string game,
            string stage,
            Version version)
        {
            HashSet<string> payload = PayloadPaths(manifest);
            foreach (string path in entries.Keys)
            {
                if (!payload.Contains(path))
                    throw new Failure(21, "The archive contains an undeclared file: " + path + ".");
            }

            foreach (ManifestFile file in manifest.Files)
            {
                ZipArchiveEntry entry;
                if (!entries.TryGetValue(file.Relative, out entry))
                {
                    if (!manifest.Patch)
                        throw new Failure(21, "The archive omits " + file.Relative + ".");
                    string installed = Destination(game, file.Relative);
                    if (!File.Exists(installed) || new FileInfo(installed).Length != file.Size ||
                        UpdateClient.Digest(installed) != file.Hash)
                        RejectPatch(game, version, "The installed " + file.Relative + " does not match the patch base.");
                    continue;
                }

                if (entry.Length != file.Size)
                    throw new Failure(21, "The archive entry " + file.Relative + " has an unexpected size.");
                string destination = Destination(stage, file.Relative);
                Extract(entry, destination);
                if (UpdateClient.Digest(destination) != file.Hash)
                    throw new Failure(21, "The archive entry " + file.Relative + " failed its SHA-256 check.");
            }
            WriteCanonicalManifest(manifest, Destination(stage, ManifestPath));

            string managedPath = Destination(stage, ManagedListPath);
            if (!File.Exists(managedPath))
                managedPath = Destination(game, ManagedListPath);
            if (!ReadManagedList(managedPath).SetEquals(payload))
                throw new Failure(21, "The ownership manifest does not match the archive contents.");

            string versionPath = Destination(stage, VersionPath);
            if (!File.Exists(versionPath))
                versionPath = Destination(game, VersionPath);
            if (File.ReadAllText(versionPath).Trim() != manifest.Version.ToString())
                throw new Failure(21, "The version file does not match the manifest.");
        }

        // The installed manifest always takes the full-archive form, so patch and full installs end up identical.
        private static void WriteCanonicalManifest(Manifest manifest, string destination)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destination));
            StringBuilder text = new StringBuilder();
            text.Append("schema=ild-fixes.update/1\nversion=").Append(manifest.Version).Append('\n');
            IEnumerable<ManifestFile> ordered = manifest.Files.OrderBy(
                delegate(ManifestFile entry) { return entry.Relative; }, StringComparer.OrdinalIgnoreCase);
            foreach (ManifestFile file in ordered)
            {
                text.Append(file.Hash).Append('\t').Append(file.Size.ToString(CultureInfo.InvariantCulture))
                    .Append('\t').Append(file.Relative).Append('\n');
            }
            File.WriteAllText(destination, text.ToString(), new UTF8Encoding(false));
        }

        private static void Extract(ZipArchiveEntry entry, string destination)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destination));
            using (Stream input = entry.Open())
            using (FileStream output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                input.CopyTo(output);
        }

        // Only bytes that still match the installed manifest are fix-pack property; anything else was put there
        // by someone else and is never replaced. Installations without a manifest keep path-based ownership.
        private static void VerifyInstalledFiles(string game, HashSet<string> previous, Version installedVersion)
        {
            string manifestPath = Destination(game, ManifestPath);
            if (installedVersion == null || !File.Exists(manifestPath))
                return;
            Manifest installed;
            try
            {
                installed = ParseManifest(File.ReadAllText(manifestPath, new UTF8Encoding(false, true)));
            }
            catch (InvalidDataException) { return; }
            catch (IOException) { return; }
            if (installed.Version != installedVersion)
                return;
            foreach (ManifestFile file in installed.Files)
            {
                if (!previous.Contains(file.Relative))
                    continue;
                string path = Destination(game, file.Relative);
                if (!File.Exists(path))
                    continue;
                if (new FileInfo(path).Length != file.Size || UpdateClient.Digest(path) != file.Hash)
                    throw new Failure(27, "The installed " + file.Relative +
                        " was modified outside the fix pack; refusing to replace it.");
            }
        }

        private static void RemoveOrphanTemporaries(string game, HashSet<string> target)
        {
            foreach (string relative in target)
            {
                string destination = Destination(game, relative);
                string directory = Path.GetDirectoryName(destination);
                if (!Directory.Exists(directory))
                    continue;
                string[] stale;
                try { stale = Directory.GetFiles(directory, Path.GetFileName(destination) + ".ild-update-*.tmp"); }
                catch (IOException) { continue; }
                foreach (string file in stale)
                    TryDelete(file);
            }
        }

        private static HashSet<string> ReadManagedFiles(string game)
        {
            string path = Path.Combine(game, ManagedListPath.Replace('/', Path.DirectorySeparatorChar));
            return File.Exists(path) ? ReadManagedList(path) : new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        }

        private static HashSet<string> ReadManagedList(string path)
        {
            HashSet<string> result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string line in File.ReadAllLines(path))
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;
                string relative = Normalize(line.Trim());
                if (!OwnedPath(relative) || !result.Add(relative))
                    throw new InvalidDataException("The managed-file list is invalid.");
            }
            return result;
        }

        private static Version ReadInstalledVersion(string game)
        {
            try
            {
                string path = Path.Combine(game, VersionPath.Replace('/', Path.DirectorySeparatorChar));
                return File.Exists(path) ? ParseVersion(File.ReadAllText(path).Trim()) : null;
            }
            catch (InvalidDataException) { return null; }
            catch (IOException) { return null; }
        }

        // The fix pack owns only its own namespaces; every other path belongs to the game, the mod, or another addon.
        private static bool OwnedPath(string value)
        {
            string path;
            try { path = Normalize(value); }
            catch (InvalidDataException) { return false; }
            string[] parts = path.Split('/');
            string name = parts[parts.Length - 1];
            StringComparison ignoreCase = StringComparison.OrdinalIgnoreCase;
            if (parts.Length == 1)
                return name.Equals("InTheLineOfDutyFixesUpdater.exe", ignoreCase) ||
                    name.Equals("README-InTheLineOfDutyFixes.txt", ignoreCase);
            if (parts.Length == 2 && parts[0].Equals(".ild-fixes", ignoreCase))
                return name.Equals("version.txt", ignoreCase) || name.Equals("managed-files.txt", ignoreCase) ||
                    name.Equals("update-manifest.txt", ignoreCase);
            if (parts.Length == 2 && parts[0].Equals("bin", ignoreCase))
                return name.Equals("dinput8.dll", ignoreCase) || name.StartsWith("ild_fixes_", ignoreCase);
            if (parts.Length == 3 && parts[0].Equals("gamedata", ignoreCase) && parts[1].Equals("scripts", ignoreCase))
                return name.StartsWith("ild_", ignoreCase) && name.EndsWith(".script", ignoreCase);
            if (parts.Length == 4 && parts[0].Equals("gamedata", ignoreCase) && parts[1].Equals("config", ignoreCase) &&
                parts[2].Equals("ui", ignoreCase))
                return name.StartsWith("ild_fixes_", ignoreCase) && name.EndsWith(".xml", ignoreCase);
            return false;
        }

        private static string Normalize(string value)
        {
            if (string.IsNullOrWhiteSpace(value) || Path.IsPathRooted(value) || value.IndexOf(':') >= 0)
                throw new InvalidDataException("An absolute path is not allowed.");
            string normalized = value.Replace('\\', '/');
            string[] parts = normalized.Split('/');
            if (parts.Any(delegate(string part)
            {
                if (part.Length == 0 || part == "." || part == ".." || part.EndsWith(".") || part.EndsWith(" ") ||
                    part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
                    return true;
                string stem = part.Split('.')[0].ToUpperInvariant();
                return stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
                    stem.Length == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) &&
                        stem[3] >= '0' && stem[3] <= '9';
            }))
                throw new InvalidDataException("A path traversal is not allowed.");
            return string.Join("/", parts);
        }

        private static string Destination(string root, string relative)
        {
            string fullRoot = FullDirectory(root);
            string destination = Path.GetFullPath(Path.Combine(fullRoot, relative.Replace('/', Path.DirectorySeparatorChar)));
            string prefix = fullRoot.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            if (!destination.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("A path escaped the installation root.");
            EnsureNoReparsePoints(fullRoot, destination);
            return destination;
        }

        private static void AtomicCopy(string source, string destination)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destination));
            string temporary = destination + ".ild-update-" + Guid.NewGuid().ToString("N") + ".tmp";
            File.Copy(source, temporary, false);
            if (!MoveFileEx(temporary, destination, MoveFileReplaceExisting | MoveFileWriteThrough))
            {
                int error = Marshal.GetLastWin32Error();
                TryDelete(temporary);
                throw new IOException("Atomic replacement failed: " + error);
            }
        }

        private static bool Rollback(
            string game,
            string backup,
            IEnumerable<string> scope,
            IDictionary<string, bool> existed)
        {
            bool restored = true;
            foreach (string relative in scope)
            {
                try
                {
                    string destination = Destination(game, relative);
                    bool wasPresent;
                    if (!existed.TryGetValue(relative, out wasPresent))
                        continue;
                    if (wasPresent)
                    {
                        string saved = Destination(backup, relative);
                        if (File.Exists(saved) && (!File.Exists(destination) ||
                            UpdateClient.Digest(saved) != UpdateClient.Digest(destination)))
                            AtomicCopy(saved, destination);
                    }
                    else if (File.Exists(destination))
                    {
                        File.Delete(destination);
                    }
                }
                catch (Exception) { restored = false; }
            }
            return restored;
        }

        // The root itself may be a junction (a relocated game library); only components below it are checked.
        private static void EnsureNoReparsePoints(string root, string path)
        {
            string fullRoot = FullDirectory(root);
            string current = Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar);
            while (current.Length > fullRoot.Length && current.StartsWith(fullRoot, StringComparison.OrdinalIgnoreCase))
            {
                if ((File.Exists(current) || Directory.Exists(current)) &&
                    (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                    throw new Failure(19, "A reparse point is not a valid fix-pack path: " + current);
                current = Path.GetDirectoryName(current);
                if (string.IsNullOrEmpty(current))
                    break;
            }
        }

        private static bool ValidDigest(string digest)
        {
            return digest != null && digest.StartsWith("sha256:", StringComparison.OrdinalIgnoreCase) &&
                digest.Length == 71 && digest.Substring(7).All(Uri.IsHexDigit);
        }

        // Returns false only when the process is still alive after the timeout; a recycled identifier counts as exited.
        private static bool WaitForProcess(int processId, long startFileTime, TimeSpan timeout)
        {
            if (processId <= 0)
                return true;
            try
            {
                using (Process process = Process.GetProcessById(processId))
                {
                    if (startFileTime > 0)
                    {
                        long actual;
                        try { actual = process.StartTime.ToFileTimeUtc(); }
                        catch (InvalidOperationException) { return true; }
                        catch (System.ComponentModel.Win32Exception) { return true; }
                        if (actual != startFileTime)
                            return true;
                    }
                    return process.WaitForExit((int)timeout.TotalMilliseconds);
                }
            }
            catch (ArgumentException) { return true; }
            catch (InvalidOperationException) { return true; }
        }

        private static void RecreateDirectory(string path)
        {
            if (Directory.Exists(path))
                Directory.Delete(path, true);
            Directory.CreateDirectory(path);
        }

        private static void WriteTextAtomic(string path, string text)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            string temporary = path + ".tmp";
            File.WriteAllText(temporary, text + Environment.NewLine, new UTF8Encoding(false));
            if (!MoveFileEx(temporary, path, MoveFileReplaceExisting | MoveFileWriteThrough))
            {
                int error = Marshal.GetLastWin32Error();
                TryDelete(temporary);
                throw new IOException("Atomic write failed: " + error);
            }
        }

        private static void TryDelete(string path)
        {
            try
            {
                if (File.Exists(path))
                    File.Delete(path);
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }

        private static string Value(string line, string prefix)
        {
            if (!line.StartsWith(prefix, StringComparison.Ordinal) || line.Length == prefix.Length)
                throw new InvalidDataException("The update manifest header is invalid.");
            return line.Substring(prefix.Length);
        }

        private static Version ParseVersion(string value)
        {
            Version version;
            if (!Version.TryParse(value, out version) || version.Build < 0 || version.Revision >= 0 ||
                version.ToString() != value)
                throw new InvalidDataException("The update version is invalid.");
            return version;
        }

        private static string FullDirectory(string path)
        {
            return Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        }

        internal static string Quote(string value)
        {
            if (value.Length > 0 && value.IndexOfAny(new[] { ' ', '\t', '"' }) < 0)
                return value;
            StringBuilder result = new StringBuilder("\"");
            int slashes = 0;
            foreach (char character in value)
            {
                if (character == '\\')
                {
                    ++slashes;
                    continue;
                }
                if (character == '"')
                {
                    result.Append('\\', slashes * 2 + 1);
                    result.Append(character);
                    slashes = 0;
                    continue;
                }
                result.Append('\\', slashes);
                slashes = 0;
                result.Append(character);
            }
            result.Append('\\', slashes * 2);
            result.Append('"');
            return result.ToString();
        }
    }
}
