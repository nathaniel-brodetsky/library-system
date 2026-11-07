#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "library/book.hpp"
#include "library/reader.hpp"
#include "library/transaction.hpp"

namespace library {

class ReaderRegistry {
public:
    bool add(Reader reader);

    [[nodiscard]] const Reader* find(ReaderId id) const;
    [[nodiscard]] bool contains(ReaderId id) const;
    [[nodiscard]] std::size_t size() const noexcept { return readers_.size(); }

private:
    std::unordered_map<ReaderId, Reader> readers_;
};

class BookCatalog {
public:
    bool add(Book book);

    [[nodiscard]] const Book* find(std::string_view isbn) const;
    [[nodiscard]] bool contains(std::string_view isbn) const;
    [[nodiscard]] std::size_t size() const noexcept { return books_.size(); }

private:
    std::map<std::string, Book, std::less<>> books_;
};

class CopyRegistry {
public:
    bool add(BookCopy copy);

    [[nodiscard]] BookCopy* find(CopyId inventoryNumber);
    [[nodiscard]] const BookCopy* find(CopyId inventoryNumber) const;
    [[nodiscard]] std::size_t size() const noexcept { return copies_.size(); }

private:
    std::unordered_map<CopyId, BookCopy> copies_;
};

class TransactionRegistry {
public:
    explicit TransactionRegistry(TransactionId firstId = 1) noexcept : nextId_(firstId) {}

    TransactionId open(ReaderId readerId, CopyId copyId, Date issueDate, Date dueDate);
    void refreshOverdue(Date currentDate);

    [[nodiscard]] Transaction* findOpen(ReaderId readerId, CopyId copyId);
    [[nodiscard]] const Transaction* find(TransactionId id) const;
    [[nodiscard]] std::vector<Transaction> openFor(ReaderId readerId) const;
    [[nodiscard]] std::size_t countOpen(ReaderId readerId) const;
    [[nodiscard]] bool hasOverdue(ReaderId readerId, Date currentDate) const;
    [[nodiscard]] std::size_t size() const noexcept { return transactions_.size(); }

private:
    std::vector<Transaction> transactions_;
    TransactionId nextId_;
};

}  // namespace library
