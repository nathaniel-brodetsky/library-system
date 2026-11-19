#include "library/book_validator.hpp"

#include "library/reader_validator.hpp"

namespace library {

IsbnValidity BookValidator::checkIsbn(std::string_view isbn) noexcept {
    std::size_t digits = 0;
    int checksum = 0;
    for (const char symbol : isbn) {
        if (symbol == '-') {
            continue;
        }
        if (symbol < '0' || symbol > '9') {
            return IsbnValidity::InvalidCharacter;
        }
        const int weight = digits % 2 == 0 ? 1 : 3;
        checksum += weight * (symbol - '0');
        ++digits;
    }
    if (digits != kIsbnDigits) {
        return IsbnValidity::WrongLength;
    }
    return checksum % 10 == 0 ? IsbnValidity::Valid : IsbnValidity::BadChecksum;
}

std::size_t BookValidator::countCharacters(std::string_view utf8) noexcept {
    std::size_t count = 0;
    for (const char symbol : utf8) {
        if ((static_cast<unsigned char>(symbol) & 0xC0U) != 0x80U) {
            ++count;
        }
    }
    return count;
}

CatalogResult BookValidator::check(const Book& book, int currentYear) noexcept {
    if (checkIsbn(book.isbn()) != IsbnValidity::Valid) {
        return CatalogResult::InvalidIsbn;
    }
    if (ReaderValidator::isBlank(book.title())) {
        return CatalogResult::EmptyTitle;
    }
    if (countCharacters(book.title()) > kMaxTitleLength) {
        return CatalogResult::TitleTooLong;
    }
    if (ReaderValidator::isBlank(book.author())) {
        return CatalogResult::EmptyAuthor;
    }
    if (book.publicationYear() < kMinPublicationYear) {
        return CatalogResult::YearTooEarly;
    }
    if (book.publicationYear() > currentYear) {
        return CatalogResult::YearInFuture;
    }
    return CatalogResult::Added;
}

}  // namespace library
