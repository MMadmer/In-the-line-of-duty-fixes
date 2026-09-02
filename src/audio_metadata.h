#pragma once
#include <cstddef>
#include <span>

namespace ild
{
enum class AudioCommentKind { metadata, text, malformed };
[[nodiscard]] AudioCommentKind classify_audio_comment(std::span<const std::byte> value);
}
