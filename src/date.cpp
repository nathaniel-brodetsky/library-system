#include "library/date.hpp"

#include <charconv>
#include <iomanip>
#include <sstream>

namespace library {

std::string formatDate(Date date) {
    const std::chrono::year_month_day calendarDate{date};
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(4) << static_cast<int>(calendarDate.year()) << '-' << std::setw(2)
           << static_cast<unsigned>(calendarDate.month()) << '-' << std::setw(2)
           << static_cast<unsigned>(calendarDate.day());
    return stream.str();
}

std::optional<Date> parseDate(std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return std::nullopt;
    }
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    const auto parse = [&text](std::size_t offset, std::size_t length, auto& value) {
        const char* first = text.data() + offset;
        const char* last = first + length;
        const auto [end, error] = std::from_chars(first, last, value);
        return error == std::errc{} && end == last;
    };
    if (!parse(0, 4, year) || !parse(5, 2, month) || !parse(8, 2, day)) {
        return std::nullopt;
    }
    const std::chrono::year_month_day calendarDate{std::chrono::year{year}, std::chrono::month{month},
                                                   std::chrono::day{day}};
    if (!calendarDate.ok()) {
        return std::nullopt;
    }
    return Date{calendarDate};
}

}  // namespace library
