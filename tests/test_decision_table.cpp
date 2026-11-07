#include <gtest/gtest.h>

#include <cstddef>
#include <ostream>
#include <string>

#include "library_test_support.hpp"

namespace library::test {
namespace {

inline constexpr Date kDebtIssueDate = addDays(kToday, -30);
inline constexpr Date kRegularIssueDate = addDays(kToday, -15);
inline constexpr Date kOtherReaderIssueDate = addDays(kToday, -1);

struct DecisionColumn {
    int column;
    bool readerRegistered;
    bool noOverdueDebt;
    bool withinLoanLimit;
    bool copyAvailable;
    BorrowResult expected;
};

void PrintTo(const DecisionColumn& value, std::ostream* os) {
    *os << "column " << value.column << " (" << value.readerRegistered << value.noOverdueDebt << value.withinLoanLimit
        << value.copyAvailable << ")";
}

std::string columnName(const ::testing::TestParamInfo<DecisionColumn>& info) {
    const int column = info.param.column;
    return std::string{"Column"} + (column < 10 ? "0" : "") + std::to_string(column);
}

class IssueBookDecisionTableTest : public LibraryTest, public ::testing::WithParamInterface<DecisionColumn> {
protected:
    void arrange(const DecisionColumn& column) {
        if (!column.copyAvailable) {
            borrowOrFail(kOtherReader, kTargetCopy, kOtherReaderIssueDate);
        }
        if (!column.readerRegistered) {
            return;
        }

        CopyId nextCopy = kFirstSpareCopy;
        if (!column.noOverdueDebt) {
            borrowOrFail(kRegisteredReader, nextCopy++, kDebtIssueDate);
        }
        if (!column.withinLoanLimit) {
            while (service_.transactions().countOpen(kRegisteredReader) < service_.policy().maxActiveLoans) {
                borrowOrFail(kRegisteredReader, nextCopy++, kRegularIssueDate);
            }
        }
    }

    void verifyConditions(const DecisionColumn& column, ReaderId reader) const {
        ASSERT_EQ(service_.readers().contains(reader), column.readerRegistered);
        ASSERT_EQ(!service_.transactions().hasOverdue(reader, kToday), column.noOverdueDebt);
        ASSERT_EQ(service_.transactions().countOpen(reader) < service_.policy().maxActiveLoans, column.withinLoanLimit);
        ASSERT_EQ(service_.copies().find(kTargetCopy)->isAvailable(), column.copyAvailable);
    }
};

TEST_P(IssueBookDecisionTableTest, PerformsActionOfColumn) {
    const DecisionColumn& column = GetParam();
    const ReaderId reader = column.readerRegistered ? kRegisteredReader : kUnregisteredReader;

    arrange(column);
    verifyConditions(column, reader);

    const BookStatus statusBefore = copyStatus(kTargetCopy);
    const std::size_t transactionsBefore = service_.transactions().size();

    const BorrowReceipt receipt = service_.borrowBook(reader, kTargetCopy, kToday);

    EXPECT_EQ(receipt.result, column.expected);
    if (column.expected == BorrowResult::Issued) {
        ASSERT_TRUE(receipt.transactionId.has_value());
        ASSERT_TRUE(receipt.dueDate.has_value());
        EXPECT_EQ(*receipt.dueDate, addDays(kToday, LoanPolicy::kDefaultLoanPeriodDays));
        EXPECT_EQ(copyStatus(kTargetCopy), BookStatus::Borrowed);
        EXPECT_EQ(service_.transactions().size(), transactionsBefore + 1);
        const Transaction* transaction = service_.transactions().find(*receipt.transactionId);
        ASSERT_NE(transaction, nullptr);
        EXPECT_EQ(transaction->status(), TransactionStatus::Active);
        EXPECT_EQ(transaction->readerId(), reader);
        EXPECT_EQ(transaction->copyId(), kTargetCopy);
    } else {
        EXPECT_FALSE(receipt.transactionId.has_value());
        EXPECT_FALSE(receipt.dueDate.has_value());
        EXPECT_EQ(copyStatus(kTargetCopy), statusBefore);
        EXPECT_EQ(service_.transactions().size(), transactionsBefore);
    }
}

constexpr DecisionColumn kFullDecisionTable[] = {
    {6, false, true, true, false, BorrowResult::ReaderNotRegistered},
    {7, false, true, true, true, BorrowResult::ReaderNotRegistered},
    {8, true, false, false, false, BorrowResult::OverdueDebt},
    {9, true, false, false, true, BorrowResult::OverdueDebt},
    {10, true, false, true, false, BorrowResult::OverdueDebt},
    {11, true, false, true, true, BorrowResult::OverdueDebt},
    {12, true, true, false, false, BorrowResult::LoanLimitReached},
    {13, true, true, false, true, BorrowResult::LoanLimitReached},
    {14, true, true, true, false, BorrowResult::CopyUnavailable},
    {15, true, true, true, true, BorrowResult::Issued},
};

INSTANTIATE_TEST_SUITE_P(IssueBook, IssueBookDecisionTableTest, ::testing::ValuesIn(kFullDecisionTable), columnName);

class IssueBookConditionTest : public LibraryTest {};

TEST_F(IssueBookConditionTest, NonExistentCopyIsNotAvailable) {
    EXPECT_EQ(service_.borrowBook(kRegisteredReader, kMissingCopy, kToday).result, BorrowResult::CopyUnavailable);
}

TEST_F(IssueBookConditionTest, OverdueLoanIsMarkedOverdueWhenDebtIsDetected) {
    borrowOrFail(kRegisteredReader, kFirstSpareCopy, kDebtIssueDate);

    EXPECT_EQ(service_.borrowBook(kRegisteredReader, kTargetCopy, kToday).result, BorrowResult::OverdueDebt);

    const auto openLoans = service_.transactions().openFor(kRegisteredReader);
    ASSERT_EQ(openLoans.size(), 1U);
    EXPECT_EQ(openLoans.front().status(), TransactionStatus::Overdue);
}

TEST_F(IssueBookConditionTest, DebtOfAnotherReaderDoesNotBlockIssuing) {
    borrowOrFail(kOtherReader, kFirstSpareCopy, kDebtIssueDate);

    EXPECT_EQ(service_.borrowBook(kRegisteredReader, kTargetCopy, kToday).result, BorrowResult::Issued);
}

}  // namespace
}  // namespace library::test
