#pragma once

#include <cstddef>
#include <string_view>

#include "library/identifiers.hpp"

namespace library {

enum class ReaderIdValidity { Valid, BelowMinimum, AboveMaximum };

enum class EmailValidity {
    Valid,
    Empty,
    TooLong,
    MissingAt,
    MultipleAt,
    LocalPartTooLong,
    InvalidLocalPart,
    DomainLabelTooLong,
    InvalidDomain,
};

class ReaderValidator {
public:
    static constexpr ReaderId kMinReaderId = 1;
    static constexpr ReaderId kMaxReaderId = 999'999;
    static constexpr std::size_t kMaxEmailLength = 254;
    static constexpr std::size_t kMaxLocalPartLength = 64;
    static constexpr std::size_t kMaxDomainLabelLength = 63;

    [[nodiscard]] static constexpr ReaderIdValidity checkId(ReaderId id) noexcept {
        if (id < kMinReaderId) {
            return ReaderIdValidity::BelowMinimum;
        }
        if (id > kMaxReaderId) {
            return ReaderIdValidity::AboveMaximum;
        }
        return ReaderIdValidity::Valid;
    }

    [[nodiscard]] static EmailValidity checkEmail(std::string_view email);
    [[nodiscard]] static bool isBlank(std::string_view text) noexcept;

private:
    [[nodiscard]] static EmailValidity checkDomain(std::string_view domain);
};

}  // namespace library
