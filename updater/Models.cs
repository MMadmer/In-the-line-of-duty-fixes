using System;
using System.Collections.Generic;

namespace IldFixes.Updater
{
    internal sealed class ReleaseAsset
    {
        internal string Name;
        internal string Url;
        internal string Digest;
        internal long Size;
    }

    internal sealed class ReleaseInfo
    {
        internal Version Version;
        internal string Tag;
        internal string Body;
        internal string PageUrl;
        internal readonly List<ReleaseAsset> Assets = new List<ReleaseAsset>();
    }

    internal sealed class LocalizedNotes
    {
        internal string ThemeEn = string.Empty;
        internal string ThemeRu = string.Empty;
        internal string ChangesEn = string.Empty;
        internal string ChangesRu = string.Empty;
    }

    internal sealed class UpdateOffer
    {
        internal Version Version;
        internal ReleaseAsset Asset;
        internal bool IsPatch;
        internal LocalizedNotes Notes;
    }

    internal sealed class MajorOffer
    {
        internal Version Version;
        internal string PageUrl;
        internal LocalizedNotes Notes;
    }

    internal sealed class CheckResult
    {
        internal UpdateOffer Update;
        internal MajorOffer Major;
    }

    internal sealed class DownloadProgress
    {
        internal long Downloaded;
        internal long Total;
    }

    internal sealed class LaunchContext
    {
        internal string GameDirectory;
        internal int GamePid;
        internal string RestartExe;
        internal string RestartArguments;
        internal bool Russian;
        internal bool Qa;
        internal string ApiUrl;
    }
}
