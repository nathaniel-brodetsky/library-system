#pragma once

#include <string>

enum class BookStatus { Available, Borrowed, Maintenance, Missing };

enum class TransactionStatus { Active, Closed, Overdue };

struct Date {
    int year;
    int month;
    int day;
};

inline int toDays(Date date) {
    return date.year * 365 + date.month * 30 + date.day;
}

class Reader {
public:
    int readerId;
    std::string fullName;
    std::string email;

    Reader() {
        readerId = 0;
    }

    Reader(int id, std::string name, std::string mail) {
        readerId = id;
        fullName = name;
        email = mail;
    }

    int getId() {
        return readerId;
    }
};

class Book {
public:
    std::string ISBN;
    std::string title;
    std::string author;

    Book() {}

    Book(std::string isbnCode, std::string bookTitle, std::string bookAuthor) {
        ISBN = isbnCode;
        title = bookTitle;
        author = bookAuthor;
    }

    std::string getIsbn() {
        return ISBN;
    }
};

class BookCopy {
public:
    int inventoryNumber;
    std::string ISBN;
    BookStatus status;

    BookCopy() {
        inventoryNumber = 0;
        status = BookStatus::Missing;
    }

    BookCopy(int number) {
        inventoryNumber = number;
        status = BookStatus::Available;
    }

    int getInventoryNumber() {
        return inventoryNumber;
    }

    BookStatus getStatus() {
        return status;
    }

    void setStatus(BookStatus newStatus) {
        status = newStatus;
    }
};

class Transaction {
public:
    int transactionId;
    int readerId;
    int inventoryNumber;
    Date issueDate;
    Date dueDate;
    TransactionStatus status;

    Transaction(int id, int reader, int number, Date issued, Date due) {
        transactionId = id;
        readerId = reader;
        inventoryNumber = number;
        issueDate = issued;
        dueDate = due;
        status = TransactionStatus::Active;
    }

    int getInventoryNumber() {
        return inventoryNumber;
    }
};