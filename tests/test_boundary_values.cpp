#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "library_test_support.hpp"

namespace library::test {
namespace {

constexpr int kCurrentYear = 2025;
constexpr int kLoanDays = LoanPolicy::kDefaultLoanPeriodDays;
constexpr int kWindow = LoanPolicy::kDefaultRenewalWindowDays;
constexpr std::size_t kMaxTitle = BookValidator::kMaxTitleLength;

static_assert(BookValidator::isValidInventoryNumber(BookValidator::kMinInventoryNumber));
static_assert(BookValidator::isValidInventoryNumber(BookValidator::kMaxInventoryNumber));
static_assert(!BookValidator::isValidInventoryNumber(BookValidator::kMaxInventoryNumber + 1));

std::string cyrillicTitle(std::size_t letters) {
    std::string title;
    for (std::size_t i = 0; i < letters; ++i) {
        title += "К";
    }
    return title;
}

Book bookWith(std::string isbn, std::string title, int year) {
    return Book{std::move(isbn), std::move(title), "Тарас Шевченко", year};
}

template <typename Case>
std::string caseName(const ::testing::TestParamInfo<Case>& info) {
    return info.param.name;
}

struct IsbnCase {
    std::string name;
    std::string isbn;
    IsbnValidity expected;
};

void PrintTo(const IsbnCase& param, std::ostream* os) {
    *os << param.name;
}

class IsbnLengthBoundaryTest : public ::testing::TestWithParam<IsbnCase> {};

TEST_P(IsbnLengthBoundaryTest, ChecksDigitCount) {
    const IsbnCase& param = GetParam();
    EXPECT_EQ(BookValidator::checkIsbn(param.isbn), param.expected);
}

INSTANTIATE_TEST_SUITE_P(IsbnDigits, IsbnLengthBoundaryTest,
                         ::testing::Values(IsbnCase{"Digits12_Before", "978966034058", IsbnValidity::WrongLength},
                                           IsbnCase{"Digits13_On", "978-966-03-4058-9", IsbnValidity::Valid},
                                           IsbnCase{"Digits14_After", "97896603405890", IsbnValidity::WrongLength}),
                         caseName<IsbnCase>);

struct CatalogCase {
    std::string name;
    std::size_t titleLength;
    int year;
    CatalogResult expected;
};

void PrintTo(const CatalogCase& param, std::ostream* os) {
    *os << param.name;
}

class CatalogBoundaryTest : public ::testing::TestWithParam<CatalogCase> {
protected:
    LibraryService service_;
};

TEST_P(CatalogBoundaryTest, ValidatesBookOnAdd) {
    const CatalogCase& param = GetParam();
    const Book book = bookWith(std::string{kIsbn}, cyrillicTitle(param.titleLength), param.year);
    EXPECT_EQ(service_.addBook(book, kToday), param.expected);
    EXPECT_EQ(service_.catalog().contains(kIsbn), param.expected == CatalogResult::Added);
}

INSTANTIATE_TEST_SUITE_P(
    TitleLength, CatalogBoundaryTest,
    ::testing::Values(CatalogCase{"Min_Before_0", 0, 1840, CatalogResult::EmptyTitle},
                      CatalogCase{"Min_On_1", 1, 1840, CatalogResult::Added},
                      CatalogCase{"Min_After_2", 2, 1840, CatalogResult::Added},
                      CatalogCase{"Max_Before_199", kMaxTitle - 1, 1840, CatalogResult::Added},
                      CatalogCase{"Max_On_200", kMaxTitle, 1840, CatalogResult::Added},
                      CatalogCase{"Max_After_201", kMaxTitle + 1, 1840, CatalogResult::TitleTooLong}),
    caseName<CatalogCase>);

INSTANTIATE_TEST_SUITE_P(
    PublicationYear, CatalogBoundaryTest,
    ::testing::Values(CatalogCase{"Min_Before_1449", 6, 1449, CatalogResult::YearTooEarly},
                      CatalogCase{"Min_On_1450", 6, 1450, CatalogResult::Added},
                      CatalogCase{"Min_After_1451", 6, 1451, CatalogResult::Added},
                      CatalogCase{"Max_Before_2024", 6, kCurrentYear - 1, CatalogResult::Added},
                      CatalogCase{"Max_On_2025", 6, kCurrentYear, CatalogResult::Added},
                      CatalogCase{"Max_After_2026", 6, kCurrentYear + 1, CatalogResult::YearInFuture}),
    caseName<CatalogCase>);

TEST(TitleLengthUnits, CountsCharactersNotBytes) {
    const std::string title = cyrillicTitle(kMaxTitle);
    EXPECT_EQ(title.size(), 2 * kMaxTitle);
    EXPECT_EQ(BookValidator::countCharacters(title), kMaxTitle);
}

struct InventoryCase {
    std::string name;
    CopyId inventoryNumber;
    CatalogResult expected;
};

void PrintTo(const InventoryCase& param, std::ostream* os) {
    *os << param.name;
}

class InventoryBoundaryTest : public LibraryTest, public ::testing::WithParamInterface<InventoryCase> {};

TEST_P(InventoryBoundaryTest, ValidatesInventoryNumber) {
    const InventoryCase& param = GetParam();
    EXPECT_EQ(service_.addCopy(BookCopy{param.inventoryNumber, std::string{kIsbn}}), param.expected);
    EXPECT_EQ(service_.copies().find(param.inventoryNumber) != nullptr, param.expected == CatalogResult::Added);
}

INSTANTIATE_TEST_SUITE_P(
    InventoryNumber, InventoryBoundaryTest,
    ::testing::Values(InventoryCase{"Min_Before_0", 0, CatalogResult::InvalidInventoryNumber},
                      InventoryCase{"Min_On_1", 1, CatalogResult::Added},
                      InventoryCase{"Min_After_2", 2, CatalogResult::Added},
                      InventoryCase{"Max_Before_999998", 999'998, CatalogResult::Added},
                      InventoryCase{"Max_On_999999", 999'999, CatalogResult::Added},
                      InventoryCase{"Max_After_1000000", 1'000'000, CatalogResult::InvalidInventoryNumber}),
    caseName<InventoryCase>);

struct DateOffsetCase {
    std::string name;
    int daysRelativeToDue;
    int expectedResult;
};

void PrintTo(const DateOffsetCase& param, std::ostream* os) {
    *os << param.name;
}

class LoanDateBoundaryTest : public LibraryTest {
protected:
    Date issueTargetCopy() {
        const BorrowReceipt receipt = service_.borrowBook(kRegisteredReader, kTargetCopy, kToday);
        EXPECT_TRUE(receipt.issued());
        EXPECT_EQ(receipt.dueDate, addDays(kToday, kLoanDays));
        return receipt.dueDate.value_or(kToday);
    }
};

class OverdueBorrowBoundaryTest : public LoanDateBoundaryTest, public ::testing::WithParamInterface<DateOffsetCase> {};

TEST_P(OverdueBorrowBoundaryTest, BlocksNewLoanOnlyAfterDueDate) {
    const DateOffsetCase& param = GetParam();
    const Date dueDate = issueTargetCopy();
    const BorrowReceipt receipt =
        service_.borrowBook(kRegisteredReader, kFirstSpareCopy, addDays(dueDate, param.daysRelativeToDue));
    EXPECT_EQ(static_cast<int>(receipt.result), param.expectedResult);
}

INSTANTIATE_TEST_SUITE_P(
    DueDate, OverdueBorrowBoundaryTest,
    ::testing::Values(DateOffsetCase{"Before_DueMinus1", -1, static_cast<int>(BorrowResult::Issued)},
                      DateOffsetCase{"On_Due", 0, static_cast<int>(BorrowResult::Issued)},
                      DateOffsetCase{"After_DuePlus1", 1, static_cast<int>(BorrowResult::OverdueDebt)}),
    caseName<DateOffsetCase>);

class RenewalDateBoundaryTest : public LoanDateBoundaryTest, public ::testing::WithParamInterface<DateOffsetCase> {};

TEST_P(RenewalDateBoundaryTest, AllowsRenewalOnlyInsideWindow) {
    const DateOffsetCase& param = GetParam();
    const Date dueDate = issueTargetCopy();
    const RenewReceipt receipt =
        service_.renewLoan(kRegisteredReader, kTargetCopy, addDays(dueDate, param.daysRelativeToDue));
    EXPECT_EQ(static_cast<int>(receipt.result), param.expectedResult);
    if (receipt.renewed()) {
        EXPECT_EQ(receipt.dueDate, addDays(dueDate, kLoanDays));
    } else {
        EXPECT_FALSE(receipt.dueDate.has_value());
    }
}

INSTANTIATE_TEST_SUITE_P(
    WindowStart, RenewalDateBoundaryTest,
    ::testing::Values(DateOffsetCase{"Before_8DaysToDue", -(kWindow + 1), static_cast<int>(RenewResult::TooEarly)},
                      DateOffsetCase{"On_7DaysToDue", -kWindow, static_cast<int>(RenewResult::Renewed)},
                      DateOffsetCase{"After_6DaysToDue", -(kWindow - 1), static_cast<int>(RenewResult::Renewed)}),
    caseName<DateOffsetCase>);

INSTANTIATE_TEST_SUITE_P(
    WindowEnd, RenewalDateBoundaryTest,
    ::testing::Values(DateOffsetCase{"Before_DueMinus1", -1, static_cast<int>(RenewResult::Renewed)},
                      DateOffsetCase{"On_Due", 0, static_cast<int>(RenewResult::Renewed)},
                      DateOffsetCase{"After_DuePlus1", 1, static_cast<int>(RenewResult::Overdue)}),
    caseName<DateOffsetCase>);

struct RenewalCountCase {
    std::string name;
    int requestNumber;
    RenewResult expected;
};

void PrintTo(const RenewalCountCase& param, std::ostream* os) {
    *os << param.name;
}

class RenewalCountBoundaryTest : public LoanDateBoundaryTest,
                                 public ::testing::WithParamInterface<RenewalCountCase> {};

TEST_P(RenewalCountBoundaryTest, LimitsNumberOfRenewals) {
    const RenewalCountCase& param = GetParam();
    Date dueDate = issueTargetCopy();
    RenewReceipt receipt{RenewResult::NoActiveLoan, std::nullopt};
    for (int request = 1; request <= param.requestNumber; ++request) {
        receipt = service_.renewLoan(kRegisteredReader, kTargetCopy, dueDate);
        if (receipt.dueDate) {
            dueDate = *receipt.dueDate;
        }
    }
    EXPECT_EQ(receipt.result, param.expected);
    const Transaction* loan = service_.transactions().find(1);
    ASSERT_NE(loan, nullptr);
    EXPECT_EQ(loan->renewals(), std::min(param.requestNumber, LoanPolicy::kDefaultMaxRenewals));
}

INSTANTIATE_TEST_SUITE_P(
    Renewals, RenewalCountBoundaryTest,
    ::testing::Values(RenewalCountCase{"Before_Request1", 1, RenewResult::Renewed},
                      RenewalCountCase{"On_Request2", 2, RenewResult::Renewed},
                      RenewalCountCase{"After_Request3", 3, RenewResult::RenewalLimitReached}),
    caseName<RenewalCountCase>);

}  // namespace
}  // namespace library::test
