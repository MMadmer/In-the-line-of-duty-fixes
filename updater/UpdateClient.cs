using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;

namespace IldFixes.Updater
{
    internal sealed class UpdateClient
    {
        private const int MaximumApiBytes = 4 * 1024 * 1024;
        private readonly LaunchContext context;

        internal UpdateClient(LaunchContext context)
        {
            this.context = context;
            ServicePointManager.SecurityProtocol = (SecurityProtocolType)3072;
        }

        internal CheckResult Check()
        {
            Uri api = ResolveApiUri();
            string json = ReadText(api, MaximumApiBytes);
            List<ReleaseInfo> releases = ParseReleases(json);
            releases.Sort(delegate(ReleaseInfo left, ReleaseInfo right)
            {
                return left.Version.CompareTo(right.Version);
            });

            CheckResult result = new CheckResult();
            ReleaseInfo target = releases.LastOrDefault(delegate(ReleaseInfo release)
            {
                return release.Version.Major == ProductInfo.Version.Major &&
                    release.Version > ProductInfo.Version && FindFullAsset(release) != null;
            });
            if (target != null)
                result.Update = BuildUpdateOffer(releases, target);

            ReleaseInfo major = releases.LastOrDefault(delegate(ReleaseInfo release)
            {
                return release.Version.Major > ProductInfo.Version.Major && FindFullAsset(release) != null;
            });
            if (major != null)
            {
                result.Major = new MajorOffer();
                result.Major.Version = major.Version;
                result.Major.PageUrl = major.PageUrl;
                result.Major.Notes = ParseNotes(major.Body);
            }
            return result;
        }

        internal string Download(UpdateOffer offer, Action<DownloadProgress> progress)
        {
            string cache = Path.Combine(context.GameDirectory, ".ild-fixes", "update-cache", offer.Version.ToString());
            Directory.CreateDirectory(cache);
            string destination = Path.Combine(cache, offer.Asset.Name);
            string partial = destination + ".part";

            if (File.Exists(destination) && new FileInfo(destination).Length == offer.Asset.Size &&
                Digest(destination) == NormalizeDigest(offer.Asset.Digest))
                return destination;

            if (File.Exists(partial))
                File.Delete(partial);

            HttpWebRequest request = CreateRequest(new Uri(offer.Asset.Url));
            using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
            {
                ValidateResponseUri(response.ResponseUri);
                if (response.StatusCode != HttpStatusCode.OK || response.ContentLength != offer.Asset.Size)
                    throw new InvalidDataException("The update size does not match the release metadata.");

                using (Stream input = response.GetResponseStream())
                using (FileStream output = new FileStream(partial, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    byte[] buffer = new byte[64 * 1024];
                    long total = 0;
                    int read;
                    while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        total += read;
                        if (total > offer.Asset.Size)
                            throw new InvalidDataException("The update exceeded its declared size.");
                        output.Write(buffer, 0, read);
                        if (progress != null)
                            progress(new DownloadProgress { Downloaded = total, Total = offer.Asset.Size });
                    }
                    output.Flush(true);
                }
            }

            if (new FileInfo(partial).Length != offer.Asset.Size ||
                Digest(partial) != NormalizeDigest(offer.Asset.Digest))
            {
                File.Delete(partial);
                throw new InvalidDataException("The downloaded update failed SHA-256 verification.");
            }

            if (File.Exists(destination))
                File.Delete(destination);
            File.Move(partial, destination);
            return destination;
        }

        private Uri ResolveApiUri()
        {
            if (string.IsNullOrEmpty(context.ApiUrl))
                return new Uri("https://api.github.com/repos/" + ProductInfo.Repository + "/releases?per_page=30");

            Uri uri;
            if (!context.Qa || !Uri.TryCreate(context.ApiUrl, UriKind.Absolute, out uri) || !uri.IsLoopback)
                throw new InvalidOperationException("The QA update API must be an explicit loopback URL.");
            return uri;
        }

        private string ReadText(Uri uri, int maximumBytes)
        {
            HttpWebRequest request = CreateRequest(uri);
            using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
            {
                ValidateResponseUri(response.ResponseUri);
                if (response.StatusCode != HttpStatusCode.OK || response.ContentLength > maximumBytes)
                    throw new InvalidDataException("The release list is not usable.");

                using (Stream stream = response.GetResponseStream())
                using (MemoryStream memory = new MemoryStream())
                {
                    byte[] buffer = new byte[16 * 1024];
                    int read;
                    while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        if (memory.Length + read > maximumBytes)
                            throw new InvalidDataException("The release list exceeded its size limit.");
                        memory.Write(buffer, 0, read);
                    }
                    return Encoding.UTF8.GetString(memory.ToArray());
                }
            }
        }

        private HttpWebRequest CreateRequest(Uri uri)
        {
            if (!context.Qa && !string.Equals(uri.Scheme, "https", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("Production update traffic must use HTTPS.");
            if (context.Qa && !uri.IsLoopback && !string.Equals(uri.Scheme, "https", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("Only a loopback QA server may use HTTP.");

            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(uri);
            request.UserAgent = "InTheLineOfDutyFixes/" + ProductInfo.VersionText;
            request.Accept = "application/vnd.github+json";
            request.Timeout = 15000;
            request.ReadWriteTimeout = 15000;
            request.AllowAutoRedirect = true;
            return request;
        }

        private void ValidateResponseUri(Uri uri)
        {
            if (uri.Scheme != Uri.UriSchemeHttps && !(context.Qa && uri.IsLoopback))
                throw new InvalidOperationException("The update connection was redirected to an insecure URL.");
        }

        private List<ReleaseInfo> ParseReleases(string json)
        {
            JavaScriptSerializer serializer = new JavaScriptSerializer();
            serializer.MaxJsonLength = MaximumApiBytes;
            object[] source = serializer.DeserializeObject(json) as object[];
            if (source == null)
                throw new InvalidDataException("GitHub returned an invalid release list.");

            List<ReleaseInfo> result = new List<ReleaseInfo>();
            foreach (object item in source)
            {
                Dictionary<string, object> value = item as Dictionary<string, object>;
                if (value == null || ReadBool(value, "draft") || ReadBool(value, "prerelease"))
                    continue;

                string tag = ReadString(value, "tag_name");
                Version version;
                if (!TryVersion(tag, out version))
                    continue;

                ReleaseInfo release = new ReleaseInfo();
                release.Tag = tag;
                release.Version = version;
                release.Body = ReadString(value, "body") ?? string.Empty;
                release.PageUrl = ReadString(value, "html_url") ?? string.Empty;

                object[] assets = ReadArray(value, "assets");
                if (assets != null)
                {
                    foreach (object assetItem in assets)
                    {
                        Dictionary<string, object> assetValue = assetItem as Dictionary<string, object>;
                        if (assetValue == null)
                            continue;
                        ReleaseAsset asset = new ReleaseAsset();
                        asset.Name = ReadString(assetValue, "name");
                        asset.Url = ReadString(assetValue, "browser_download_url");
                        asset.Digest = ReadString(assetValue, "digest");
                        asset.Size = ReadLong(assetValue, "size");
                        if (ValidAsset(asset))
                            release.Assets.Add(asset);
                    }
                }
                result.Add(release);
            }
            return result;
        }

        private UpdateOffer BuildUpdateOffer(List<ReleaseInfo> releases, ReleaseInfo target)
        {
            ReleaseAsset selected = FindFullAsset(target);
            bool patch = false;
            List<ReleaseInfo> usable = releases.Where(delegate(ReleaseInfo release)
            {
                return release.Version.Major == ProductInfo.Version.Major && FindFullAsset(release) != null;
            }).ToList();
            ReleaseInfo previous = usable.LastOrDefault(delegate(ReleaseInfo release)
            {
                return release.Version < target.Version;
            });

            ReleaseAsset patchAsset = FindAsset(target, ProductInfo.AssetPrefix + target.Version + "-Update_Patch.zip");
            string rejected = Path.Combine(context.GameDirectory, ".ild-fixes", "patch-rejected.txt");
            bool rejectedForTarget = File.Exists(rejected) &&
                string.Equals(File.ReadAllText(rejected).Trim(), target.Version.ToString(), StringComparison.Ordinal);
            if (previous != null && previous.Version == ProductInfo.Version && patchAsset != null && !rejectedForTarget)
            {
                selected = patchAsset;
                patch = true;
            }

            UpdateOffer offer = new UpdateOffer();
            offer.Version = target.Version;
            offer.Asset = selected;
            offer.IsPatch = patch;
            offer.Notes = ParseNotes(target.Body);
            return offer;
        }

        private ReleaseAsset FindFullAsset(ReleaseInfo release)
        {
            ReleaseAsset current = FindAsset(release,
                ProductInfo.AssetPrefix + release.Version + "-Setup_Manual.zip");
            return current ?? FindAsset(release, ProductInfo.AssetPrefix + release.Version + "-Update.zip");
        }

        private static ReleaseAsset FindAsset(ReleaseInfo release, string name)
        {
            return release.Assets.FirstOrDefault(delegate(ReleaseAsset asset)
            {
                return string.Equals(asset.Name, name, StringComparison.Ordinal);
            });
        }

        private bool ValidAsset(ReleaseAsset asset)
        {
            Uri uri;
            if (asset == null || string.IsNullOrEmpty(asset.Name) || asset.Size <= 0 ||
                !ValidDigest(asset.Digest) || !Uri.TryCreate(asset.Url, UriKind.Absolute, out uri))
                return false;
            return context.Qa ? uri.IsLoopback || uri.Scheme == Uri.UriSchemeHttps :
                uri.Scheme == Uri.UriSchemeHttps && string.Equals(uri.Host, "github.com", StringComparison.OrdinalIgnoreCase);
        }

        private static LocalizedNotes ParseNotes(string body)
        {
            LocalizedNotes notes = new LocalizedNotes();
            string language = string.Empty;
            string section = string.Empty;
            List<string> en = new List<string>();
            List<string> ru = new List<string>();
            string[] lines = (body ?? string.Empty).Replace("\r", string.Empty).Split('\n');
            foreach (string raw in lines)
            {
                string line = raw.Trim();
                if (line == "## EN" || line == "## RU")
                {
                    language = line.Substring(3);
                    section = string.Empty;
                    continue;
                }
                if (line.StartsWith("## ", StringComparison.Ordinal))
                {
                    section = line.Substring(3);
                    continue;
                }

                bool theme = section == "Theme" || section == "Тема";
                bool changes = section == "Changes" || section == "Изменения";
                if (theme && line.Length > 0 && !line.StartsWith("* ") && !line.StartsWith("- "))
                {
                    if (Encoding.UTF8.GetByteCount(line) <= 256)
                    {
                        if (language == "EN" && notes.ThemeEn.Length == 0)
                            notes.ThemeEn = line;
                        if (language == "RU" && notes.ThemeRu.Length == 0)
                            notes.ThemeRu = line;
                    }
                    section = string.Empty;
                }
                else if (changes && (line.StartsWith("* ") || line.StartsWith("- ")))
                {
                    string item = "• " + line.Substring(2).Trim();
                    if (language == "EN")
                        en.Add(item);
                    if (language == "RU")
                        ru.Add(item);
                }
            }
            notes.ChangesEn = string.Join(Environment.NewLine, en.ToArray());
            notes.ChangesRu = string.Join(Environment.NewLine, ru.ToArray());
            return notes;
        }

        private static bool TryVersion(string text, out Version version)
        {
            version = null;
            if (string.IsNullOrEmpty(text) || text.Count(delegate(char value) { return value == '.'; }) != 2)
                return false;
            Version parsed;
            if (!Version.TryParse(text, out parsed) || parsed.Major < 0 || parsed.Minor < 0 || parsed.Build < 0)
                return false;
            version = new Version(parsed.Major, parsed.Minor, parsed.Build);
            return version.ToString() == text;
        }

        private static bool ValidDigest(string value)
        {
            if (string.IsNullOrEmpty(value) || !value.StartsWith("sha256:", StringComparison.OrdinalIgnoreCase) ||
                value.Length != 71)
                return false;
            for (int index = 7; index < value.Length; ++index)
            {
                char character = value[index];
                if (!Uri.IsHexDigit(character))
                    return false;
            }
            return true;
        }

        internal static string Digest(string path)
        {
            using (SHA256 algorithm = SHA256.Create())
            using (FileStream stream = File.OpenRead(path))
                return BitConverter.ToString(algorithm.ComputeHash(stream)).Replace("-", string.Empty).ToLowerInvariant();
        }

        internal static string NormalizeDigest(string digest)
        {
            return digest.Substring(7).ToLowerInvariant();
        }

        private static string ReadString(Dictionary<string, object> value, string name)
        {
            object result;
            return value.TryGetValue(name, out result) ? result as string : null;
        }

        private static bool ReadBool(Dictionary<string, object> value, string name)
        {
            object result;
            return value.TryGetValue(name, out result) && result is bool && (bool)result;
        }

        private static object[] ReadArray(Dictionary<string, object> value, string name)
        {
            object result;
            return value.TryGetValue(name, out result) ? result as object[] : null;
        }

        private static long ReadLong(Dictionary<string, object> value, string name)
        {
            object result;
            if (!value.TryGetValue(name, out result) || result == null)
                return 0;
            return Convert.ToInt64(result, CultureInfo.InvariantCulture);
        }
    }
}
