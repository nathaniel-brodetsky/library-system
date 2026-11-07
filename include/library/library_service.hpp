#pragma once

#include <optional>
#include <string>

#include "library/book.hpp"
#include "library/date.hpp"
#include "library/fine_policy.hpp"
#include "library/identifiers.hpp"
#include "library/loan_policy.hpp"
#include "library/reader.hpp"
#include "library/registries.hpp"

namespace library {

enum class RegistrationResult { Registered, DuplicateId };

enum class BorrowResult { Issued, ReaderNotRegistered, OverdueDebt, LoanLimitReached, CopyUnavailable };

struct BorrowReceipt {
    BorrowResult result;
    std::optional<TransactionId> transactionId;
    std::optional<Date> dueDate;

    [[nodiscard]] bool issued() const noexcept { return result == BorrowResult::Issued; }
};

enum class ReturnOutcome { Returned, ReaderNotRegistered, CopyNotFound, NoActiveLoan, InvalidReturnDate };

struct ReturnReceipt {
    ReturnOutcome outcome;
    FineBreakdown fine;

    [[nodiscard]] bool returned() const noexcept { return outcome == ReturnOutcome::Returned; }
};

class LibraryService {
public:
    explicit LibraryService(LoanPolicy policy = {}) noexcept : policy_(policy) {}

    RegistrationResult registerReader(ReaderId id, std::string fullName, std::string email, Date registrationDate);
    bool addBook(Book book);
    bool addCopy(BookCopy copy);

    BorrowReceipt borrowBook(ReaderId readerId, CopyId copyId, Date currentDate);
    ReturnReceipt returnBook(ReaderId readerId, CopyId copyId, Date returnDate,
                             BookCondition condition = BookCondition::Intact);

    [[nodiscard]] const LoanPolicy& policy() const noexcept { return policy_; }
    [[nodiscard]] const ReaderRegistry& readers() const noexcept { return readers_; }
    [[nodiscard]] const BookCatalog& catalog() const noexcept { return catalog_; }
    [[nodiscard]] const CopyRegistry& copies() const noexcept { return copies_; }
    [[nodiscard]] const TransactionRegistry& transactions() const noexcept { return transactions_; }

private:
    [[nodiscard]] BorrowResult checkBorrowEligibility(ReaderId readerId, CopyId copyId, Date currentDate) const;

    LoanPolicy policy_;
    ReaderRegistry readers_;
    BookCatalog catalog_;
    CopyRegistry copies_;
    TransactionRegistry transactions_;
};

}  // namespace library
