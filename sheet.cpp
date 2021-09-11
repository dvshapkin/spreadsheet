#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException{"InvalidPosition"};
    }
    Increase(pos);
    auto &cell_interface = cells_[pos.row][pos.col];
    if (cell_interface == nullptr) {
        cell_interface = std::make_unique<Cell>(*this);
    }
    Cell *cell = dynamic_cast<Cell *>(cell_interface.get());
    cell->Set(std::move(text));
}

const CellInterface *Sheet::GetCell(Position pos) const {
    return const_cast<Sheet *>(this)->GetCell(pos);
}

CellInterface *Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException{"InvalidPosition"};
    }
    if (!IsPrintable(pos)) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException{"InvalidPosition"};
    }
    if (!IsPrintable(pos)) {
        return;
    }
    cells_[pos.row][pos.col].reset();
    Decrease(pos);
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream &output) const {
    auto printer = [](const std::unique_ptr<CellInterface> &cell, std::ostream &output) {
        std::visit([&output](auto &&arg) { output << arg; }, cell->GetValue());
    };
    PrintTable(printer, output);
}

void Sheet::PrintTexts(std::ostream &output) const {
    auto printer = [](const std::unique_ptr<CellInterface> &cell, std::ostream &output) {
        output << cell->GetText();
    };
    PrintTable(printer, output);
}

bool Sheet::IsPrintable(Position pos) const {
    if (SizeOf(cells_) <= pos.row) {
        return false;
    }
    if (SizeOf(cells_[pos.row]) <= pos.col) {
        return false;
    }
    return true;
}

void Sheet::Increase(Position pos) {
    if (size_.rows <= pos.row) {
        size_.rows = pos.row + 1;
        cells_.resize(size_.rows);
    }
    if (SizeOf(cells_[pos.row]) <= pos.col) {
        cells_[pos.row].resize(pos.col + 1);
    }
    if (size_.cols <= pos.col) {
        size_.cols = pos.col + 1;
    }
}

void Sheet::Decrease(Position pos) {

    // Удалим лишние пустые ячейки в строке
    int curr_size = SizeOf(cells_[pos.row]);
    if (curr_size == pos.col + 1) {
        int new_size = curr_size;
        while (new_size != 0 && cells_[pos.row][new_size - 1] == nullptr) {
            --new_size;
        }
        cells_[pos.row].resize(new_size);
    }

    // Удалим лишние пустые строки
    if (cells_[pos.row].empty() && size_.rows == pos.row + 1) {
        int new_size = size_.rows;
        while (new_size != 0 && cells_[new_size - 1].empty()) {
            --new_size;
        }
        size_.rows = new_size;  // обновим количество строк в печатной области
        cells_.resize(size_.rows);
    }

    // Обновим количество колонок в печатной области
    if (size_.cols == pos.col + 1) {
        int max_size = 0;
        for (const auto &row: cells_) {
            if (max_size < SizeOf(row)) {
                max_size = SizeOf(row);
            }
        }
        size_.cols = max_size;
    }
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
