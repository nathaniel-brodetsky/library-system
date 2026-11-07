#include "library/date.hpp"

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

}  // namespace library
