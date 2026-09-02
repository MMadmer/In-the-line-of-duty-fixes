#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace ild
{
class ConsoleEditor
{
public:
    static constexpr std::size_t maximum_length = 1015;
    void sync(std::string_view text);
    void assign(std::string_view text);
    void insert(std::string_view text);
    void erase(bool backward);
    void move(int direction, bool select, bool word = false);
    void home(bool select);
    void end(bool select);
    void select_all();
    [[nodiscard]] std::string copy_text() const;
    [[nodiscard]] std::string preview(bool cursor) const;
    [[nodiscard]] const std::string& text() const { return text_; }
    [[nodiscard]] std::size_t cursor() const { return cursor_; }
    [[nodiscard]] std::pair<std::size_t, std::size_t> selection() const;

private:
    void position(std::size_t value, bool select);
    std::string text_;
    std::size_t cursor_{};
    std::size_t anchor_{};
};
}
