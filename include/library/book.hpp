#pragma once

#include <string>
#include <utility>

#include "library/identifiers.hpp"
#include "library/statuses.hpp"

namespace library {

class Book {
public:
    Book(std::string isbn, std::string title, std::string author, int publicationYear)
        : isbn_(std::move(isbn)), title_(std::move(title)), author_(std::move(author)), publicationYear_(publicationYear) {}

    [[nodiscard]] const std::string& isbn() const noexcept { return isbn_; }
    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    [[nodiscard]] const std::string& author() const noexcept { return author_; }
    [[nodiscard]] int publicationYear() const noexcept { return publicationYear_; }

private:
    std::string isbn_;
    std::string title_;
    std::string author_;
    int publicationYear_;
};

class BookCopy {
public:
    BookCopy(CopyId inventoryNumber, std::string isbn)
        : inventoryNumber_(inventoryNumber), isbn_(std::move(isbn)) {}

    [[nodiscard]] CopyId inventoryNumber() const noexcept { return inventoryNumber_; }
    [[nodiscard]] const std::string& isbn() const noexcept { return isbn_; }
    [[nodiscard]] BookStatus status() const noexcept { return status_; }
    [[nodiscard]] bool isAvailable() const noexcept { return status_ == BookStatus::Available; }

    void setStatus(BookStatus status) noexcept { status_ = status; }

private:
    CopyId inventoryNumber_;
    std::string isbn_;
    BookStatus status_{BookStatus::Available};
};

}  // namespace library
