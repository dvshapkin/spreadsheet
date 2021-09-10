#include "formula.h"

#include "FormulaAST.h"

#include <functional>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category)
        : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    static std::string REF = "#REF!"s;
    static std::string VALUE = "#VALUE!"s;
    static std::string DIV0 = "#DIV/0!"s;

    switch (category_) {
        case FormulaError::Category::Ref:
            return REF;
        case FormulaError::Category::Value:
            return VALUE;
        case FormulaError::Category::Div0:
            return DIV0;
        default:
            return "";
    }
}

std::ostream &operator<<(std::ostream &output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:

        explicit Formula(const std::string &expression)
                : ast_(ParseFormulaAST(expression)) {
        }

        [[nodiscard]] Value Evaluate(const SheetInterface &sheet) const override {
            using namespace std;
            auto accessor = [&sheet](Position pos) -> double {
                auto value = sheet.GetCell(pos)->GetValue();
                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                } else if (std::holds_alternative<std::string>(value)) {
                    std::string str_value = std::get<std::string>(value);
                    if (str_value.empty()) {
                        return 0;
                    }
                    try {
                        return std::stod(str_value);
                    } catch (std::exception &) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                } else {
                    throw std::get<FormulaError>(value);
                }
            };
            try {
                return ast_.Execute(accessor);
            } catch (FormulaError &fe) {
                return fe;
            }
        }

        [[nodiscard]] std::string GetExpression() const override {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }

        [[nodiscard]] std::vector<Position> GetReferencedCells() const override {
            auto cells = ast_.GetCells();
            return std::vector<Position>(cells.cbegin(), cells.cend());
        }

    private:
        FormulaAST ast_;
    };

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(const std::string &expression) {
    try {
        return std::make_unique<Formula>(expression);
    } catch (FormulaException &fe) {
        throw fe;
    }
}
