using System;
using System.Collections.Generic;
using System.Globalization;

namespace IldFixes.Updater
{
    internal sealed class Arguments
    {
        private readonly Dictionary<string, string> values =
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        private readonly HashSet<string> flags = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        internal static Arguments Parse(string[] source)
        {
            Arguments result = new Arguments();
            for (int index = 0; index < source.Length; ++index)
            {
                string current = source[index];
                if (!current.StartsWith("--", StringComparison.Ordinal))
                    throw new ArgumentException("Unexpected argument: " + current);

                if (index + 1 < source.Length && !source[index + 1].StartsWith("--", StringComparison.Ordinal))
                {
                    if (result.values.ContainsKey(current) || result.flags.Contains(current))
                        throw new ArgumentException("Duplicate argument: " + current);
                    result.values.Add(current, source[++index]);
                }
                else
                {
                    if (result.values.ContainsKey(current) || !result.flags.Add(current))
                        throw new ArgumentException("Duplicate argument: " + current);
                }
            }
            return result;
        }

        internal bool HasFlag(string name)
        {
            return flags.Contains(name);
        }

        internal string Required(string name)
        {
            string value;
            if (!values.TryGetValue(name, out value) || string.IsNullOrWhiteSpace(value))
                throw new ArgumentException("Missing argument: " + name);
            return value;
        }

        internal string Optional(string name)
        {
            string value;
            return values.TryGetValue(name, out value) ? value : null;
        }

        internal int RequiredInt(string name)
        {
            int value;
            if (!int.TryParse(Required(name), NumberStyles.None, CultureInfo.InvariantCulture, out value))
                throw new ArgumentException("Invalid integer: " + name);
            return value;
        }

        internal long RequiredLong(string name)
        {
            long value;
            if (!long.TryParse(Required(name), NumberStyles.None, CultureInfo.InvariantCulture, out value))
                throw new ArgumentException("Invalid integer: " + name);
            return value;
        }

        internal long OptionalLong(string name)
        {
            string text = Optional(name);
            long value;
            if (text == null)
                return 0;
            if (!long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out value))
                throw new ArgumentException("Invalid integer: " + name);
            return value;
        }
    }
}
