#pragma once

#include <cstddef>
#include <string_view>

#include "library/book.hpp"
#include "library/identifiers.hpp"

namespace library {

enum class IsbnValidity { Valid, WrongLength, InvalidCharacter, BadChecksum };

enum class CatalogResult {
    Added,
    InvalidIsbn,
    EmptyTitle,
    TitleTooLong,
    EmptyAuthor,
    YearTooEarly,
    YearInFuture,
    InvalidInventoryNumber,
    UnknownBook,
    Duplicate,
};

class BookValidator {
public:
    static constexpr std::size_t kIsbnDigits = 13;
    static constexpr std::size_t kMaxTitleLength = 200;
    static constexpr int kMinPublicationYear = 1450;
    static constexpr CopyId kMinInventoryNumber = 1;
    static constexpr CopyId kMaxInventoryNumber = 999'999;

    [[nodiscard]] static IsbnValidity checkIsbn(std::string_view isbn) noexcept;
    [[nodiscard]] static std::size_t countCharacters(std::string_view utf8) noexcept;
    [[nodiscard]] static CatalogResult check(const Book& book, int currentYear) noexcept;

    [[nodiscard]] static constexpr bool isValidInventoryNumber(CopyId inventoryNumber) noexcept {
        return inventoryNumber >= kMinInventoryNumber && inventoryNumber <= kMaxInventoryNumber;
    }
};

}  // namespace library
