#include "console_editor.h"

#include <algorithm>
#include <cctype>

namespace ild
{
void ConsoleEditor::sync(std::string_view text)
{
    if (text != text_) assign(text);
}

void ConsoleEditor::assign(std::string_view text)
{
    text_ = text.substr(0, maximum_length);
    cursor_ = anchor_ = text_.size();
}

std::pair<std::size_t, std::size_t> ConsoleEditor::selection() const
{
    return std::minmax(cursor_, anchor_);
}

void ConsoleEditor::insert(std::string_view value)
{
    std::string clean;
    clean.reserve(std::min(value.size(), maximum_length));
    for (const auto character : value)
    {
        if (clean.size() == maximum_length) break;
        if (character == '\r' || character == '\n' || character == '\t') clean.push_back(' ');
        else if (static_cast<unsigned char>(character) >= 32) clean.push_back(character);
    }
    if (clean.empty()) return;
    const auto [begin, end] = selection();
    clean.resize(std::min(clean.size(), maximum_length - (text_.size() - (end - begin))));
    text_.replace(begin, end - begin, clean);
    cursor_ = anchor_ = begin + clean.size();
}

void ConsoleEditor::erase(bool backward)
{
    auto [begin, end] = selection();
    if (begin == end)
    {
        if (backward && begin) --begin;
        else if (!backward && end < text_.size()) ++end;
    }
    text_.erase(begin, end - begin);
    cursor_ = anchor_ = begin;
}

void ConsoleEditor::position(std::size_t value, bool select)
{
    cursor_ = std::min(value, text_.size());
    if (!select) anchor_ = cursor_;
}

void ConsoleEditor::move(int direction, bool select, bool word)
{
    const auto [begin, end] = selection();
    if (!select && begin != end)
    {
        position(direction < 0 ? begin : end, false);
        return;
    }
    auto next = cursor_;
    if (direction < 0)
    {
        if (next) --next;
        if (word)
        {
            while (next && text_[next] == ' ') --next;
            while (next && text_[next - 1] != ' ') --next;
        }
    }
    else
    {
        if (next < text_.size()) ++next;
        if (word)
        {
            while (next < text_.size() && text_[next] != ' ') ++next;
            while (next < text_.size() && text_[next] == ' ') ++next;
        }
    }
    position(next, select);
}

void ConsoleEditor::home(bool select) { position(0, select); }
void ConsoleEditor::end(bool select) { position(text_.size(), select); }
void ConsoleEditor::select_all() { anchor_ = 0; cursor_ = text_.size(); }

std::string ConsoleEditor::copy_text() const
{
    const auto [begin, end] = selection();
    return begin == end ? text_ : text_.substr(begin, end - begin);
}

std::string ConsoleEditor::preview(bool cursor) const
{
    const auto [begin, end] = selection();
    std::string result;
    result.reserve(text_.size() + 3);
    for (std::size_t index = 0; index <= text_.size(); ++index)
    {
        if (begin != end && index == begin) result.push_back('[');
        if (begin != end && index == end) result.push_back(']');
        if (cursor && index == cursor_) result.push_back('|');
        if (index < text_.size()) result.push_back(text_[index]);
    }
    return result;
}
}
