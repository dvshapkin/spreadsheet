#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <deque>
#include <functional>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_set>
#include <optional>

class Sheet;

class Impl;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet &sheet);

    ~Cell() override = default;

    void Set(std::string text);

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    //void CheckForCircularDependencies(const std::vector<Position> &referenced_cells) const;
    bool HasCircularDependency(const Cell *p_begin_cell,
                               std::unordered_set<const Cell *> &checked_refs) const;

    void ClearCache();

    void ClearCacheOfDependentCells();

    void ClearDependencies();

    std::vector<std::vector<bool>> CreateVisitedCells() const;

    void RebuildGraph();

    static void AddToStack(std::stack<Position> &destination, const std::vector<Position> &source);

    static void AddToStack(std::stack<CellInterface *> &destination, const std::unordered_set<CellInterface *> &source);

    static std::stack<Position> CreateStack(const std::vector<Position> &referenced_cells);

    static std::stack<CellInterface *> CreateStack(const std::unordered_set<CellInterface *> &deps);

    Cell *PosToCell(Position pos) const;


    Sheet &sheet_;
    std::unique_ptr<Impl> impl_;
    std::unordered_set<CellInterface *> deps_;
    mutable std::optional<Value> cache_;
};

class Impl {
public:
    virtual ~Impl() = default;

    [[nodiscard]] virtual CellInterface::Value GetValue(const Sheet &sheet) const = 0;

    [[nodiscard]] virtual std::string GetText() const = 0;

    [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const;

private:
    std::vector<Position> empty_vector_;
};

class EmptyImpl : public Impl {
public:
    [[nodiscard]] CellInterface::Value GetValue(const Sheet &sheet) const override;

    [[nodiscard]] std::string GetText() const override;

private:
    std::string empty_text_;
};

class TextImpl : public Impl {
public:
    explicit TextImpl(std::string text);

    [[nodiscard]] CellInterface::Value GetValue(const Sheet &sheet) const override;

    [[nodiscard]] std::string GetText() const override;

private:
    std::string text_;
};

class FormulaImpl : public Impl {
public:
    explicit FormulaImpl(const std::string &text);

    [[nodiscard]] CellInterface::Value GetValue(const Sheet &sheet) const override;

    [[nodiscard]] std::string GetText() const override;

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;

private:
    std::unique_ptr<FormulaInterface> formula_;
};