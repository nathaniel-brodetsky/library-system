#pragma once

#include <algorithm>

#include "library/date.hpp"
#include "library/identifiers.hpp"
#include "library/statuses.hpp"

namespace library {

enum class OverdueSeverity { None, Regular, Severe };

struct FineBreakdown {
    int overdueDays{0};
    OverdueSeverity severity{OverdueSeverity::None};
    Money overdueFine{0};
    Money damageFine{0};

    [[nodiscard]] constexpr Money total() const noexcept { return overdueFine + damageFine; }

    friend constexpr bool operator==(const FineBreakdown&, const FineBreakdown&) = default;
};

class FinePolicy {
public:
    static constexpr int kSevereOverdueThresholdDays = 30;
    static constexpr Money kRegularDailyRate = 5;
    static constexpr Money kSevereDailyRate = 10;
    static constexpr Money kDamageFee = 200;

    [[nodiscard]] static constexpr OverdueSeverity classify(int overdueDays) noexcept {
        if (overdueDays <= 0) {
            return OverdueSeverity::None;
        }
        if (overdueDays <= kSevereOverdueThresholdDays) {
            return OverdueSeverity::Regular;
        }
        return OverdueSeverity::Severe;
    }

    [[nodiscard]] static constexpr Money dailyRate(OverdueSeverity severity) noexcept {
        switch (severity) {
            case OverdueSeverity::None:
                return 0;
            case OverdueSeverity::Regular:
                return kRegularDailyRate;
            case OverdueSeverity::Severe:
                return kSevereDailyRate;
        }
        return 0;
    }

    [[nodiscard]] static constexpr FineBreakdown calculateFine(Date dueDate, Date returnDate,
                                                               BookCondition condition) noexcept {
        const int overdueDays = std::max(0, daysBetween(dueDate, returnDate));
        const OverdueSeverity severity = classify(overdueDays);
        return FineBreakdown{
            overdueDays,
            severity,
            dailyRate(severity) * overdueDays,
            condition == BookCondition::Damaged ? kDamageFee : Money{0},
        };
    }
};

}  // namespace library
