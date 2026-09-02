using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;

namespace IldFixes.Updater
{
    internal sealed class UpdateService
    {
        private readonly LaunchContext context;
        private readonly UpdateClient client;
        private readonly string runtime;
        private readonly string statusPath;
        private readonly string commandPath;
        private CheckResult result;
        private string state = "checking";
        private string archive;
        private long downloaded;
        private long lastProgress;
        private bool majorPending;

        internal UpdateService(LaunchContext context)
        {
            this.context = context;
            client = new UpdateClient(context);
            runtime = Path.Combine(context.GameDirectory, ".ild-fixes", "runtime");
            statusPath = Path.Combine(runtime, "status.txt");
            commandPath = Path.Combine(runtime, "command.txt");
        }

        internal int Run(bool checkOnly, bool downloadOnly)
        {
            Directory.CreateDirectory(runtime);
            if ((File.GetAttributes(runtime) & FileAttributes.ReparsePoint) != 0)
                return 4;
            if (File.Exists(commandPath))
                File.Delete(commandPath);
            WriteStatus(null);
            try
            {
                result = client.Check();
                majorPending = result.Major != null &&
                    !File.Exists(Path.Combine(context.GameDirectory, ".ild-fixes", "major-notice-disabled"));
                state = majorPending ? "major_available" : result.Update != null ? "available" : "current";
                WriteStatus(null);
            }
            catch (Exception error)
            {
                state = "check_failed";
                WriteStatus(error.Message);
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

            while (GameRunning())
            {
                string command = ReadCommand();
                if (command == "dismiss_major" || command == "disable_major")
                {
                    if (command == "disable_major")
                        File.WriteAllText(Path.Combine(context.GameDirectory, ".ild-fixes", "major-notice-disabled"), "1");
                    majorPending = false;
                    state = result.Update != null ? "available" : "current";
                    WriteStatus(null);
                }
                else if (command == "open_major" && majorPending)
                {
                    Uri page;
                    if (Uri.TryCreate(result.Major.PageUrl, UriKind.Absolute, out page) &&
                        page.Scheme == Uri.UriSchemeHttps && page.Host == "github.com")
                        Process.Start(page.AbsoluteUri);
                }
                else if (command == "dismiss" && (state == "available" || state == "download_failed"))
                {
                    state = "dismissed";
                    WriteStatus(null);
                    return 0;
                }
                else if (command == "download" && (state == "available" || state == "download_failed" || state == "apply_failed"))
                    Download();
                else if (command == "apply" && state == "ready")
                {
                    if (StartApply() == 0) return 0;
                }
                Thread.Sleep(100);
            }
            return 0;
        }

        private void Download()
        {
            state = "downloading";
            downloaded = 0;
            WriteStatus(null);
            try
            {
                archive = client.Download(result.Update, delegate(DownloadProgress progress)
                {
                    downloaded = progress.Downloaded;
                    if (Environment.TickCount - lastProgress >= 200 || progress.Downloaded == progress.Total)
                    {
                        lastProgress = Environment.TickCount;
                        WriteStatus(null);
                    }
                });
                state = "ready";
                WriteStatus(null);
            }
            catch (Exception error)
            {
                state = "download_failed";
                WriteStatus(error.Message);
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
                    " --wait-pid " + context.GamePid + " --restart-exe " + UpdateApplier.Quote(context.RestartExe) +
                    " --restart-args " + UpdateApplier.Quote(context.RestartArguments);
                Process.Start(new ProcessStartInfo(runner, arguments)
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WorkingDirectory = context.GameDirectory
                });
                state = "apply_started";
                WriteStatus(null);
                return 0;
            }
            catch (Exception error)
            {
                state = "apply_failed";
                WriteStatus(error.Message);
                return 11;
            }
        }

        private bool GameRunning()
        {
            try
            {
                using (Process game = Process.GetProcessById(context.GamePid))
                    return !game.HasExited;
            }
            catch (ArgumentException) { return false; }
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

        private void WriteStatus(string error)
        {
            Dictionary<string, string> fields = new Dictionary<string, string>();
            fields["session"] = context.GamePid.ToString(CultureInfo.InvariantCulture);
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
                    fields["theme"] = context.Russian ? notes.ThemeRu : notes.ThemeEn;
                    fields["changes"] = context.Russian ? notes.ChangesRu : notes.ChangesEn;
                }
            }
            fields["error"] = error ?? string.Empty;
            bool ru = context.Russian;
            fields["caption"] = majorPending ? (ru ? "Новая крупная версия" : "New major version") :
                (ru ? "Обновление фиксов «По долгу службы»" : "In the Line of Duty Fixes update");
            fields["message"] = majorPending ?
                (ru ? "Установи крупную версию в отдельную папку. Текущие моды и сейвы останутся на месте." :
                    "Install the major version in a separate folder. Current mods and saves stay in place.") :
                state == "ready" ? (ru ? "Архив проверен. Установка перезапустит игру." :
                    "Archive verified. Installation will restart the game.") :
                state == "download_failed" || state == "apply_failed" ?
                    (ru ? "Не удалось обновить фикспак. Можно повторить попытку." : "Update failed. You can retry.") :
                string.Format(ru ? "Установлено: {0}    Доступно: {1}" : "Installed: {0}    Available: {1}",
                    ProductInfo.VersionText, fields.ContainsKey("version") ? fields["version"] : "...");
            fields["size_label"] = result != null && result.Update != null ?
                string.Format(ru ? "Размер загрузки: {0:0.00} МиБ" : "Download size: {0:0.00} MiB",
                    result.Update.Asset.Size / 1048576.0) : string.Empty;
            fields["action_label"] = majorPending ? (ru ? "Страница релиза" : "Release page") :
                state == "ready" ? (ru ? "Установить и перезапустить" : "Install and restart") :
                state == "downloading" ? (ru ? "Загрузка..." : "Downloading...") : (ru ? "Скачать" : "Download");
            fields["cancel_label"] = ru ? "Не сейчас" : "Not now";
            fields["disable_label"] = ru ? "Не уведомлять о крупных версиях" : "Disable major-version notices";
            StringBuilder text = new StringBuilder();
            foreach (KeyValuePair<string, string> field in fields)
                text.Append(field.Key).Append('=').Append(Escape(field.Value)).Append('\n');

            string temporary = statusPath + ".tmp";
            File.WriteAllText(temporary, text.ToString(), Encoding.GetEncoding(1251));
            for (int attempt = 0; attempt < 10; ++attempt)
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
            }
        }

        private static string Escape(string value)
        {
            return value.Replace("%", "%25").Replace("\r", string.Empty).Replace("\n", "%0A");
        }
    }
}
