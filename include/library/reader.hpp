#pragma once

#include <string>
#include <utility>

#include "library/date.hpp"
#include "library/identifiers.hpp"

namespace library {

class Reader {
public:
    Reader(ReaderId id, std::string fullName, std::string email, Date registrationDate)
        : id_(id), fullName_(std::move(fullName)), email_(std::move(email)), registrationDate_(registrationDate) {}

    [[nodiscard]] ReaderId id() const noexcept { return id_; }
    [[nodiscard]] const std::string& fullName() const noexcept { return fullName_; }
    [[nodiscard]] const std::string& email() const noexcept { return email_; }
    [[nodiscard]] Date registrationDate() const noexcept { return registrationDate_; }

private:
    ReaderId id_;
    std::string fullName_;
    std::string email_;
    Date registrationDate_;
};

}  // namespace library
