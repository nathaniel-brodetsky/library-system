#pragma once

#include <string>

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
    std::string status;

    BookCopy() {
        inventoryNumber = 0;
        status = "відсутній";
    }

    BookCopy(int number) {
        inventoryNumber = number;
        status = "доступний";
    }

    int getInventoryNumber() {
        return inventoryNumber;
    }

    std::string getStatus() {
        return status;
    }

    void setStatus(std::string newStatus) {
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
    std::string status;

    Transaction(int id, int reader, int number, Date issued, Date due) {
        transactionId = id;
        readerId = reader;
        inventoryNumber = number;
        issueDate = issued;
        dueDate = due;
        status = "активна";
    }

    int getInventoryNumber() {
        return inventoryNumber;
    }
};