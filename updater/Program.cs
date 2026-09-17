using System;
using System.Globalization;
using System.IO;
using System.Text;

namespace IldFixes.Updater
{
    internal static class Program
    {
        private static int Main(string[] source)
        {
            string gameDirectory = null;
            try
            {
                Arguments arguments = Arguments.Parse(source);
                gameDirectory = arguments.Optional("--game-dir");
                if (arguments.HasFlag("--apply"))
                    return UpdateApplier.Apply(arguments);
                if (arguments.HasFlag("--finish"))
                    return UpdateApplier.Finish(arguments);
                if (!arguments.HasFlag("--service") && !arguments.HasFlag("--qa-check") && !arguments.HasFlag("--qa-download"))
                    return 7;

                LaunchContext context = new LaunchContext();
                context.GameDirectory = Path.GetFullPath(arguments.Required("--game-dir"));
                context.GamePid = arguments.RequiredInt("--game-pid");
                context.RestartExe = Path.GetFullPath(arguments.Required("--restart-exe"));
                context.RestartArguments = arguments.Optional("--restart-args") ?? string.Empty;
                context.Qa = arguments.HasFlag("--qa");
                context.ApiUrl = arguments.Optional("--api");
                context.Russian = IsRussian(arguments.Optional("--lang"), context.GameDirectory);
                if (!File.Exists(Path.Combine(context.GameDirectory, "bin", "XR_3DA.exe")))
                {
                    Note(gameDirectory, "no_engine", "bin\\XR_3DA.exe is not there");
                    return 3;
                }
                return new UpdateService(context).Run(arguments.HasFlag("--qa-check"), arguments.HasFlag("--qa-download"));
            }
            catch (Exception error)
            {
                Note(gameDirectory, "crashed", error.GetType().Name + ": " + error.Message);
                return 2;
            }
        }

        // A helper that dies before its first status write would leave nothing behind; the loader reads this.
        private static void Note(string gameDirectory, string state, string error)
        {
            if (string.IsNullOrEmpty(gameDirectory))
                return;
            try
            {
                string runtime = Path.Combine(Path.GetFullPath(gameDirectory), ".ild-fixes", "runtime");
                Directory.CreateDirectory(runtime);
                File.WriteAllText(Path.Combine(runtime, "update-last.txt"),
                    "time=" + DateTime.UtcNow.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture) +
                    "\ninstalled=" + ProductInfo.VersionText + "\nstate=" + state + "\nerror=" +
                    error.Replace("\r", " ").Replace("\n", " ") + "\n", new UTF8Encoding(false));
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }

        private static bool IsRussian(string language, string gameDirectory)
        {
            if (language != null)
                return language == "ru" || language == "rus";
            bool? game = GameLanguageIsRussian(gameDirectory);
            if (game.HasValue)
                return game.Value;
            return CultureInfo.CurrentUICulture.TwoLetterISOLanguageName == "ru";
        }

        // The engine's own string-table language wins over the Windows UI culture when the mod declares it.
        private static bool? GameLanguageIsRussian(string gameDirectory)
        {
            try
            {
                string path = Path.Combine(gameDirectory, "gamedata", "config", "localization.ltx");
                if (!File.Exists(path))
                    return null;
                foreach (string raw in File.ReadAllLines(path, Encoding.GetEncoding(1251)))
                {
                    string line = raw;
                    int comment = line.IndexOf(';');
                    if (comment >= 0)
                        line = line.Substring(0, comment);
                    int equals = line.IndexOf('=');
                    if (equals < 0 || line.Substring(0, equals).Trim() != "language")
                        continue;
                    return line.Substring(equals + 1).Trim().ToLowerInvariant() == "rus";
                }
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
            return null;
        }
    }
}
