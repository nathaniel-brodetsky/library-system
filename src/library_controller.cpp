#include "library_controller.h"

#include <iostream>

std::string LibraryController::issueBook(int readerId, int inventoryNumber, Date date) {
    Reader currentReader = readerRegistry.findReader(readerId);
    if (currentReader.getId() == 0) {
        std::cout << "Помилка: читач не зареєстрований" << std::endl;
        return "Читач не зареєстрований";
    }

    std::vector<Transaction> active = transactionRegistry.findActiveTransactions(readerId);
    for (int i = 0; i < (int)active.size(); i++) {
        if (toDays(date) > toDays(active[i].dueDate)) {
            std::cout << "Помилка: читач має заборгованість" << std::endl;
            return "Заборгованість";
        }
    }

    if (active.size() >= 3) {
        std::cout << "Помилка: перевищено ліміт" << std::endl;
        return "Ліміт";
    }

    BookCopy currentCopy = copyRegistry.findCopy(inventoryNumber);
    if (currentCopy.getStatus() != "доступний") {
        std::cout << "Помилка: примірник недоступний" << std::endl;
        return "Недоступний";
    }

    Date expectedReturn = date;
    expectedReturn.day = expectedReturn.day + 21;

    Transaction newTransaction(0, currentReader.getId(), currentCopy.getInventoryNumber(), date,
                              expectedReturn);
    bool saved = transactionRegistry.recordTransaction(newTransaction);
    if (saved) {
        currentCopy.setStatus("виданий");
        std::cout << "Книгу видано" << std::endl;
        return "OK";
    }
    return "Помилка";
}
