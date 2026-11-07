#include <iostream>

#include "library_controller.h"

int main() {
    LibraryController controller;
    controller.readerRegistry.addReader(Reader(1, "Шевченко Тарас", "taras@example.com"));
    controller.copyRegistry.addCopy(BookCopy(101));

    Date today = {2025, 3, 10};
    std::cout << controller.issueBook(1, 101, today) << std::endl;
    std::cout << controller.issueBook(1, 101, today) << std::endl;
    std::cout << controller.issueBook(7, 101, today) << std::endl;
    return 0;
}