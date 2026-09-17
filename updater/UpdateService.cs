using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net;
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
        private readonly string lastCheckPath;
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
            // Survives the session cleanup: the loader copies it into the next report, so a check that failed or
            // a helper that found nothing leaves a trace the player can send.
            lastCheckPath = Path.Combine(runtime, "update-last.txt");
        }

        internal int Run(bool checkOnly, bool downloadOnly)
        {
            Directory.CreateDirectory(runtime);
            if ((File.GetAttributes(runtime) & FileAttributes.ReparsePoint) != 0)
                return 4;
            if (!OpenGame())
            {
                WriteLastCheck("no_game", string.Empty);
                return 6;
            }
            RemoveStaleSessionFiles();
            UpdateApplier.RemoveRetiredFiles(context.GameDirectory, UpdateApplier.ReadManagedFiles(context.GameDirectory));
            if (File.Exists(commandPath))
                File.Delete(commandPath);
            WriteStatus();
            // The release list comes from an API that allows sixty unauthenticated requests an hour per address,
            // so a player behind a shared address is refused more often than not. A refusal or a broken connection
            // is retried while the game runs - after the pause the API names, or a short one - and when every try
            // fails the release list published beside the repository is read instead.
            int attempt = 0;
            while (true)
            {
                try
                {
                    result = client.Check();
                    break;
                }
                catch (Exception error)
                {
                    lastError = error.Message;
                    TimeSpan delay;
                    if (attempt < 2 && RetryDelay(error, attempt, out delay) && WaitWithHeartbeat(delay))
                    {
                        ++attempt;
                        continue;
                    }
                    result = TryFallback();
                    if (result == null)
                    {
                        state = "check_failed";
                        WriteStatus();
                        WriteLastCheck(state, lastError);
                        return 10;
                    }
                    lastError = string.Empty;
                    break;
                }
            }
            try
            {
                majorPending = result.Major != null &&
                    !File.Exists(Path.Combine(context.GameDirectory, ".ild-fixes", "major-notice-disabled"));
                state = majorPending ? "major_available" : result.Update != null ? "available" : "current";
                ReportPreviousApply();
                WriteStatus();
                WriteLastCheck(state, lastError);
            }
            catch (Exception error)
            {
                state = "check_failed";
                lastError = error.Message;
                WriteStatus();
                WriteLastCheck(state, lastError);
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
                        WriteLastCheck(state, lastError);
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

        // The pause before a check is tried again: what the API asks for after a refusal, capped at a quarter of
        // an hour, or half a minute and then two for anything else. False when the error is not worth retrying.
        private static bool RetryDelay(Exception error, int attempt, out TimeSpan delay)
        {
            delay = attempt == 0 ? TimeSpan.FromSeconds(30) : TimeSpan.FromMinutes(2);
            WebException web = error as WebException;
            HttpWebResponse response = web != null ? web.Response as HttpWebResponse : null;
            if (response == null)
                return true;
            int status = (int)response.StatusCode;
            if (status != 403 && status != 429)
                return status >= 500;
            double seconds;
            string retryAfter = response.Headers["Retry-After"];
            string reset = response.Headers["X-RateLimit-Reset"];
            if (!string.IsNullOrEmpty(retryAfter) && double.TryParse(retryAfter, NumberStyles.Float,
                CultureInfo.InvariantCulture, out seconds))
                delay = TimeSpan.FromSeconds(seconds);
            else if (!string.IsNullOrEmpty(reset) && double.TryParse(reset, NumberStyles.Float,
                CultureInfo.InvariantCulture, out seconds))
                delay = TimeSpan.FromSeconds(seconds) - (DateTime.UtcNow - new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc));
            if (delay < TimeSpan.FromSeconds(5))
                delay = TimeSpan.FromSeconds(5);
            if (delay > TimeSpan.FromMinutes(15))
                delay = TimeSpan.FromMinutes(15);
            return true;
        }

        // Waits out the pause a second at a time, keeping the status file's heartbeat alive; false once the game is gone.
        private bool WaitWithHeartbeat(TimeSpan delay)
        {
            DateTime until = DateTime.UtcNow + delay;
            while (DateTime.UtcNow < until)
            {
                if (game.HasExited)
                    return false;
                WriteStatus();
                Thread.Sleep(1000);
            }
            return !game.HasExited;
        }

        private CheckResult TryFallback()
        {
            try { return client.CheckFallback(); }
            catch (Exception) { return null; }
        }

        private void WriteLastCheck(string outcome, string error)
        {
            try
            {
                string version = result != null && result.Update != null ? result.Update.Version.ToString() :
                    result != null && result.Major != null ? result.Major.Version.ToString() : ProductInfo.VersionText;
                string text = "time=" + DateTime.UtcNow.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture) +
                    "\npid=" + context.GamePid.ToString(CultureInfo.InvariantCulture) +
                    "\ninstalled=" + ProductInfo.VersionText + "\nstate=" + outcome + "\nversion=" + version +
                    "\nerror=" + (error ?? string.Empty).Replace("\r", " ").Replace("\n", " ") + "\n";
                string temporary = lastCheckPath + ".tmp";
                File.WriteAllText(temporary, text, new UTF8Encoding(false));
                if (File.Exists(lastCheckPath))
                    File.Replace(temporary, lastCheckPath, null);
                else
                    File.Move(temporary, lastCheckPath);
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
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
            catch (UnauthorizedAccessException) { return; }
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
                UpdateOffer offer = result.Update;
                archive = client.Download(offer, ReportProgress);
                // The client presumes a patch was cut against the release listed before its target. One that does not
                // fit this installation is swapped for the full archive now, while the game still runs.
                if (offer.IsPatch && !UpdateApplier.PatchApplies(context.GameDirectory, archive))
                {
                    UpdateApplier.MarkPatchRejected(context.GameDirectory, offer.Version);
                    result.Update = new UpdateOffer
                    {
                        Version = offer.Version,
                        Asset = offer.FullAsset,
                        FullAsset = offer.FullAsset,
                        Notes = offer.Notes
                    };
                    downloaded = 0;
                    WriteStatus();
                    archive = client.Download(result.Update, ReportProgress);
                }
                downloaded = new FileInfo(archive).Length;
                state = "ready";
                WriteStatus();
            }
            catch (Exception error)
            {
                state = "download_failed";
                lastError = error.Message;
                WriteStatus();
                WriteLastCheck(state, lastError);
            }
        }

        private void ReportProgress(DownloadProgress progress)
        {
            downloaded = progress.Downloaded;
            if (Environment.TickCount - lastProgress >= 200 || progress.Downloaded == progress.Total)
            {
                lastProgress = Environment.TickCount;
                WriteStatus();
            }
        }

        private int StartApply()
        {
            try
            {
                UpdateOffer offer = result.Update;
                string runner = Path.Combine(Path.GetDirectoryName(archive), "IldFixesUpdater.cached.exe");
                // The updater inside the archive applies it, so a release that changes what an update may contain
                // still reaches every older installation in one step. That updater keeps accepting these arguments
                // and writes the apply result and the patch rejection the way this service reads them. A patch that
                // carries no updater keeps the installed one, which then is the release's own.
                if (!UpdateApplier.ExtractUpdater(archive, offer.Asset.Size, offer.Asset.Digest, runner))
                    File.Copy(Assembly.GetExecutingAssembly().Location, runner, true);
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
                WriteLastCheck(state, string.Empty);
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
