#pragma once

#include <optional>

#include "library/date.hpp"
#include "library/identifiers.hpp"
#include "library/statuses.hpp"

namespace library {

class Transaction {
public:
    Transaction(TransactionId id, ReaderId readerId, CopyId copyId, Date issueDate, Date dueDate) noexcept
        : id_(id), readerId_(readerId), copyId_(copyId), issueDate_(issueDate), dueDate_(dueDate) {}

    [[nodiscard]] TransactionId id() const noexcept { return id_; }
    [[nodiscard]] ReaderId readerId() const noexcept { return readerId_; }
    [[nodiscard]] CopyId copyId() const noexcept { return copyId_; }
    [[nodiscard]] Date issueDate() const noexcept { return issueDate_; }
    [[nodiscard]] Date dueDate() const noexcept { return dueDate_; }
    [[nodiscard]] std::optional<Date> returnDate() const noexcept { return returnDate_; }
    [[nodiscard]] TransactionStatus status() const noexcept { return status_; }

    [[nodiscard]] bool isOpen() const noexcept { return status_ != TransactionStatus::Closed; }
    [[nodiscard]] bool isOverdueOn(Date date) const noexcept { return isOpen() && date > dueDate_; }

    void markOverdue() noexcept {
        if (status_ == TransactionStatus::Active) {
            status_ = TransactionStatus::Overdue;
        }
    }

    void close(Date returnDate) noexcept {
        status_ = TransactionStatus::Closed;
        returnDate_ = returnDate;
    }

private:
    TransactionId id_;
    ReaderId readerId_;
    CopyId copyId_;
    Date issueDate_;
    Date dueDate_;
    std::optional<Date> returnDate_;
    TransactionStatus status_{TransactionStatus::Active};
};

}  // namespace library
