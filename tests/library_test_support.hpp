#pragma once

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "library/library_service.hpp"

namespace library::test {

inline constexpr Date kToday = makeDate(2025, 3, 10);
inline constexpr Date kRegistrationDate = makeDate(2025, 1, 15);

inline constexpr ReaderId kRegisteredReader = 1;
inline constexpr ReaderId kOtherReader = 2;
inline constexpr ReaderId kUnregisteredReader = 42;

inline constexpr CopyId kTargetCopy = 101;
inline constexpr CopyId kFirstSpareCopy = 102;
inline constexpr CopyId kLastCopy = 110;
inline constexpr CopyId kMissingCopy = 999;

inline constexpr std::string_view kIsbn = "978-966-03-4058-5";

class LibraryTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(service_.registerReader(kRegisteredReader, "Шевченко Тарас", "taras@example.com", kRegistrationDate),
                  RegistrationResult::Registered);
        ASSERT_EQ(service_.registerReader(kOtherReader, "Українка Леся", "lesya@example.com", kRegistrationDate),
                  RegistrationResult::Registered);
        ASSERT_TRUE(service_.addBook(Book{std::string{kIsbn}, "Кобзар", "Тарас Шевченко", 1840}));
        for (CopyId copy = kTargetCopy; copy <= kLastCopy; ++copy) {
            ASSERT_TRUE(service_.addCopy(BookCopy{copy, std::string{kIsbn}}));
        }
    }

    void borrowOrFail(ReaderId reader, CopyId copy, Date date) {
        const BorrowReceipt receipt = service_.borrowBook(reader, copy, date);
        ASSERT_TRUE(receipt.issued()) << "precondition loan for copy " << copy << " was not issued";
    }

    [[nodiscard]] BookStatus copyStatus(CopyId copy) const {
        const BookCopy* found = service_.copies().find(copy);
        EXPECT_NE(found, nullptr);
        return found == nullptr ? BookStatus::Maintenance : found->status();
    }

    LibraryService service_;
};

}  // namespace library::test
