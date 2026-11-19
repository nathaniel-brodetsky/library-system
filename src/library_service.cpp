#include "library/library_service.hpp"

#include <utility>

#include "library/reader_validator.hpp"

namespace library {

RegistrationResult LibraryService::registerReader(ReaderId id, std::string fullName, std::string email,
                                                  Date registrationDate) {
    if (ReaderValidator::checkId(id) != ReaderIdValidity::Valid) {
        return RegistrationResult::InvalidId;
    }
    if (ReaderValidator::isBlank(fullName)) {
        return RegistrationResult::EmptyName;
    }
    if (ReaderValidator::checkEmail(email) != EmailValidity::Valid) {
        return RegistrationResult::InvalidEmail;
    }
    const bool added = readers_.add(Reader{id, std::move(fullName), std::move(email), registrationDate});
    return added ? RegistrationResult::Registered : RegistrationResult::DuplicateId;
}

CatalogResult LibraryService::addBook(Book book, Date currentDate) {
    const int currentYear = static_cast<int>(std::chrono::year_month_day{currentDate}.year());
    const CatalogResult validity = BookValidator::check(book, currentYear);
    if (validity != CatalogResult::Added) {
        return validity;
    }
    return catalog_.add(std::move(book)) ? CatalogResult::Added : CatalogResult::Duplicate;
}

CatalogResult LibraryService::addCopy(BookCopy copy) {
    if (!BookValidator::isValidInventoryNumber(copy.inventoryNumber())) {
        return CatalogResult::InvalidInventoryNumber;
    }
    if (!catalog_.contains(copy.isbn())) {
        return CatalogResult::UnknownBook;
    }
    return copies_.add(std::move(copy)) ? CatalogResult::Added : CatalogResult::Duplicate;
}

BorrowReceipt LibraryService::borrowBook(ReaderId readerId, CopyId copyId, Date currentDate) {
    transactions_.refreshOverdue(currentDate);

    const BorrowResult eligibility = checkBorrowEligibility(readerId, copyId, currentDate);
    if (eligibility != BorrowResult::Issued) {
        return {eligibility, std::nullopt, std::nullopt};
    }

    const Date dueDate = addDays(currentDate, policy_.loanPeriodDays);
    const TransactionId transactionId = transactions_.open(readerId, copyId, currentDate, dueDate);
    copies_.find(copyId)->setStatus(BookStatus::Borrowed);
    return {BorrowResult::Issued, transactionId, dueDate};
}

ReturnReceipt LibraryService::returnBook(ReaderId readerId, CopyId copyId, Date returnDate,
                                         BookCondition condition) {
    if (!readers_.contains(readerId)) {
        return {ReturnOutcome::ReaderNotRegistered, {}};
    }
    BookCopy* copy = copies_.find(copyId);
    if (copy == nullptr) {
        return {ReturnOutcome::CopyNotFound, {}};
    }
    Transaction* loan = transactions_.findOpen(readerId, copyId);
    if (loan == nullptr) {
        return {ReturnOutcome::NoActiveLoan, {}};
    }
    if (returnDate < loan->issueDate()) {
        return {ReturnOutcome::InvalidReturnDate, {}};
    }

    const FineBreakdown fine = FinePolicy::calculateFine(loan->dueDate(), returnDate, condition);
    loan->close(returnDate);
    copy->setStatus(condition == BookCondition::Damaged ? BookStatus::Maintenance : BookStatus::Available);
    return {ReturnOutcome::Returned, fine};
}

RenewReceipt LibraryService::renewLoan(ReaderId readerId, CopyId copyId, Date currentDate) {
    if (!readers_.contains(readerId)) {
        return {RenewResult::ReaderNotRegistered, std::nullopt};
    }
    Transaction* loan = transactions_.findOpen(readerId, copyId);
    if (loan == nullptr) {
        return {RenewResult::NoActiveLoan, std::nullopt};
    }
    const int daysUntilDue = daysBetween(currentDate, loan->dueDate());
    if (daysUntilDue < 0) {
        return {RenewResult::Overdue, std::nullopt};
    }
    if (daysUntilDue > policy_.renewalWindowDays) {
        return {RenewResult::TooEarly, std::nullopt};
    }
    if (loan->renewals() >= policy_.maxRenewals) {
        return {RenewResult::RenewalLimitReached, std::nullopt};
    }
    const Date newDueDate = addDays(loan->dueDate(), policy_.loanPeriodDays);
    loan->renew(newDueDate);
    return {RenewResult::Renewed, newDueDate};
}

BorrowResult LibraryService::checkBorrowEligibility(ReaderId readerId, CopyId copyId, Date currentDate) const {
    if (!readers_.contains(readerId)) {
        return BorrowResult::ReaderNotRegistered;
    }
    if (transactions_.hasOverdue(readerId, currentDate)) {
        return BorrowResult::OverdueDebt;
    }
    if (transactions_.countOpen(readerId) >= policy_.maxActiveLoans) {
        return BorrowResult::LoanLimitReached;
    }
    const BookCopy* copy = copies_.find(copyId);
    if (copy == nullptr || !copy->isAvailable()) {
        return BorrowResult::CopyUnavailable;
    }
    return BorrowResult::Issued;
}

}  // namespace library
