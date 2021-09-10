#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>
#include <cmath>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

const int ASCII_A = 0x41;

// Реализуйте методы:
bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    if (row == rhs.row) {
        return col < rhs.col;
    }
    return row < rhs.row;
}

bool Position::IsValid() const {
    return row >= 0 && row < MAX_ROWS
           && col >= 0 && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) return {};

    std::string Col;
    auto col_copy = col;
    do {
        int ascii = col_copy % LETTERS + ASCII_A;
        Col.push_back(static_cast<char>(ascii));
        col_copy /= LETTERS;
    } while (col_copy-- != 0);
    std::reverse(Col.begin(), Col.end());

    return Col + std::to_string(row + 1);
}

Position Position::FromString(std::string_view str) {
    if (str.empty()) return Position::NONE;

    // Разделим пользовательский индекс ячейки (str) на две части:
    // колонку (col_part) и строку (row_part)
    std::string col_part, row_part;
    bool numbers_have_already_started = false;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (isupper(c) && i < MAX_POS_LETTER_COUNT && !numbers_have_already_started) {
            col_part += c;
        } else if (isdigit(c)) {
            row_part += c;
            numbers_have_already_started = true;
        } else {
            return Position::NONE;
        }
    }
    if (row_part.empty() || col_part.empty()) return Position::NONE;

    // Преобразуем row_part в число
    auto row_as_num = std::stoll(row_part);
    if (row_as_num > MAX_ROWS) return Position::NONE;

    // Преобразуем col_part в число
    int col_as_num = 0;
    for (size_t i = 0; i < col_part.size(); ++i) {
        char c = col_part.at(col_part.size() - i - 1);
        auto digit = c - ASCII_A + 1;
        col_as_num += digit * static_cast<int>(std::pow(LETTERS, i));
    }

    Position pos{
            static_cast<int>(row_as_num) - 1,
            col_as_num - 1
    };

    return pos.IsValid() ? pos : Position::NONE;
}


bool Size::operator==(Size rhs) const {
    return rows == rhs.rows
           && cols == rhs.cols;
}
