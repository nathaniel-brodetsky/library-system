#include <gtest/gtest.h>

#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "library_test_support.hpp"

namespace library::test {
namespace {

inline constexpr Date kIssueDate = makeDate(2025, 3, 1);
inline constexpr Date kDueDate = addDays(kIssueDate, LoanPolicy::kDefaultLoanPeriodDays);
inline constexpr Date kOnTimeReturn = addDays(kDueDate, -3);
inline constexpr Date kRegularOverdueReturn = addDays(kDueDate, 10);
inline constexpr Date kSevereOverdueReturn = addDays(kDueDate, 40);
inline constexpr CopyId kLoanedToOtherReader = kFirstSpareCopy;

static_assert(kDueDate == makeDate(2025, 3, 22));
static_assert(FinePolicy::calculateFine(kDueDate, kOnTimeReturn, BookCondition::Intact).total() == 0);
static_assert(FinePolicy::calculateFine(kDueDate, kRegularOverdueReturn, BookCondition::Intact).total() == 50);
static_assert(FinePolicy::calculateFine(kDueDate, kSevereOverdueReturn, BookCondition::Damaged).total() == 600);

struct Effects {
    bool e20ReaderRejected;
    bool e21CopyRejected;
    bool e22NoLoanRejected;
    bool e23LoanClosed;
    bool e24ReturnDateRecorded;
    bool e25CopyAvailable;
    bool e26CopyMaintenance;
    bool e27RegularFine;
    bool e28SevereFine;
    bool e29DamageFee;
    bool e30NoFine;
};

struct CausalColumn {
    std::string_view name;
    ReaderId reader;
    CopyId copy;
    Date returnDate;
    BookCondition condition;
    Effects expected;
};

void PrintTo(const CausalColumn& value, std::ostream* os) {
    *os << value.name;
}

class ReturnBookCauseEffectTest : public LibraryTest, public ::testing::WithParamInterface<CausalColumn> {
protected:
    void SetUp() override {
        LibraryTest::SetUp();
        const BorrowReceipt loan = service_.borrowBook(kRegisteredReader, kTargetCopy, kIssueDate);
        ASSERT_TRUE(loan.issued());
        loanId_ = *loan.transactionId;
        borrowOrFail(kOtherReader, kLoanedToOtherReader, kIssueDate);
    }

    [[nodiscard]] const Transaction& targetLoan() const { return *service_.transactions().find(loanId_); }

    [[nodiscard]] Effects observe(const ReturnReceipt& receipt, CopyId copy, std::optional<BookStatus> statusBefore) const {
        const bool accepted = receipt.outcome == ReturnOutcome::Returned;
        const std::optional<BookStatus> statusAfter =
            service_.copies().find(copy) != nullptr ? std::optional{copyStatus(copy)} : std::nullopt;
        const bool statusChanged = accepted && statusAfter != statusBefore;
        return Effects{
            receipt.outcome == ReturnOutcome::ReaderNotRegistered,
            receipt.outcome == ReturnOutcome::CopyNotFound,
            receipt.outcome == ReturnOutcome::NoActiveLoan,
            targetLoan().status() == TransactionStatus::Closed,
            targetLoan().returnDate().has_value(),
            statusChanged && statusAfter == BookStatus::Available,
            statusChanged && statusAfter == BookStatus::Maintenance,
            receipt.fine.severity == OverdueSeverity::Regular && receipt.fine.overdueFine > 0,
            receipt.fine.severity == OverdueSeverity::Severe && receipt.fine.overdueFine > 0,
            receipt.fine.damageFine == FinePolicy::kDamageFee,
            accepted && receipt.fine.total() == 0,
        };
    }

    TransactionId loanId_{0};
};

void expectEffects(const Effects& actual, const Effects& expected) {
    EXPECT_EQ(actual.e20ReaderRejected, expected.e20ReaderRejected) << "effect 20";
    EXPECT_EQ(actual.e21CopyRejected, expected.e21CopyRejected) << "effect 21";
    EXPECT_EQ(actual.e22NoLoanRejected, expected.e22NoLoanRejected) << "effect 22";
    EXPECT_EQ(actual.e23LoanClosed, expected.e23LoanClosed) << "effect 23";
    EXPECT_EQ(actual.e24ReturnDateRecorded, expected.e24ReturnDateRecorded) << "effect 24";
    EXPECT_EQ(actual.e25CopyAvailable, expected.e25CopyAvailable) << "effect 25";
    EXPECT_EQ(actual.e26CopyMaintenance, expected.e26CopyMaintenance) << "effect 26";
    EXPECT_EQ(actual.e27RegularFine, expected.e27RegularFine) << "effect 27";
    EXPECT_EQ(actual.e28SevereFine, expected.e28SevereFine) << "effect 28";
    EXPECT_EQ(actual.e29DamageFee, expected.e29DamageFee) << "effect 29";
    EXPECT_EQ(actual.e30NoFine, expected.e30NoFine) << "effect 30";
}

void expectConstraints(const Effects& e) {
    const int outcomes = e.e20ReaderRejected + e.e21CopyRejected + e.e22NoLoanRejected + e.e23LoanClosed;
    EXPECT_EQ(outcomes, 1) << "O(20, 21, 22, 23)";
    EXPECT_EQ(e.e23LoanClosed, e.e24ReturnDateRecorded) << "R(23, 24)";
    EXPECT_FALSE(e.e25CopyAvailable && e.e26CopyMaintenance) << "E(25, 26)";
    EXPECT_FALSE(e.e27RegularFine && e.e28SevereFine) << "E(27, 28)";
    EXPECT_FALSE(e.e30NoFine && (e.e27RegularFine || e.e28SevereFine || e.e29DamageFee)) << "M(30; 27, 28, 29)";
}

TEST_P(ReturnBookCauseEffectTest, ProducesEffectsOfColumn) {
    const CausalColumn& column = GetParam();
    const std::optional<BookStatus> statusBefore =
        service_.copies().find(column.copy) != nullptr ? std::optional{copyStatus(column.copy)} : std::nullopt;

    const ReturnReceipt receipt = service_.returnBook(column.reader, column.copy, column.returnDate, column.condition);
    const Effects actual = observe(receipt, column.copy, statusBefore);

    expectEffects(actual, column.expected);
    expectConstraints(actual);
}

constexpr Money kRegularFine = 10 * FinePolicy::kRegularDailyRate;
constexpr Money kSevereFine = 40 * FinePolicy::kSevereDailyRate;

constexpr CausalColumn kCausalColumns[] = {
    {"T1_Effect20", kUnregisteredReader, kTargetCopy, kOnTimeReturn, BookCondition::Intact,
     {true, false, false, false, false, false, false, false, false, false, false}},
    {"T2_Effect21", kRegisteredReader, kMissingCopy, kOnTimeReturn, BookCondition::Intact,
     {false, true, false, false, false, false, false, false, false, false, false}},
    {"T3_Effect22", kRegisteredReader, kLoanedToOtherReader, kOnTimeReturn, BookCondition::Intact,
     {false, false, true, false, false, false, false, false, false, false, false}},
    {"T4_Effects23_24_25_30", kRegisteredReader, kTargetCopy, kOnTimeReturn, BookCondition::Intact,
     {false, false, false, true, true, true, false, false, false, false, true}},
    {"T5_Effects23_24_26_29", kRegisteredReader, kTargetCopy, kOnTimeReturn, BookCondition::Damaged,
     {false, false, false, true, true, false, true, false, false, true, false}},
    {"T6_Effects23_24_25_27", kRegisteredReader, kTargetCopy, kRegularOverdueReturn, BookCondition::Intact,
     {false, false, false, true, true, true, false, true, false, false, false}},
    {"T7_Effects23_24_26_27_29", kRegisteredReader, kTargetCopy, kRegularOverdueReturn, BookCondition::Damaged,
     {false, false, false, true, true, false, true, true, false, true, false}},
    {"T8_Effects23_24_25_28", kRegisteredReader, kTargetCopy, kSevereOverdueReturn, BookCondition::Intact,
     {false, false, false, true, true, true, false, false, true, false, false}},
    {"T9_Effects23_24_26_28_29", kRegisteredReader, kTargetCopy, kSevereOverdueReturn, BookCondition::Damaged,
     {false, false, false, true, true, false, true, false, true, true, false}},
};

INSTANTIATE_TEST_SUITE_P(ReturnBook, ReturnBookCauseEffectTest, ::testing::ValuesIn(kCausalColumns),
                         [](const ::testing::TestParamInfo<CausalColumn>& info) {
                             return std::string{info.param.name};
                         });

class ReturnBookFineAmountTest : public ReturnBookCauseEffectTest {};

TEST_F(ReturnBookFineAmountTest, RegularOverdueIsChargedFivePerDay) {
    const ReturnReceipt receipt = service_.returnBook(kRegisteredReader, kTargetCopy, kRegularOverdueReturn);
    EXPECT_EQ(receipt.fine.overdueDays, 10);
    EXPECT_EQ(receipt.fine.total(), kRegularFine);
}

TEST_F(ReturnBookFineAmountTest, SevereOverdueWithDamageIsChargedTenPerDayPlusFee) {
    const ReturnReceipt receipt =
        service_.returnBook(kRegisteredReader, kTargetCopy, kSevereOverdueReturn, BookCondition::Damaged);
    EXPECT_EQ(receipt.fine.overdueDays, 40);
    EXPECT_EQ(receipt.fine.total(), kSevereFine + FinePolicy::kDamageFee);
}

TEST_F(ReturnBookFineAmountTest, RejectedReturnLeavesLoanAndCopyUntouched) {
    const ReturnReceipt receipt =
        service_.returnBook(kUnregisteredReader, kTargetCopy, kSevereOverdueReturn, BookCondition::Damaged);

    EXPECT_EQ(receipt.fine, FineBreakdown{});
    EXPECT_TRUE(targetLoan().isOpen());
    EXPECT_EQ(copyStatus(kTargetCopy), BookStatus::Borrowed);
}

TEST_F(ReturnBookFineAmountTest, SecondReturnOfSameCopyHasNoActiveLoan) {
    ASSERT_TRUE(service_.returnBook(kRegisteredReader, kTargetCopy, kOnTimeReturn).returned());

    EXPECT_EQ(service_.returnBook(kRegisteredReader, kTargetCopy, kOnTimeReturn).outcome, ReturnOutcome::NoActiveLoan);
}

TEST_F(ReturnBookFineAmountTest, CopyInMaintenanceCannotBeIssued) {
    ASSERT_TRUE(service_.returnBook(kRegisteredReader, kTargetCopy, kOnTimeReturn, BookCondition::Damaged).returned());

    EXPECT_EQ(service_.borrowBook(kOtherReader, kTargetCopy, kOnTimeReturn).result, BorrowResult::CopyUnavailable);
}

}  // namespace
}  // namespace library::test
