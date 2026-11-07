#include "library/library_service.hpp"

#include <utility>

namespace library {

RegistrationResult LibraryService::registerReader(ReaderId id, std::string fullName, std::string email,
                                                  Date registrationDate) {
    const bool added = readers_.add(Reader{id, std::move(fullName), std::move(email), registrationDate});
    return added ? RegistrationResult::Registered : RegistrationResult::DuplicateId;
}

bool LibraryService::addBook(Book book) {
    return catalog_.add(std::move(book));
}

bool LibraryService::addCopy(BookCopy copy) {
    if (!catalog_.contains(copy.isbn())) {
        return false;
    }
    return copies_.add(std::move(copy));
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
