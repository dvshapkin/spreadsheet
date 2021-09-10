#include "cell.h"


#include <stack>
#include <string>
#include <optional>

// ---------- Cell::Impl ----------



// ---------- Cell methods ----------

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
        //auto refs = copy_impl->GetReferencedCells();
        //CheckForCircularDependencies(refs);
        std::unordered_set<const Cell *> checked_refs;
        if (HasCircularDependency(this, checked_refs)) {
            throw CircularDependencyException{"Circular dependency"};
        }
    }

    std::unique_ptr<Impl> formula_impl = std::move(copy_impl);

    ClearDependencies();

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        impl_ = std::move(formula_impl);
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

    ClearCacheOfDependentCells();
    RebuildGraph();
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
    std::stack<CellInterface *> stack_positions = CreateStack(deps_);
    std::unordered_set<CellInterface *> visited_cells;

    while (!stack_positions.empty()) {
        CellInterface *now_cell = stack_positions.top();
        stack_positions.pop();
        auto is_visited = visited_cells.find(now_cell);
        if (is_visited != visited_cells.end()) {
            continue;
        }
        visited_cells.insert(now_cell);
        Cell *real_cell = dynamic_cast<Cell *>(now_cell);
        AddToStack(stack_positions, real_cell->deps_);
        real_cell->ClearCache();
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

std::vector<std::vector<bool>> Cell::CreateVisitedCells() const {
    Size printable_size = sheet_.GetPrintableSize();
    std::vector<bool> cols(printable_size.cols, false);
    std::vector<std::vector<bool>> rows(printable_size.rows, cols);
    return rows;
}

bool Cell::HasCircularDependency(const Cell *p_begin_cell,
                                 std::unordered_set<const Cell *> &checked_refs) const {
    for (Position pos: GetReferencedCells()) {
        auto p_cell = PosToCell(pos);
        if (p_cell) {
            if (checked_refs.count(p_cell)) continue;
            checked_refs.insert(p_cell);
            if (p_begin_cell == p_cell
                || p_cell->HasCircularDependency(p_begin_cell, checked_refs)) {
                return true;
            }
        }
    }
    return false;
}

//bool Cell::HasCircularDependency(std::unordered_set<Position> &checked_refs) const {
//    for (Position pos: GetReferencedCells()) {
//        if (checked_refs.count(pos)) continue;
//        checked_refs.insert(pos);
//        Cell* p_cell = PosToCell(pos);
//        if (p_cell == this) {
//            //throw CircularDependencyException{"Circular dependency"};
//            return true;
//        }
//        if (!p_cell) {
//            sheet_.SetCell(pos, "");
//            p_cell = dynamic_cast<Cell *>(sheet_.GetCell(pos));
//        }
//        if (p_cell->HasCircularDependency(checked_refs)) {
//            return true;
//        }
//    }
//    return false;
//}

//void Cell::CheckForCircularDependencies(const std::vector<Position>& refs) const {
//    std::stack<Position> stack_positions = CreateStack(refs);
//    std::vector<std::vector<bool>> visited_cells = CreateVisitedCells();
//
//    while (!stack_positions.empty()) {
//        Position pos = stack_positions.top();
//        stack_positions.pop();
//        if (visited_cells[pos.row][pos.col]) {
//            continue;
//        }
//        visited_cells[pos.row][pos.col] = true;
//        const CellInterface* current_cell = sheet_.GetCell(pos);
//        if (current_cell == this) {
//            throw CircularDependencyException{"Circular dependency"};
//        }
//        if (current_cell == nullptr) {
//            sheet_.SetCell(pos, "");
//            current_cell = sheet_.GetCell(pos);
//        }
//        AddToStack(stack_positions, current_cell->GetReferencedCells());
//    }
//}

void Cell::RebuildGraph() {
    for (Position pos: impl_->GetReferencedCells()) {
        Cell *cell = PosToCell(pos);
        if (cell == nullptr) {
            auto &dirty_sheet = const_cast<Sheet &>(sheet_);
            dirty_sheet.SetCell(pos, {});
            cell = PosToCell(pos);
        }
        cell->deps_.insert(this);
    }
}

void Cell::AddToStack(std::stack<Position> &destination, const std::vector<Position> &source) {
    for (Position pos: source) {
        destination.push(pos);
    }
}

void Cell::AddToStack(std::stack<CellInterface *> &destination, const std::unordered_set<CellInterface *> &source) {
    for (CellInterface *cell: source) {
        destination.push(cell);
    }
}

std::stack<Position> Cell::CreateStack(const std::vector<Position> &referenced_cells) {
    std::stack<Position> stack_positions;
    AddToStack(stack_positions, referenced_cells);
    return stack_positions;
}

std::stack<CellInterface *> Cell::CreateStack(const std::unordered_set<CellInterface *> &deps) {
    std::stack<CellInterface *> stack_positions;
    AddToStack(stack_positions, deps);
    return stack_positions;
}

Cell *Cell::PosToCell(Position pos) const {
    const CellInterface *p_cell = sheet_.GetCell(pos);
    if (p_cell) {
        return dynamic_cast<Cell *>(const_cast<CellInterface *>(p_cell));
    }
    return nullptr;
}


// ---------- Cell::impl methods ----------

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