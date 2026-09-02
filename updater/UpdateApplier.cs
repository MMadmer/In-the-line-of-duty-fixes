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
        private const int MaximumFiles = 1024;
        private const long MaximumExpandedBytes = 1024L * 1024 * 1024;
        private const int MoveFileReplaceExisting = 0x1;
        private const int MoveFileWriteThrough = 0x8;

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

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool MoveFileEx(string existing, string replacement, int flags);

        internal static int Apply(Arguments arguments)
        {
            string game = FullDirectory(arguments.Required("--game-dir"));
            string archive = Path.GetFullPath(arguments.Required("--archive"));
            Version version = ParseVersion(arguments.Required("--version"));
            string digest = arguments.Required("--digest");
            long size = arguments.RequiredLong("--size");
            int waitPid = arguments.RequiredInt("--wait-pid");
            string restartExe = arguments.Required("--restart-exe");
            string restartArgs = arguments.Optional("--restart-args") ?? string.Empty;

            string expectedCache = Path.Combine(game, ".ild-fixes", "update-cache", version.ToString());
            if (!string.Equals(Path.GetDirectoryName(archive), expectedCache, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(Path.GetFullPath(restartExe), Path.Combine(game, "bin", "XR_3DA.exe"),
                    StringComparison.OrdinalIgnoreCase) || !ValidDigest(digest))
                return 19;
            EnsureNoReparsePoints(game, archive);

            if (!File.Exists(archive) || new FileInfo(archive).Length != size ||
                UpdateClient.Digest(archive) != UpdateClient.NormalizeDigest(digest))
                return 20;

            WaitForProcess(waitPid, TimeSpan.FromMinutes(2));
            string cache = Path.GetDirectoryName(archive);
            string stage = Path.Combine(cache, "stage");
            string backup = Path.Combine(cache, "backup");
            RecreateDirectory(stage);
            RecreateDirectory(backup);

            Manifest manifest;
            Dictionary<string, ZipArchiveEntry> entries;
            try
            {
                using (ZipArchive zip = ZipFile.OpenRead(archive))
                {
                    entries = ValidateArchive(zip);
                    manifest = ReadManifest(entries["update-manifest.txt"]);
                    if (manifest.Version != version)
                        return 21;
                    int validation = ValidateAndStage(zip, entries, manifest, game, stage);
                    if (validation != 0)
                    {
                        if (validation == 24)
                            WriteTextAtomic(Path.Combine(game, ".ild-fixes", "patch-rejected.txt"), version.ToString());
                        return validation;
                    }
                }
            }
            catch
            {
                return 21;
            }

            HashSet<string> previous = ReadManagedFiles(game);
            HashSet<string> target = new HashSet<string>(manifest.Files.Select(delegate(ManifestFile file)
            {
                return file.Relative;
            }), StringComparer.OrdinalIgnoreCase);
            HashSet<string> scope = new HashSet<string>(previous, StringComparer.OrdinalIgnoreCase);
            scope.UnionWith(target);
            Dictionary<string, bool> existed = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);

            foreach (string relative in target)
            {
                string destination = Destination(game, relative);
                EnsureNoReparsePoints(game, destination);
                if (File.Exists(destination) && !previous.Contains(relative))
                    return 26;
            }

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

                foreach (ManifestFile file in manifest.Files)
                {
                    string staged = Destination(stage, file.Relative);
                    if (File.Exists(staged))
                        AtomicCopy(staged, Destination(game, file.Relative));
                }

                foreach (string relative in previous)
                {
                    if (!target.Contains(relative))
                    {
                        string dropped = Destination(game, relative);
                        if (File.Exists(dropped))
                            File.Delete(dropped);
                    }
                }

                foreach (ManifestFile file in manifest.Files)
                {
                    string installed = Destination(game, file.Relative);
                    if (!File.Exists(installed) || new FileInfo(installed).Length != file.Size ||
                        UpdateClient.Digest(installed) != file.Hash)
                        throw new InvalidDataException("The installed payload failed its final verification.");
                }
            }
            catch
            {
                return Rollback(game, backup, scope, existed) ? 22 : 25;
            }

            string rejected = Path.Combine(game, ".ild-fixes", "patch-rejected.txt");
            if (File.Exists(rejected))
                File.Delete(rejected);

            string installedUpdater = Path.Combine(game, "InTheLineOfDutyFixesUpdater.exe");
            if (!File.Exists(installedUpdater))
            {
                return Rollback(game, backup, scope, existed) ? 23 : 25;
            }

            string finishArguments = "--finish --game-dir " + Quote(game) + " --cache " + Quote(cache) +
                " --wait-pid " + Process.GetCurrentProcess().Id.ToString(CultureInfo.InvariantCulture) +
                " --restart-exe " + Quote(restartExe) + " --restart-args " + Quote(restartArgs);
            Process.Start(new ProcessStartInfo(installedUpdater, finishArguments)
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = game
            });
            return 0;
        }

        internal static int Finish(Arguments arguments)
        {
            string game = FullDirectory(arguments.Required("--game-dir"));
            string cache = Path.GetFullPath(arguments.Required("--cache"));
            int waitPid = arguments.RequiredInt("--wait-pid");
            string restartExe = arguments.Required("--restart-exe");
            string restartArgs = arguments.Optional("--restart-args") ?? string.Empty;
            string cacheRoot = FullDirectory(Path.Combine(game, ".ild-fixes", "update-cache"));
            string cacheWithSeparator = cacheRoot.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            if (!cache.StartsWith(cacheWithSeparator, StringComparison.OrdinalIgnoreCase))
                return 30;
            if (!string.Equals(Path.GetDirectoryName(cache), cacheRoot, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(Path.GetFullPath(restartExe), Path.Combine(game, "bin", "XR_3DA.exe"),
                    StringComparison.OrdinalIgnoreCase))
                return 30;
            ParseVersion(Path.GetFileName(cache));
            EnsureNoReparsePoints(game, cache);

            WaitForProcess(waitPid, TimeSpan.FromMinutes(1));
            if (Directory.Exists(cache))
                Directory.Delete(cache, true);
            if (Directory.Exists(cacheRoot) && !Directory.EnumerateFileSystemEntries(cacheRoot).Any())
                Directory.Delete(cacheRoot);

            Process.Start(new ProcessStartInfo(restartExe, restartArgs)
            {
                UseShellExecute = false,
                WorkingDirectory = Path.GetDirectoryName(restartExe)
            });
            return 0;
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
                if (relative != "update-manifest.txt" && !OwnedPath(relative))
                    throw new InvalidDataException("The update archive contains a non-fix-pack path.");
                if (result.ContainsKey(relative))
                    throw new InvalidDataException("The update archive contains duplicate paths.");
                result.Add(relative, entry);
            }
            if (!result.ContainsKey("update-manifest.txt"))
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
            string[] required = { "bin/dinput8.dll", "InTheLineOfDutyFixesUpdater.exe",
                ".ild-fixes/version.txt", ".ild-fixes/managed-files.txt" };
            if (required.Any(delegate(string path) { return !paths.Contains(path); }))
                throw new InvalidDataException("The update manifest omits a required fix-pack file.");
            return result;
        }

        private static int ValidateAndStage(
            ZipArchive zip,
            Dictionary<string, ZipArchiveEntry> entries,
            Manifest manifest,
            string game,
            string stage)
        {
            HashSet<string> declared = new HashSet<string>(manifest.Files.Select(delegate(ManifestFile file)
            {
                return file.Relative;
            }), StringComparer.OrdinalIgnoreCase);
            foreach (string path in entries.Keys)
            {
                if (path != "update-manifest.txt" && !declared.Contains(path))
                    return 21;
            }

            foreach (ManifestFile file in manifest.Files)
            {
                ZipArchiveEntry entry;
                if (!entries.TryGetValue(file.Relative, out entry))
                {
                    string installed = Destination(game, file.Relative);
                    if (!manifest.Patch || !File.Exists(installed) || new FileInfo(installed).Length != file.Size ||
                        UpdateClient.Digest(installed) != file.Hash)
                        return manifest.Patch ? 24 : 21;
                    continue;
                }

                if (entry.Length != file.Size)
                    return 21;
                string destination = Destination(stage, file.Relative);
                Directory.CreateDirectory(Path.GetDirectoryName(destination));
                using (Stream input = entry.Open())
                using (FileStream output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                    input.CopyTo(output);
                if (UpdateClient.Digest(destination) != file.Hash)
                    return 21;
            }

            string managedPath = Destination(stage, ".ild-fixes/managed-files.txt");
            if (!File.Exists(managedPath))
                managedPath = Destination(game, ".ild-fixes/managed-files.txt");
            HashSet<string> managed = new HashSet<string>(File.ReadAllLines(managedPath)
                .Where(delegate(string line) { return !string.IsNullOrWhiteSpace(line); })
                .Select(delegate(string line) { return Normalize(line.Trim()); }), StringComparer.OrdinalIgnoreCase);
            if (!managed.SetEquals(declared))
                return 21;

            string versionPath = Destination(stage, ".ild-fixes/version.txt");
            if (!File.Exists(versionPath))
                versionPath = Destination(game, ".ild-fixes/version.txt");
            if (File.ReadAllText(versionPath).Trim() != manifest.Version.ToString())
                return 21;
            return 0;
        }

        private static HashSet<string> ReadManagedFiles(string game)
        {
            string path = Path.Combine(game, ".ild-fixes", "managed-files.txt");
            HashSet<string> result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (!File.Exists(path))
                return result;
            foreach (string line in File.ReadAllLines(path))
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;
                string relative = Normalize(line.Trim());
                if (!OwnedPath(relative) || !result.Add(relative))
                    throw new InvalidDataException("The installed managed-file list is invalid.");
            }
            return result;
        }

        private static bool OwnedPath(string value)
        {
            string path;
            try { path = Normalize(value); }
            catch { return false; }
            return path.Equals("bin/dinput8.dll", StringComparison.OrdinalIgnoreCase) ||
                path.Equals("gamedata/scripts/ild_fix_ui.script", StringComparison.OrdinalIgnoreCase) ||
                path.Equals("gamedata/scripts/ild_gameplay.script", StringComparison.OrdinalIgnoreCase) ||
                path.Equals("gamedata/config/ui/ild_fixes_update.xml", StringComparison.OrdinalIgnoreCase) ||
                path.StartsWith("bin/ild_fixes_", StringComparison.OrdinalIgnoreCase) ||
                path.Equals("InTheLineOfDutyFixesUpdater.exe", StringComparison.OrdinalIgnoreCase) ||
                path.Equals("README-InTheLineOfDutyFixes.txt", StringComparison.OrdinalIgnoreCase) ||
                path.Equals(".ild-fixes/version.txt", StringComparison.OrdinalIgnoreCase) ||
                path.Equals(".ild-fixes/managed-files.txt", StringComparison.OrdinalIgnoreCase);
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
                if (File.Exists(temporary))
                    File.Delete(temporary);
                throw new IOException("Atomic replacement failed: " + Marshal.GetLastWin32Error());
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
                catch { restored = false; }
            }
            return restored;
        }

        private static void EnsureNoReparsePoints(string root, string path)
        {
            string fullRoot = FullDirectory(root);
            string current = Path.GetFullPath(path);
            while (current.Length >= fullRoot.Length)
            {
                if ((File.Exists(current) || Directory.Exists(current)) &&
                    (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                    throw new InvalidDataException("Reparse points are not allowed in managed paths.");
                if (string.Equals(current, fullRoot, StringComparison.OrdinalIgnoreCase))
                    break;
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

        private static void WaitForProcess(int processId, TimeSpan timeout)
        {
            if (processId <= 0)
                return;
            try
            {
                using (Process process = Process.GetProcessById(processId))
                {
                    if (!process.WaitForExit((int)timeout.TotalMilliseconds))
                        throw new TimeoutException("The game did not exit in time.");
                }
            }
            catch (ArgumentException) { }
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
            if (File.Exists(path))
                File.Delete(path);
            File.Move(temporary, path);
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
