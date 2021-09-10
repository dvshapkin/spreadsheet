#include "cell.h"

#include <string>
#include <optional>

Cell::Cell(Sheet &sheet)
        : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {
}

void Cell::Set(std::string text) {

    if (text == impl_->GetText()) {
        return;
    }

    std::unique_ptr<Impl> copy_impl;

    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        try {
            copy_impl = std::make_unique<FormulaImpl>(text.substr(1));
        } catch (std::exception &) {
            throw FormulaException("Formula error");
        }

        auto refs = copy_impl->GetReferencedCells();
        std::unordered_set<const Cell *> checked_refs;
        if (HasCircularDependency(this, refs, checked_refs)) {
            throw CircularDependencyException{"Circular dependency"};
        }
    }

    // Удалим старые зависимости
    ClearDependencies();

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        impl_ = std::move(copy_impl);
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

    // Добавим новые зависимости
    for (Position ref: impl_->GetReferencedCells()) {
        auto p_cell = PosToCell(ref);
        if (!p_cell) {
            sheet_.SetCell(ref, "");
            p_cell = PosToCell(ref);
        }
        p_cell->deps_.insert(this);
    }

    ClearCacheOfDependentCells();
}

Cell::Value Cell::GetValue() const {
    if (cache_ == std::nullopt) {
        cache_ = impl_->GetValue(sheet_);
    }
    return cache_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !GetReferencedCells().empty();
}

void Cell::ClearCache() {
    cache_.reset();
}

void Cell::ClearCacheOfDependentCells() {
    for (Cell *dep: deps_) {
        if (dep && dep->cache_.has_value()) {
            dep->ClearCacheOfDependentCells();
            dep->ClearCache();
        }
    }
    ClearCache();
}

void Cell::ClearDependencies() {
    for (const Position &pos: impl_->GetReferencedCells()) {
        auto *p_cell = PosToCell(pos);
        if (p_cell) {
            p_cell->deps_.erase(this);
        }
    }
}

bool Cell::HasCircularDependency(const Cell *p_begin_cell, const std::vector<Position> &refs,
                                 std::unordered_set<const Cell *> &checked_refs) const {
    for (Position pos: refs) {
        auto p_cell = PosToCell(pos);
        if (p_cell) {
            if (checked_refs.count(p_cell)) continue;
            if (p_begin_cell == p_cell
                || p_cell->HasCircularDependency(p_begin_cell, p_cell->GetReferencedCells(), checked_refs)) {
                return true;
            }
            checked_refs.insert(p_cell);
        }
    }
    return false;
}

Cell *Cell::PosToCell(Position pos) const {
    const CellInterface *p_cell = sheet_.GetCell(pos);
    if (p_cell) {
        return dynamic_cast<Cell *>(const_cast<CellInterface *>(p_cell));
    }
    return nullptr;
}

std::vector<Position> Impl::GetReferencedCells() const {
    return empty_vector_;
}

CellInterface::Value EmptyImpl::GetValue(const Sheet &sheet) const {
    return empty_text_;
}

std::string EmptyImpl::GetText() const {
    return empty_text_;
}

TextImpl::TextImpl(std::string text)
        : text_(std::move(text)) {
}

CellInterface::Value TextImpl::GetValue(const Sheet &sheet) const {
    if (!text_.empty() && text_.front() == ESCAPE_SIGN) {
        return text_.substr(1);
    } else {
        return text_;
    }
}

std::string TextImpl::GetText() const {
    return text_;
}

FormulaImpl::FormulaImpl(const std::string &text)
        : formula_(ParseFormula(text)) {
}

CellInterface::Value FormulaImpl::GetValue(const Sheet &sheet) const {
    auto formula_evaluate = formula_->Evaluate(sheet);
    if (std::holds_alternative<double>(formula_evaluate)) {
        return std::get<double>(formula_evaluate);
    } else {
        return std::get<FormulaError>(formula_evaluate);
    }
}

std::string FormulaImpl::GetText() const {
    return '=' + formula_->GetExpression();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}
