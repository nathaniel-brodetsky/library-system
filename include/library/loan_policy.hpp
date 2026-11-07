#pragma once

#include <cstddef>

namespace library {

struct LoanPolicy {
    static constexpr int kDefaultLoanPeriodDays = 21;
    static constexpr std::size_t kDefaultMaxActiveLoans = 3;

    int loanPeriodDays{kDefaultLoanPeriodDays};
    std::size_t maxActiveLoans{kDefaultMaxActiveLoans};
};

}  // namespace library
