#pragma once

#include <string_view>

namespace library {

enum class BookStatus { Available, Borrowed, Maintenance };

enum class TransactionStatus { Active, Closed, Overdue };

[[nodiscard]] constexpr std::string_view toString(BookStatus status) noexcept {
    switch (status) {
        case BookStatus::Available:
            return "Available";
        case BookStatus::Borrowed:
            return "Borrowed";
        case BookStatus::Maintenance:
            return "Maintenance";
    }
    return {};
}

[[nodiscard]] constexpr std::string_view toString(TransactionStatus status) noexcept {
    switch (status) {
        case TransactionStatus::Active:
            return "Active";
        case TransactionStatus::Closed:
            return "Closed";
        case TransactionStatus::Overdue:
            return "Overdue";
    }
    return {};
}

}  // namespace library
