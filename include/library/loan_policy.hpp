#pragma once

#include <cstddef>

namespace library {

struct LoanPolicy {
    static constexpr int kDefaultLoanPeriodDays = 21;
    static constexpr std::size_t kDefaultMaxActiveLoans = 3;
    static constexpr int kDefaultMaxRenewals = 2;
    static constexpr int kDefaultRenewalWindowDays = 7;

    int loanPeriodDays{kDefaultLoanPeriodDays};
    std::size_t maxActiveLoans{kDefaultMaxActiveLoans};
    int maxRenewals{kDefaultMaxRenewals};
    int renewalWindowDays{kDefaultRenewalWindowDays};
};

}  // namespace library
