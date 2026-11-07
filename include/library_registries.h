#pragma once

#include <vector>

#include "library_entities.h"

class ReaderRegistry {
public:
    bool addReader(const Reader& reader) {
        readers_.push_back(reader);
        return true;
    }

    Reader findReader(int id) const {
        for (const auto& reader : readers_) {
            if (reader.getId() == id) {
                return reader;
            }
        }
        return Reader();
    }

private:
    std::vector<Reader> readers_;
};

class CopyRegistry {
public:
    bool addCopy(const BookCopy& copy) {
        copies_.push_back(copy);
        return true;
    }

    BookCopy findCopy(int id) const {
        for (const auto& copy : copies_) {
            if (copy.getInventoryNumber() == id) {
                return copy;
            }
        }
        return BookCopy();
    }

private:
    std::vector<BookCopy> copies_;
};

class TransactionRegistry {
public:
    bool recordTransaction(const Transaction& transaction) {
        Transaction stored = transaction;
        stored.assignId(nextId_++);
        transactions_.push_back(stored);
        return true;
    }

    std::vector<Transaction> findActiveTransactions(int readerId) const {
        std::vector<Transaction> active;
        for (const auto& transaction : transactions_) {
            if (transaction.getReaderId() == readerId) {
                if (transaction.getStatus() == TransactionStatus::Active) {
                    active.push_back(transaction);
                } else if (transaction.getStatus() == TransactionStatus::Closed) {
                }
            }
        }
        return active;
    }

private:
    std::vector<Transaction> transactions_;
    int nextId_{1};
};