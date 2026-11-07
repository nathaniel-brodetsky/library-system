#include "library/registries.hpp"

#include <algorithm>
#include <utility>

namespace library {

bool ReaderRegistry::add(Reader reader) {
    const ReaderId id = reader.id();
    return readers_.try_emplace(id, std::move(reader)).second;
}

const Reader* ReaderRegistry::find(ReaderId id) const {
    const auto it = readers_.find(id);
    return it == readers_.end() ? nullptr : &it->second;
}

bool ReaderRegistry::contains(ReaderId id) const {
    return readers_.contains(id);
}

bool BookCatalog::add(Book book) {
    std::string isbn = book.isbn();
    return books_.try_emplace(std::move(isbn), std::move(book)).second;
}

const Book* BookCatalog::find(std::string_view isbn) const {
    const auto it = books_.find(isbn);
    return it == books_.end() ? nullptr : &it->second;
}

bool BookCatalog::contains(std::string_view isbn) const {
    return books_.find(isbn) != books_.end();
}

bool CopyRegistry::add(BookCopy copy) {
    const CopyId inventoryNumber = copy.inventoryNumber();
    return copies_.try_emplace(inventoryNumber, std::move(copy)).second;
}

BookCopy* CopyRegistry::find(CopyId inventoryNumber) {
    const auto it = copies_.find(inventoryNumber);
    return it == copies_.end() ? nullptr : &it->second;
}

const BookCopy* CopyRegistry::find(CopyId inventoryNumber) const {
    const auto it = copies_.find(inventoryNumber);
    return it == copies_.end() ? nullptr : &it->second;
}

TransactionId TransactionRegistry::open(ReaderId readerId, CopyId copyId, Date issueDate, Date dueDate) {
    const TransactionId id = nextId_++;
    transactions_.emplace_back(id, readerId, copyId, issueDate, dueDate);
    return id;
}

void TransactionRegistry::refreshOverdue(Date currentDate) {
    for (auto& transaction : transactions_) {
        if (transaction.isOverdueOn(currentDate)) {
            transaction.markOverdue();
        }
    }
}

Transaction* TransactionRegistry::findOpen(ReaderId readerId, CopyId copyId) {
    const auto it = std::find_if(transactions_.begin(), transactions_.end(), [&](const Transaction& transaction) {
        return transaction.isOpen() && transaction.readerId() == readerId && transaction.copyId() == copyId;
    });
    return it == transactions_.end() ? nullptr : &*it;
}

const Transaction* TransactionRegistry::find(TransactionId id) const {
    const auto it = std::find_if(transactions_.begin(), transactions_.end(),
                                 [id](const Transaction& transaction) { return transaction.id() == id; });
    return it == transactions_.end() ? nullptr : &*it;
}

std::vector<Transaction> TransactionRegistry::openFor(ReaderId readerId) const {
    std::vector<Transaction> result;
    std::copy_if(transactions_.begin(), transactions_.end(), std::back_inserter(result),
                 [readerId](const Transaction& transaction) {
                     return transaction.isOpen() && transaction.readerId() == readerId;
                 });
    return result;
}

std::size_t TransactionRegistry::countOpen(ReaderId readerId) const {
    return static_cast<std::size_t>(
        std::count_if(transactions_.begin(), transactions_.end(), [readerId](const Transaction& transaction) {
            return transaction.isOpen() && transaction.readerId() == readerId;
        }));
}

bool TransactionRegistry::hasOverdue(ReaderId readerId, Date currentDate) const {
    return std::any_of(transactions_.begin(), transactions_.end(), [&](const Transaction& transaction) {
        return transaction.readerId() == readerId && transaction.isOverdueOn(currentDate);
    });
}

}  // namespace library
