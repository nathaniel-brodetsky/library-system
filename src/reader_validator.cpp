#include "library/reader_validator.hpp"

#include <algorithm>
#include <regex>
#include <vector>

namespace library {
namespace {

const std::regex& localPartPattern() {
    static const std::regex pattern{R"(^[A-Za-z0-9!#$%&'*+/=?^_`{|}~-]+(\.[A-Za-z0-9!#$%&'*+/=?^_`{|}~-]+)*$)"};
    return pattern;
}

const std::regex& domainLabelPattern() {
    static const std::regex pattern{R"(^[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?$)"};
    return pattern;
}

const std::regex& topLevelDomainPattern() {
    static const std::regex pattern{R"(^[A-Za-z]{2,}$)"};
    return pattern;
}

bool matches(std::string_view text, const std::regex& pattern) {
    return std::regex_match(text.begin(), text.end(), pattern);
}

std::vector<std::string_view> splitLabels(std::string_view domain) {
    std::vector<std::string_view> labels;
    std::size_t start = 0;
    while (true) {
        const std::size_t dot = domain.find('.', start);
        labels.push_back(domain.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
        if (dot == std::string_view::npos) {
            return labels;
        }
        start = dot + 1;
    }
}

}  // namespace

EmailValidity ReaderValidator::checkEmail(std::string_view email) {
    if (email.empty()) {
        return EmailValidity::Empty;
    }
    if (email.size() > kMaxEmailLength) {
        return EmailValidity::TooLong;
    }

    const std::size_t at = email.find('@');
    if (at == std::string_view::npos) {
        return EmailValidity::MissingAt;
    }
    if (email.find('@', at + 1) != std::string_view::npos) {
        return EmailValidity::MultipleAt;
    }

    const std::string_view localPart = email.substr(0, at);
    if (localPart.size() > kMaxLocalPartLength) {
        return EmailValidity::LocalPartTooLong;
    }
    if (!matches(localPart, localPartPattern())) {
        return EmailValidity::InvalidLocalPart;
    }
    return checkDomain(email.substr(at + 1));
}

EmailValidity ReaderValidator::checkDomain(std::string_view domain) {
    const std::vector<std::string_view> labels = splitLabels(domain);
    if (labels.size() < 2) {
        return EmailValidity::InvalidDomain;
    }
    for (const std::string_view label : labels) {
        if (label.size() > kMaxDomainLabelLength) {
            return EmailValidity::DomainLabelTooLong;
        }
        if (!matches(label, domainLabelPattern())) {
            return EmailValidity::InvalidDomain;
        }
    }
    if (!matches(labels.back(), topLevelDomainPattern())) {
        return EmailValidity::InvalidDomain;
    }
    return EmailValidity::Valid;
}

bool ReaderValidator::isBlank(std::string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](char symbol) {
        return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
    });
}

}  // namespace library
