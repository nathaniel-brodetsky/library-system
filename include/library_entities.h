#pragma once

#include <string>
#include <utility>

enum class BookStatus { Available, Borrowed, Maintenance };

enum class TransactionStatus { Active, Closed, Overdue };

struct Date {
    int year;
    int month;
    int day;
};

inline int toDays(const Date& date) {
    return date.year * 365 + date.month * 30 + date.day;
}

class Reader {
public:
    Reader(int id, std::string fullName, std::string email)
        : id_(id), fullName_(std::move(fullName)), email_(std::move(email)) {}

    int getId() const { return id_; }
    const std::string& getFullName() const { return fullName_; }
    const std::string& getEmail() const { return email_; }

private:
    int id_;
    std::string fullName_;
    std::string email_;
};

class Book {
public:
    Book(std::string isbn, std::string title, std::string author)
        : isbn_(std::move(isbn)), title_(std::move(title)), author_(std::move(author)) {}

    const std::string& getIsbn() const { return isbn_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getAuthor() const { return author_; }

private:
    std::string isbn_;
    std::string title_;
    std::string author_;
};

class BookCopy {
public:
    explicit BookCopy(int inventoryNumber) : inventoryNumber_(inventoryNumber) {}

    int getInventoryNumber() const { return inventoryNumber_; }
    BookStatus getStatus() const { return status_; }
    void setStatus(BookStatus status) { status_ = status; }

private:
    int inventoryNumber_;
    BookStatus status_{BookStatus::Available};
};

class Transaction {
public:
    Transaction(int id, int readerId, int inventoryNumber, const Date& issueDate, const Date& dueDate)
        : id_(id),
          readerId_(readerId),
          inventoryNumber_(inventoryNumber),
          issueDate_(issueDate),
          dueDate_(dueDate),
          status_(TransactionStatus::Active) {}

    int getId() const { return id_; }
    void assignId(int id) { id_ = id; }
    int getReaderId() const { return readerId_; }
    int getInventoryNumber() const { return inventoryNumber_; }
    const Date& getIssueDate() const { return issueDate_; }
    const Date& getDueDate() const { return dueDate_; }
    TransactionStatus getStatus() const { return status_; }

private:
    int id_;
    int readerId_;
    int inventoryNumber_;
    Date issueDate_;
    Date dueDate_;
    TransactionStatus status_;
};
