#pragma once

#include <vector>

#include "library_entities.h"

class ReaderRegistry {
public:
    std::vector<Reader> readers;

    bool addReader(Reader reader) {
        readers.push_back(reader);
        return true;
    }

    Reader findReader(int id) {
        for (int i = 0; i < (int)readers.size(); i++) {
            if (readers[i].getId() == id) {
                return readers[i];
            }
        }
        return Reader();
    }
};

class CopyRegistry {
public:
    std::vector<BookCopy> copies;

    bool addCopy(BookCopy copy) {
        copies.push_back(copy);
        return true;
    }

    BookCopy findCopy(int id) {
        for (int i = 0; i < (int)copies.size(); i++) {
            if (copies[i].getInventoryNumber() == id) {
                return copies[i];
            }
        }
        return BookCopy();
    }
};

class TransactionRegistry {
public:
    std::vector<Transaction> transactions;
    int nextId = 1;

    bool recordTransaction(Transaction tx) {
        Transaction newTx = tx;
        newTx.transactionId = nextId++;
        transactions.push_back(newTx);
        return true;
    }

    std::vector<Transaction> findActiveTransactions(int readerId) {
        std::vector<Transaction> active;
        for (int i = 0; i < (int)transactions.size(); i++) {
            if (transactions[i].readerId == readerId) {
                if (transactions[i].status == TransactionStatus::Active) {
                    active.push_back(transactions[i]);
                } else if (transactions[i].status == TransactionStatus::Closed) {
                }
            }
        }
        return active;
    }
};