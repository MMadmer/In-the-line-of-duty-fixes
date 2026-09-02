using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace IldFixes.Updater
{
    internal sealed class UpdateService
    {
        private const int HeartbeatIntervalMs = 1000;
        private const int MaximumChangesChars = 30000;
        private const int MaximumThemeChars = 255;

        private readonly object sync = new object();
        private readonly LaunchContext context;
        private readonly UpdateClient client;
        private readonly string runtime;
        private readonly string statusPath;
        private readonly string commandPath;
        private readonly string resultPath;
        private Process game;
        private long gameStart;
        private CheckResult result;
        private string state = "checking";
        private string lastError = string.Empty;
        private string archive;
        private long downloaded;
        private long lastProgress;
        private bool majorPending;

        [DllImport("kernel32.dll")]
        private static extern ulong GetTickCount64();

        internal UpdateService(LaunchContext context)
        {
            this.context = context;
            client = new UpdateClient(context);
            runtime = Path.Combine(context.GameDirectory, ".ild-fixes", "runtime");
            // Every game process owns its own status/command pair, so two running games never read each other's files.
            string session = context.GamePid.ToString(CultureInfo.InvariantCulture);
            statusPath = Path.Combine(runtime, "status-" + session + ".txt");
            commandPath = Path.Combine(runtime, "command-" + session + ".txt");
            resultPath = Path.Combine(runtime, "apply-result.txt");
        }

        internal int Run(bool checkOnly, bool downloadOnly)
        {
            Directory.CreateDirectory(runtime);
            if ((File.GetAttributes(runtime) & FileAttributes.ReparsePoint) != 0)
                return 4;
            if (!OpenGame())
                return 6;
            RemoveStaleSessionFiles();
            if (File.Exists(commandPath))
                File.Delete(commandPath);
            WriteStatus();
            try
            {
                result = client.Check();
                majorPending = result.Major != null &&
                    !File.Exists(Path.Combine(context.GameDirectory, ".ild-fixes", "major-notice-disabled"));
                state = majorPending ? "major_available" : result.Update != null ? "available" : "current";
                ReportPreviousApply();
                WriteStatus();
            }
            catch (Exception error)
            {
                state = "check_failed";
                lastError = error.Message;
                WriteStatus();
                return 10;
            }

            if (checkOnly)
                return context.Qa ? 0 : 5;
            if (downloadOnly)
            {
                if (!context.Qa || result.Update == null) return 5;
                majorPending = false;
                Download();
                return state == "ready" ? 0 : 11;
            }
            // Nothing to offer: leave the status behind for the menu and stop instead of polling for hours.
            if (state == "current")
                return 0;

            using (new Timer(delegate { WriteStatus(); }, null, HeartbeatIntervalMs, HeartbeatIntervalMs))
            {
                while (!game.HasExited)
                {
                    string command = ReadCommand();
                    if (command == "dismiss_major" || command == "disable_major")
                    {
                        if (command == "disable_major")
                            File.WriteAllText(Path.Combine(context.GameDirectory, ".ild-fixes", "major-notice-disabled"), "1");
                        majorPending = false;
                        state = result.Update != null ? "available" : "current";
                        WriteStatus();
                        if (state == "current")
                            return 0;
                    }
                    else if (command == "open_major" && majorPending)
                    {
                        Uri page;
                        if (Uri.TryCreate(result.Major.PageUrl, UriKind.Absolute, out page) &&
                            page.Scheme == Uri.UriSchemeHttps && page.Host == "github.com")
                            Process.Start(page.AbsoluteUri);
                    }
                    else if (command == "dismiss" &&
                        (state == "available" || state == "download_failed" || state == "apply_failed"))
                    {
                        state = "dismissed";
                        WriteStatus();
                        return 0;
                    }
                    else if (command == "download" &&
                        (state == "available" || state == "download_failed" || state == "apply_failed"))
                        Download();
                    else if (command == "apply" && state == "ready")
                    {
                        if (StartApply() == 0) return 0;
                    }
                    game.WaitForExit(100);
                }
            }
            return 0;
        }

        private bool OpenGame()
        {
            try
            {
                // Holding the process handle keeps the identifier from being recycled while the service runs.
                game = Process.GetProcessById(context.GamePid);
                try { gameStart = game.StartTime.ToFileTimeUtc(); }
                catch (InvalidOperationException) { gameStart = 0; }
                catch (System.ComponentModel.Win32Exception) { gameStart = 0; }
                return !game.HasExited;
            }
            catch (ArgumentException) { return false; }
            catch (InvalidOperationException) { return false; }
        }

        private void RemoveStaleSessionFiles()
        {
            foreach (string legacy in new[] { "status.txt", "command.txt" })
                TryDelete(Path.Combine(runtime, legacy));
            foreach (string prefix in new[] { "status-", "command-" })
            {
                string[] files;
                try { files = Directory.GetFiles(runtime, prefix + "*.txt"); }
                catch (IOException) { continue; }
                foreach (string file in files)
                {
                    string name = Path.GetFileNameWithoutExtension(file).Substring(prefix.Length);
                    int pid;
                    if (!int.TryParse(name, NumberStyles.None, CultureInfo.InvariantCulture, out pid) ||
                        pid == context.GamePid || ProcessAlive(pid))
                        continue;
                    TryDelete(file);
                }
            }
        }

        private static bool ProcessAlive(int pid)
        {
            try
            {
                using (Process process = Process.GetProcessById(pid))
                    return !process.HasExited;
            }
            catch (ArgumentException) { return false; }
            catch (InvalidOperationException) { return false; }
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

        // A failed apply leaves its code and reason for the next session; the offer then starts in the retry state.
        private void ReportPreviousApply()
        {
            if (!File.Exists(resultPath))
                return;
            string reason = string.Empty;
            int code = 0;
            try
            {
                foreach (string line in File.ReadAllLines(resultPath, Encoding.UTF8))
                {
                    if (line.StartsWith("code=", StringComparison.Ordinal))
                        int.TryParse(line.Substring(5), NumberStyles.None, CultureInfo.InvariantCulture, out code);
                    else if (line.StartsWith("reason=", StringComparison.Ordinal))
                        reason = line.Substring(7);
                }
            }
            catch (IOException) { return; }
            TryDelete(resultPath);
            if (code != 0 && result.Update != null && !majorPending)
            {
                state = "apply_failed";
                lastError = reason;
            }
        }

        private void Download()
        {
            state = "downloading";
            downloaded = 0;
            lastError = string.Empty;
            WriteStatus();
            try
            {
                archive = client.Download(result.Update, delegate(DownloadProgress progress)
                {
                    downloaded = progress.Downloaded;
                    if (Environment.TickCount - lastProgress >= 200 || progress.Downloaded == progress.Total)
                    {
                        lastProgress = Environment.TickCount;
                        WriteStatus();
                    }
                });
                downloaded = new FileInfo(archive).Length;
                state = "ready";
                WriteStatus();
            }
            catch (Exception error)
            {
                state = "download_failed";
                lastError = error.Message;
                WriteStatus();
            }
        }

        private int StartApply()
        {
            try
            {
                string runner = Path.Combine(Path.GetDirectoryName(archive), "IldFixesUpdater.cached.exe");
                File.Copy(Assembly.GetExecutingAssembly().Location, runner, true);
                UpdateOffer offer = result.Update;
                string arguments = "--apply --game-dir " + UpdateApplier.Quote(context.GameDirectory) +
                    " --archive " + UpdateApplier.Quote(archive) + " --version " + offer.Version +
                    " --digest " + UpdateApplier.Quote(offer.Asset.Digest) + " --size " + offer.Asset.Size +
                    " --wait-pid " + context.GamePid + " --game-start " + gameStart.ToString(CultureInfo.InvariantCulture) +
                    " --restart-exe " + UpdateApplier.Quote(context.RestartExe) +
                    " --restart-args " + UpdateApplier.Quote(context.RestartArguments) +
                    " --lang " + (context.Russian ? "ru" : "en");
                Process.Start(new ProcessStartInfo(runner, arguments)
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WorkingDirectory = context.GameDirectory
                });
                state = "apply_started";
                WriteStatus();
                return 0;
            }
            catch (Exception error)
            {
                state = "apply_failed";
                lastError = error.Message;
                WriteStatus();
                return 11;
            }
        }

        private string ReadCommand()
        {
            try
            {
                if (!File.Exists(commandPath))
                    return null;
                string[] lines = File.ReadAllLines(commandPath);
                File.Delete(commandPath);
                if (lines.Length != 2 || lines[0] != "session=" + context.GamePid)
                    return null;
                return lines[1].StartsWith("action=", StringComparison.Ordinal) ? lines[1].Substring(7) : null;
            }
            catch (IOException) { return null; }
        }

        private void WriteStatus()
        {
            lock (sync)
            {
                Dictionary<string, string> fields = new Dictionary<string, string>();
                fields["session"] = context.GamePid.ToString(CultureInfo.InvariantCulture);
                fields["heartbeat"] = GetTickCount64().ToString(CultureInfo.InvariantCulture);
                fields["state"] = state;
                fields["installed"] = ProductInfo.VersionText;
                fields["russian"] = context.Russian ? "1" : "0";
                fields["qa"] = context.Qa ? "1" : "0";
                fields["downloaded"] = downloaded.ToString(CultureInfo.InvariantCulture);
                fields["total"] = result != null && result.Update != null ?
                    result.Update.Asset.Size.ToString(CultureInfo.InvariantCulture) : "0";
                if (result != null)
                {
                    LocalizedNotes notes = majorPending ? result.Major.Notes : result.Update != null ? result.Update.Notes : null;
                    fields["version"] = majorPending ? result.Major.Version.ToString() :
                        result.Update != null ? result.Update.Version.ToString() : ProductInfo.VersionText;
                    fields["patch"] = result.Update != null && result.Update.IsPatch ? "1" : "0";
                    if (notes != null)
                    {
                        fields["theme"] = Truncate(context.Russian ? notes.ThemeRu : notes.ThemeEn, MaximumThemeChars);
                        fields["changes"] = Truncate(context.Russian ? notes.ChangesRu : notes.ChangesEn, MaximumChangesChars);
                    }
                }
                fields["error"] = lastError ?? string.Empty;
                bool ru = context.Russian;
                fields["caption"] = ru ? "Обновление In the Line of Duty Fixes" : "In the Line of Duty Fixes update";
                fields["message"] = majorPending ?
                    (ru ? "Установи крупную версию в отдельную папку. Текущие моды и сейвы останутся на месте." :
                        "Install the major version in a separate folder. Current mods and saves stay in place.") :
                    state == "ready" ? (ru ? "Архив проверен. Установка перезапустит игру." :
                        "Archive verified. Installation will restart the game.") :
                    state == "download_failed" || state == "apply_failed" ?
                        (ru ? "Не удалось обновить фикспак. Можно повторить попытку." : "Update failed. You can retry.") :
                    string.Format(ru ? "Фикспак: {0}    Доступно: {1}" : "Fix pack: {0}    Available: {1}",
                        ProductInfo.VersionText, fields.ContainsKey("version") ? fields["version"] : "...");
                if ((state == "download_failed" || state == "apply_failed") && !string.IsNullOrEmpty(lastError))
                    fields["message"] += "\n" + Truncate(lastError, 160);
                fields["size_label"] = result != null && result.Update != null ?
                    string.Format(ru ? "Размер загрузки: {0:0.00} МиБ" : "Download size: {0:0.00} MiB",
                        result.Update.Asset.Size / 1048576.0) : string.Empty;
                fields["action_label"] = majorPending ? (ru ? "Страница релиза" : "Release page") :
                    state == "ready" ? (ru ? "Установить" : "Install") :
                    state == "downloading" ? (ru ? "Загрузка..." : "Downloading...") : (ru ? "Скачать" : "Download");
                fields["cancel_label"] = ru ? "Не сейчас" : "Not now";
                fields["disable_label"] = ru ? "Не напоминать" : "Don't remind me";
                StringBuilder text = new StringBuilder();
                foreach (KeyValuePair<string, string> field in fields)
                    text.Append(field.Key).Append('=').Append(Escape(field.Value)).Append('\n');

                string temporary = statusPath + ".tmp";
                File.WriteAllText(temporary, text.ToString(), Encoding.GetEncoding(1251));
                // The game reads the file with shared delete access; a short collision only needs a retry.
                for (int attempt = 0; attempt < 50; ++attempt)
                {
                    try
                    {
                        if (File.Exists(statusPath))
                            File.Replace(temporary, statusPath, null);
                        else
                            File.Move(temporary, statusPath);
                        return;
                    }
                    catch (IOException) { Thread.Sleep(20); }
                    catch (UnauthorizedAccessException) { Thread.Sleep(20); }
                }
                TryDelete(temporary);
            }
        }

        private static string Truncate(string value, int maximum)
        {
            if (value == null)
                return string.Empty;
            return value.Length <= maximum ? value : value.Substring(0, maximum - 1) + "…";
        }

        private static string Escape(string value)
        {
            return value.Replace("%", "%25").Replace("\r", string.Empty).Replace("\n", "%0A");
        }
    }
}
