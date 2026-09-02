using System;
using System.Globalization;
using System.IO;

namespace IldFixes.Updater
{
    internal static class Program
    {
        private static int Main(string[] source)
        {
            try
            {
                Arguments arguments = Arguments.Parse(source);
                if (arguments.HasFlag("--apply"))
                    return UpdateApplier.Apply(arguments);
                if (arguments.HasFlag("--finish"))
                    return UpdateApplier.Finish(arguments);

                LaunchContext context = new LaunchContext();
                context.GameDirectory = Path.GetFullPath(arguments.Required("--game-dir"));
                context.GamePid = arguments.RequiredInt("--game-pid");
                context.RestartExe = Path.GetFullPath(arguments.Required("--restart-exe"));
                context.RestartArguments = arguments.Optional("--restart-args") ?? string.Empty;
                context.Qa = arguments.HasFlag("--qa");
                context.ApiUrl = arguments.Optional("--api");
                string language = arguments.Optional("--lang");
                context.Russian = language == "ru" || language == "rus" ||
                    language == null && CultureInfo.CurrentUICulture.TwoLetterISOLanguageName == "ru";
                if (!File.Exists(Path.Combine(context.GameDirectory, "bin", "XR_3DA.exe")))
                    return 3;
                return new UpdateService(context).Run(arguments.HasFlag("--qa-check"), arguments.HasFlag("--qa-download"));
            }
            catch
            {
                return 2;
            }
        }
    }
}
