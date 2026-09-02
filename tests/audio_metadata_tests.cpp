#include "audio_metadata.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

int main()
{
    using ild::AudioCommentKind;
    std::array<std::byte, 24> bytes{};
    for (std::uint32_t version = 1; version <= 3; ++version)
    {
        std::memcpy(bytes.data(), &version, sizeof(version));
        const auto length = 12 + version * 4;
        if (ild::classify_audio_comment(std::span(bytes).first(length)) != AudioCommentKind::metadata) return 1;
        if (ild::classify_audio_comment(std::span(bytes).first(length - 1)) != AudioCommentKind::malformed) return 2;
    }
    constexpr std::string_view vendor = "encoder=Lavc58.134.100 libvorbis";
    if (ild::classify_audio_comment(std::as_bytes(std::span(vendor))) != AudioCommentKind::text) return 3;
    if (ild::classify_audio_comment({}) != AudioCommentKind::text) return 4;
    bytes[0] = std::byte{42};
    if (ild::classify_audio_comment(bytes) != AudioCommentKind::malformed) return 5;
    return 0;
}
