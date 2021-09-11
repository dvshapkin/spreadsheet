#pragma once

#include "cell.h"
#include "common.h"

#include <iostream>
#include <functional>

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, std::string text) override;

    [[nodiscard]] const CellInterface *GetCell(Position pos) const override;

    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    [[nodiscard]] Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;

    void PrintTexts(std::ostream &output) const override;

private:
    [[nodiscard]] bool IsPrintable(Position pos) const;

    void Increase(Position pos);

    void Decrease(Position pos);

    template<typename Printer>
    void PrintTable(Printer printer, std::ostream &output) const;

    std::vector<std::vector<std::unique_ptr<CellInterface>>> cells_;
    Size size_;
};

template<typename Container>
int SizeOf(const Container &c) {
    return static_cast<int>(c.size());
}

template<typename Printer>
void Sheet::PrintTable(Printer printer, std::ostream &output) const {
    for (const auto &rows: cells_) {
        int col = 0;
        int min_cols = std::min(SizeOf(rows), size_.cols);
        for (; col < min_cols; ++col) {
            if (rows[col]) {
                printer(rows[col], output);
            }
            if (col < size_.cols - 1) {
                output << '\t';
            }
        }
        for (; col < size_.cols - 1; ++col) {
            output << '\t';
        }
        output << '\n';
    }
}
