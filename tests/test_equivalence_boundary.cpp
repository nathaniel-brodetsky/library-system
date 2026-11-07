#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>

#include "library/reader_validator.hpp"
#include "library_test_support.hpp"

namespace library::test {
namespace {

static_assert(ReaderValidator::checkId(ReaderValidator::kMinReaderId) == ReaderIdValidity::Valid);
static_assert(ReaderValidator::checkId(ReaderValidator::kMaxReaderId) == ReaderIdValidity::Valid);
static_assert(FinePolicy::classify(FinePolicy::kSevereOverdueThresholdDays) == OverdueSeverity::Regular);
static_assert(FinePolicy::classify(FinePolicy::kSevereOverdueThresholdDays + 1) == OverdueSeverity::Severe);

constexpr std::string_view kValidName = "Франко Іван";
constexpr std::string_view kValidEmail = "ivan.franko@example.com";

std::string repeat(char symbol, std::size_t count) {
    return std::string(count, symbol);
}

std::string domainOfLength(std::size_t length) {
    constexpr std::string_view kTopLevelDomain = ".com";
    std::string domain;
    std::size_t remaining = length - kTopLevelDomain.size();
    while (remaining > 0) {
        if (!domain.empty()) {
            domain += '.';
            --remaining;
        }
        const std::size_t labelLength = std::min(ReaderValidator::kMaxDomainLabelLength, remaining);
        domain += repeat('d', labelLength);
        remaining -= labelLength;
    }
    return domain + std::string{kTopLevelDomain};
}

std::string emailOfLength(std::size_t length) {
    const std::string localPart = repeat('l', ReaderValidator::kMaxLocalPartLength);
    return localPart + "@" + domainOfLength(length - localPart.size() - 1);
}

struct ReaderIdCase {
    std::string_view label;
    ReaderId id;
    ReaderIdValidity validity;
    RegistrationResult registration;
};

void PrintTo(const ReaderIdCase& value, std::ostream* os) {
    *os << value.label << " id=" << value.id;
}

class ReaderIdPartitionTest : public ::testing::TestWithParam<ReaderIdCase> {};

TEST_P(ReaderIdPartitionTest, ClassifiesAndRegisters) {
    const ReaderIdCase& value = GetParam();
    LibraryService service;

    EXPECT_EQ(ReaderValidator::checkId(value.id), value.validity);
    EXPECT_EQ(service.registerReader(value.id, std::string{kValidName}, std::string{kValidEmail}, kRegistrationDate),
              value.registration);
    EXPECT_EQ(service.readers().contains(value.id), value.registration == RegistrationResult::Registered);
}

constexpr ReaderIdCase kReaderIdCases[] = {
    {"EP_NegativeInvalid", -5, ReaderIdValidity::BelowMinimum, RegistrationResult::InvalidId},
    {"BVA_MinMinus1", 0, ReaderIdValidity::BelowMinimum, RegistrationResult::InvalidId},
    {"BVA_Min", 1, ReaderIdValidity::Valid, RegistrationResult::Registered},
    {"BVA_MinPlus1", 2, ReaderIdValidity::Valid, RegistrationResult::Registered},
    {"BVA_Nominal", 500'000, ReaderIdValidity::Valid, RegistrationResult::Registered},
    {"BVA_MaxMinus1", 999'998, ReaderIdValidity::Valid, RegistrationResult::Registered},
    {"BVA_Max", 999'999, ReaderIdValidity::Valid, RegistrationResult::Registered},
    {"BVA_MaxPlus1", 1'000'000, ReaderIdValidity::AboveMaximum, RegistrationResult::InvalidId},
    {"EP_LargeInvalid", std::numeric_limits<ReaderId>::max(), ReaderIdValidity::AboveMaximum,
     RegistrationResult::InvalidId},
};

INSTANTIATE_TEST_SUITE_P(ReaderId, ReaderIdPartitionTest, ::testing::ValuesIn(kReaderIdCases),
                         [](const ::testing::TestParamInfo<ReaderIdCase>& info) {
                             return std::string{info.param.label};
                         });

class NonExistentReaderTest : public LibraryTest {};

TEST_F(NonExistentReaderTest, WellFormedButUnregisteredIdCannotBorrow) {
    constexpr ReaderId kWellFormedUnknownId = 500'000;
    ASSERT_EQ(ReaderValidator::checkId(kWellFormedUnknownId), ReaderIdValidity::Valid);

    EXPECT_EQ(service_.borrowBook(kWellFormedUnknownId, kTargetCopy, kToday).result,
              BorrowResult::ReaderNotRegistered);
}

TEST_F(NonExistentReaderTest, WellFormedButUnregisteredIdCannotReturn) {
    EXPECT_EQ(service_.returnBook(500'000, kTargetCopy, kToday).outcome, ReturnOutcome::ReaderNotRegistered);
}

TEST_F(NonExistentReaderTest, DuplicateIdIsRejected) {
    EXPECT_EQ(service_.registerReader(kRegisteredReader, std::string{kValidName}, std::string{kValidEmail},
                                      kRegistrationDate),
              RegistrationResult::DuplicateId);
}

struct EmailCase {
    std::string_view label;
    std::string email;
    EmailValidity expected;
};

void PrintTo(const EmailCase& value, std::ostream* os) {
    *os << value.label << " (" << value.email.size() << " chars)";
}

class EmailPartitionTest : public ::testing::TestWithParam<EmailCase> {};

TEST_P(EmailPartitionTest, ClassifiesEmail) {
    const EmailCase& value = GetParam();

    EXPECT_EQ(ReaderValidator::checkEmail(value.email), value.expected);

    LibraryService service;
    const RegistrationResult expectedRegistration =
        value.expected == EmailValidity::Valid ? RegistrationResult::Registered : RegistrationResult::InvalidEmail;
    EXPECT_EQ(service.registerReader(1, std::string{kValidName}, value.email, kRegistrationDate),
              expectedRegistration);
}

INSTANTIATE_TEST_SUITE_P(
    EquivalenceClasses, EmailPartitionTest,
    ::testing::Values(EmailCase{"Valid_Simple", "taras@example.com", EmailValidity::Valid},
                      EmailCase{"Valid_DotsPlusSubdomains", "taras.shevchenko+lib@stud.op.edu.ua",
                                EmailValidity::Valid},
                      EmailCase{"Valid_HyphenatedDomain", "o'brien@my-library.org", EmailValidity::Valid},
                      EmailCase{"Invalid_Empty", "", EmailValidity::Empty},
                      EmailCase{"Invalid_MissingAt", "taras.example.com", EmailValidity::MissingAt},
                      EmailCase{"Invalid_MultipleAt", "taras@home@example.com", EmailValidity::MultipleAt},
                      EmailCase{"Invalid_EmptyLocalPart", "@example.com", EmailValidity::InvalidLocalPart},
                      EmailCase{"Invalid_LeadingDot", ".taras@example.com", EmailValidity::InvalidLocalPart},
                      EmailCase{"Invalid_ConsecutiveDots", "taras..s@example.com", EmailValidity::InvalidLocalPart},
                      EmailCase{"Invalid_NonAsciiLocalPart", "тарас@example.com", EmailValidity::InvalidLocalPart},
                      EmailCase{"Invalid_SingleLabelDomain", "taras@localhost", EmailValidity::InvalidDomain},
                      EmailCase{"Invalid_LabelStartsWithHyphen", "taras@-example.com", EmailValidity::InvalidDomain},
                      EmailCase{"Invalid_EmptyLabel", "taras@example..com", EmailValidity::InvalidDomain},
                      EmailCase{"Invalid_TrailingDot", "taras@example.com.", EmailValidity::InvalidDomain},
                      EmailCase{"Invalid_OneLetterTld", "taras@example.c", EmailValidity::InvalidDomain},
                      EmailCase{"Invalid_NumericTld", "taras@example.123", EmailValidity::InvalidDomain}),
    [](const ::testing::TestParamInfo<EmailCase>& info) { return std::string{info.param.label}; });

INSTANTIATE_TEST_SUITE_P(
    LengthBoundaries, EmailPartitionTest,
    ::testing::Values(
        EmailCase{"LocalPart_MaxMinus1", repeat('l', 63) + "@example.com", EmailValidity::Valid},
        EmailCase{"LocalPart_Max", repeat('l', 64) + "@example.com", EmailValidity::Valid},
        EmailCase{"LocalPart_MaxPlus1", repeat('l', 65) + "@example.com", EmailValidity::LocalPartTooLong},
        EmailCase{"DomainLabel_MaxMinus1", "taras@" + repeat('d', 62) + ".com", EmailValidity::Valid},
        EmailCase{"DomainLabel_Max", "taras@" + repeat('d', 63) + ".com", EmailValidity::Valid},
        EmailCase{"DomainLabel_MaxPlus1", "taras@" + repeat('d', 64) + ".com", EmailValidity::DomainLabelTooLong},
        EmailCase{"Total_MaxMinus1", emailOfLength(253), EmailValidity::Valid},
        EmailCase{"Total_Max", emailOfLength(254), EmailValidity::Valid},
        EmailCase{"Total_MaxPlus1", emailOfLength(255), EmailValidity::TooLong}),
    [](const ::testing::TestParamInfo<EmailCase>& info) { return std::string{info.param.label}; });

TEST(EmailLengthGenerator, ProducesExactLengths) {
    EXPECT_EQ(emailOfLength(253).size(), 253U);
    EXPECT_EQ(emailOfLength(254).size(), 254U);
    EXPECT_EQ(emailOfLength(255).size(), 255U);
}

struct NameCase {
    std::string_view label;
    std::string_view fullName;
    RegistrationResult expected;
};

void PrintTo(const NameCase& value, std::ostream* os) {
    *os << value.label << " \"" << value.fullName << '"';
}

class ReaderNamePartitionTest : public ::testing::TestWithParam<NameCase> {};

TEST_P(ReaderNamePartitionTest, ValidatesFullName) {
    const NameCase& value = GetParam();
    LibraryService service;

    EXPECT_EQ(service.registerReader(1, std::string{value.fullName}, std::string{kValidEmail}, kRegistrationDate),
              value.expected);
}

INSTANTIATE_TEST_SUITE_P(FullName, ReaderNamePartitionTest,
                         ::testing::Values(NameCase{"Valid", kValidName, RegistrationResult::Registered},
                                           NameCase{"Empty", "", RegistrationResult::EmptyName},
                                           NameCase{"WhitespaceOnly", " \t ", RegistrationResult::EmptyName}),
                         [](const ::testing::TestParamInfo<NameCase>& info) { return std::string{info.param.label}; });

TEST(RegistrationPrecedence, InvalidIdIsReportedBeforeInvalidEmail) {
    LibraryService service;

    EXPECT_EQ(service.registerReader(0, "", "", kRegistrationDate), RegistrationResult::InvalidId);
}

inline constexpr Date kLoanIssueDate = makeDate(2025, 3, 1);
inline constexpr Date kLoanDueDate = addDays(kLoanIssueDate, LoanPolicy::kDefaultLoanPeriodDays);

struct ReturnDateCase {
    std::string_view label;
    int offsetFromDue;
    ReturnOutcome outcome;
    int overdueDays;
    OverdueSeverity severity;
    Money fine;
};

void PrintTo(const ReturnDateCase& value, std::ostream* os) {
    *os << value.label << " due" << (value.offsetFromDue >= 0 ? "+" : "") << value.offsetFromDue;
}

class ReturnDateBoundaryTest : public LibraryTest, public ::testing::WithParamInterface<ReturnDateCase> {
protected:
    void SetUp() override {
        LibraryTest::SetUp();
        borrowOrFail(kRegisteredReader, kTargetCopy, kLoanIssueDate);
    }
};

TEST_P(ReturnDateBoundaryTest, ComputesOverdueAndFine) {
    const ReturnDateCase& value = GetParam();
    const Date returnDate = addDays(kLoanDueDate, value.offsetFromDue);

    const ReturnReceipt receipt = service_.returnBook(kRegisteredReader, kTargetCopy, returnDate);

    EXPECT_EQ(receipt.outcome, value.outcome);
    EXPECT_EQ(receipt.fine.overdueDays, value.overdueDays);
    EXPECT_EQ(receipt.fine.severity, value.severity);
    EXPECT_EQ(receipt.fine.total(), value.fine);
    if (value.outcome == ReturnOutcome::Returned) {
        EXPECT_EQ(copyStatus(kTargetCopy), BookStatus::Available);
    } else {
        EXPECT_EQ(receipt.fine, FineBreakdown{});
        EXPECT_EQ(service_.transactions().countOpen(kRegisteredReader), 1U);
        EXPECT_EQ(copyStatus(kTargetCopy), BookStatus::Borrowed);
    }
}

constexpr int kIssueOffset = -LoanPolicy::kDefaultLoanPeriodDays;
constexpr int kThreshold = FinePolicy::kSevereOverdueThresholdDays;

constexpr ReturnDateCase kReturnDateCases[] = {
    {"Invalid_BeforeIssueDate", kIssueOffset - 1, ReturnOutcome::InvalidReturnDate, 0, OverdueSeverity::None, 0},
    {"Past_IssueDay", kIssueOffset, ReturnOutcome::Returned, 0, OverdueSeverity::None, 0},
    {"Past_DayBeforeDue", -1, ReturnOutcome::Returned, 0, OverdueSeverity::None, 0},
    {"ExactDueDate", 0, ReturnOutcome::Returned, 0, OverdueSeverity::None, 0},
    {"Overdue_Plus1Day", 1, ReturnOutcome::Returned, 1, OverdueSeverity::Regular, 5},
    {"Overdue_Plus2Days", 2, ReturnOutcome::Returned, 2, OverdueSeverity::Regular, 10},
    {"Overdue_Nominal15", 15, ReturnOutcome::Returned, 15, OverdueSeverity::Regular, 75},
    {"Overdue_ThresholdMinus1", kThreshold - 1, ReturnOutcome::Returned, 29, OverdueSeverity::Regular, 145},
    {"Overdue_Threshold30", kThreshold, ReturnOutcome::Returned, 30, OverdueSeverity::Regular, 150},
    {"Severe_ThresholdPlus1", kThreshold + 1, ReturnOutcome::Returned, 31, OverdueSeverity::Severe, 310},
    {"Severe_Nominal60", 60, ReturnOutcome::Returned, 60, OverdueSeverity::Severe, 600},
};

INSTANTIATE_TEST_SUITE_P(LoanDates, ReturnDateBoundaryTest, ::testing::ValuesIn(kReturnDateCases),
                         [](const ::testing::TestParamInfo<ReturnDateCase>& info) {
                             return std::string{info.param.label};
                         });

struct LoanLimitCase {
    std::string_view label;
    std::size_t openLoansBefore;
    BorrowResult expected;
};

void PrintTo(const LoanLimitCase& value, std::ostream* os) {
    *os << value.label << " open=" << value.openLoansBefore;
}

class LoanLimitBoundaryTest : public LibraryTest, public ::testing::WithParamInterface<LoanLimitCase> {};

TEST_P(LoanLimitBoundaryTest, EnforcesMaximumActiveLoans) {
    const LoanLimitCase& value = GetParam();
    CopyId nextCopy = kFirstSpareCopy;
    for (std::size_t loan = 0; loan < value.openLoansBefore; ++loan) {
        borrowOrFail(kRegisteredReader, nextCopy++, kToday);
    }

    EXPECT_EQ(service_.borrowBook(kRegisteredReader, kTargetCopy, kToday).result, value.expected);
}

INSTANTIATE_TEST_SUITE_P(
    ActiveLoans, LoanLimitBoundaryTest,
    ::testing::Values(LoanLimitCase{"Min_NoLoans", 0, BorrowResult::Issued},
                      LoanLimitCase{"MaxMinus1", LoanPolicy::kDefaultMaxActiveLoans - 1, BorrowResult::Issued},
                      LoanLimitCase{"Max", LoanPolicy::kDefaultMaxActiveLoans, BorrowResult::LoanLimitReached}),
    [](const ::testing::TestParamInfo<LoanLimitCase>& info) { return std::string{info.param.label}; });

}  // namespace
}  // namespace library::test
