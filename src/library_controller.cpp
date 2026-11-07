#include "library_controller.h"

#include <iostream>

std::string LibraryController::issueBook(int readerId, int inventoryNumber, const Date& date) {
    const Reader* currentReader = readerRegistry.findReader(readerId);
    if (currentReader == nullptr) {
        std::cout << "Помилка: читач не зареєстрований" << std::endl;
        return "Читач не зареєстрований";
    }

    std::vector<Transaction> active = transactionRegistry.findActiveTransactions(readerId);
    for (const auto& transaction : active) {
        if (date > transaction.getDueDate()) {
            std::cout << "Помилка: читач має заборгованість" << std::endl;
            return "Заборгованість";
        }
    }

    if (active.size() >= 3) {
        std::cout << "Помилка: перевищено ліміт" << std::endl;
        return "Ліміт";
    }

    BookCopy* currentCopy = copyRegistry.findCopy(inventoryNumber);
    if (currentCopy == nullptr || currentCopy->getStatus() != BookStatus::Available) {
        std::cout << "Помилка: примірник недоступний" << std::endl;
        return "Недоступний";
    }

    const Date expectedReturn = date + std::chrono::days{21};

    Transaction newTransaction(0, currentReader->getId(), currentCopy->getInventoryNumber(), date,
                              expectedReturn);
    transactionRegistry.recordTransaction(newTransaction);
    currentCopy->setStatus(BookStatus::Borrowed);
    std::cout << "Книгу видано" << std::endl;
    return "OK";
}
