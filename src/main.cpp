#include <iostream>
#include <string_view>

#include "library/library_service.hpp"

namespace {

std::string_view describe(library::BorrowResult result) {
    using library::BorrowResult;
    switch (result) {
        case BorrowResult::Issued:
            return "Книгу видано";
        case BorrowResult::ReaderNotRegistered:
            return "Відмова: читач не зареєстрований";
        case BorrowResult::OverdueDebt:
            return "Відмова: читач має прострочену заборгованість";
        case BorrowResult::LoanLimitReached:
            return "Відмова: перевищено ліміт активних видач";
        case BorrowResult::CopyUnavailable:
            return "Відмова: примірник недоступний";
    }
    return {};
}

void report(const library::BorrowReceipt& receipt) {
    std::cout << describe(receipt.result);
    if (receipt.dueDate) {
        std::cout << " (повернути до " << library::formatDate(*receipt.dueDate) << ')';
    }
    std::cout << '\n';
}

}  // namespace

int main() {
    using namespace library;

    LibraryService service;
    const Date today = makeDate(2025, 3, 10);

    service.registerReader(1, "Шевченко Тарас", "taras@example.com", today);
    service.addBook(Book{"978-966-03-4058-9", "Кобзар", "Тарас Шевченко", 1840}, today);
    service.addCopy(BookCopy{101, "978-966-03-4058-9"});

    report(service.borrowBook(1, 101, today));
    report(service.borrowBook(1, 101, today));
    report(service.borrowBook(7, 101, today));

    const RenewReceipt renewal = service.renewLoan(1, 101, addDays(today, 18));
    if (renewal.dueDate) {
        std::cout << "Видачу продовжено до " << formatDate(*renewal.dueDate) << '\n';
    }

    const ReturnReceipt receipt = service.returnBook(1, 101, addDays(today, 46));
    std::cout << "Повернення: прострочено " << receipt.fine.overdueDays << " дн., штраф " << receipt.fine.total()
              << " грн\n";
    return 0;
}
