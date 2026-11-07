#pragma once

#include <string>

#include "library_registries.h"

class LibraryController {
public:
    ReaderRegistry readerRegistry;
    CopyRegistry copyRegistry;
    TransactionRegistry transactionRegistry;

    std::string issueBook(int readerId, int inventoryNumber, Date date);
};