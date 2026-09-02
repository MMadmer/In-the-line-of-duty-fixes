#include "audio_metadata.h"
#include <cstdint>
#include <cstring>

namespace ild
{
AudioCommentKind classify_audio_comment(std::span<const std::byte> value)
{
    std::uint32_t version{};
    if (value.size() >= sizeof(version)) std::memcpy(&version, value.data(), sizeof(version));
    if (version >= 1 && version <= 3)
        return value.size() >= 12 + version * 4 ? AudioCommentKind::metadata : AudioCommentKind::malformed;
    for (const auto byte : value)
    {
        const auto character = std::to_integer<unsigned>(byte);
        if (character < 32 && character != 9 && character != 10 && character != 13)
            return AudioCommentKind::malformed;
    }
    return AudioCommentKind::text;
}
}
