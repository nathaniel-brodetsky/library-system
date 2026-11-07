#pragma once

#include <chrono>
#include <string>

namespace library {

using Date = std::chrono::sys_days;

constexpr Date makeDate(int year, unsigned month, unsigned day) {
    return Date{std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day}};
}

constexpr Date addDays(Date date, int days) {
    return date + std::chrono::days{days};
}

constexpr int daysBetween(Date from, Date to) {
    return static_cast<int>((to - from).count());
}

[[nodiscard]] std::string formatDate(Date date);

}  // namespace library
