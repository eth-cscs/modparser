#pragma once

#include <string>
#include <vector>

#include "expression.h"
#include "lexer.h"
#include "util.h"
#include "identifier.h"

class Expression {
public:
    explicit Expression(Location location)
        : location_(location)
    {}

    virtual ~Expression() {};

    // This printer should be implemented with a visitor pattern
    // expressions must provide a method for stringification
    virtual std::string to_string() const = 0;

    Location const& location() const {return location_;};
protected:
    Location location_;
};

// an identifier
class IdentifierExpression : public Expression {
public:
    IdentifierExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {
    }

    std::string const& name() const {
        return name_;
    }

    std::string to_string() const override {
        return colorize(pprintf("%", name_), kYellow);
    }

    ~IdentifierExpression() {}
protected:
    // there has to be some pointer to a table of identifiers
    std::string name_;
};

// an identifier for a derivative
class DerivativeExpression : public IdentifierExpression {
public:
    DerivativeExpression(Location loc, std::string const& name)
        : IdentifierExpression(loc, name)
    {
    }

    std::string to_string() const override {
        return colorize("diff",kBlue) + "(" + colorize(name(), kYellow) + ")";
    }

    ~DerivativeExpression() {}
};


// a number
class NumberExpression : public Expression {
public:
    NumberExpression(Location loc, std::string const& value)
        : Expression(loc), value_(std::stod(value))
    {}

    double value() const {return value_;};

    std::string to_string() const override {
        return colorize(pprintf("%", value_), kPurple);
    }

    ~NumberExpression() {}
private:
    double value_;
};

// declaration of a LOCAL variable
class LocalExpression : public Expression {
public:
    LocalExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {
    }

    std::string to_string() const override {
        return colorize("local", kBlue) + " " + colorize(name_,kYellow);
    }

    ~LocalExpression() {}
private:
    // there has to be some pointer to a table of identifiers
    std::string name_;
};

// a proceduce prototype
class PrototypeExpression : public Expression {
public:
    PrototypeExpression(Location loc, std::string const& name, std::vector<Expression*> const& args)
        : Expression(loc), name_(name), args_(args)
    {
        //std::cout << colorize("PrototypeExpression", kGreen) << std::endl;
    }

    std::string const& name() const {return name_;}

    std::string to_string() const override {
        return name_ + pprintf("(% args : %)", args_.size(), args_);
    }

    ~PrototypeExpression() {
        //std::cout << colorize("~PrototypeExpression", kYellow) << std::endl;
    }
private:
    std::string name_;
    std::vector<Expression*> args_;
};

class CallExpression : public Expression {
public:
    CallExpression(Location loc, std::string const& name, std::vector<Expression*>const &args)
        : Expression(loc), name_(name), args_(args)
    {}

    std::vector<Expression*> const& args() {
        return args_;
    }
    std::string const& name() const {
        return name_;
    }

    std::string to_string() const override {
        std::string str = colorize("call", kBlue) + " " + colorize(name_, kYellow) + " (";
        for(auto arg : args_)
            str += arg->to_string() + ", ";
        str += ")";

        return str;
    }

private:
    std::string name_;
    std::vector<Expression *> args_;
};

class ProcedureExpression : public Expression {
public:
    ProcedureExpression( Location loc,
                         std::string const& name,
                         std::vector<Expression*> const& args,
                         std::vector<Expression*>const & body)
        : Expression(loc), name_(name), args_(args), body_(body)
    {}

    std::vector<Expression*> const& args() {
        return args_;
    }
    std::vector<Expression*> const& body() {
        return body_;
    }
    std::string const& name() const {
        return name_;
    }

    std::string to_string() const override {
        std::string str = colorize("procedure", kBlue) + " " + colorize(name_, kYellow) + "\n";
        str += colorize("  args",kBlue) + " : ";
        for(auto arg : args_)
            str += arg->to_string() + " ";
        str += "\n  "+colorize("body", kBlue)+" :";
        for(auto ex : body_)
            str += "\n    " + ex->to_string();

        return str;
    }

private:
    std::string name_;
    std::vector<Expression *> args_;
    std::vector<Expression *> body_;
};

////////////////////////////////////////////////////////////
// unary expressions
////////////////////////////////////////////////////////////

/// Unary expression
class UnaryExpression : public Expression {
protected:
    Expression *e_;
    TOK op_;
public:
    UnaryExpression(Location loc, TOK op, Expression* e)
        : Expression(loc), op_(op), e_(e)
    {}

    std::string to_string() const {
        return pprintf("(% %)", colorize(token_string(op_),kGreen), e_->to_string());
    }
};

/// negation unary expression, i.e. -x
class NegUnaryExpression : public UnaryExpression {
public:
    NegUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_minus, e)
    {}
};

/// exponential unary expression, i.e. e^x or exp(x)
class ExpUnaryExpression : public UnaryExpression {
public:
    ExpUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_exp, e)
    {}
};

// logarithm unary expression, i.e. log_10(x)
class LogUnaryExpression : public UnaryExpression {
public:
    LogUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_log, e)
    {}
};

// cosine unary expression, i.e. cos(x)
class CosUnaryExpression : public UnaryExpression {
public:
    CosUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_cos, e)
    {}
};

// sin unary expression, i.e. sin(x)
class SinUnaryExpression : public UnaryExpression {
public:
    SinUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_sin, e)
    {}
};

////////////////////////////////////////////////////////////
// binary expressions
////////////////////////////////////////////////////////////

/// binary expression base class
/// never used directly in the AST, instead the specializations that derive from
/// it are inserted into the AST.
class BinaryExpression : public Expression {
protected:
    Expression *lhs_;
    Expression *rhs_;
    TOK op_;
public:
    BinaryExpression(Location loc, TOK op, Expression* lhs, Expression* rhs)
        : Expression(loc), op_(op), lhs_(lhs), rhs_(rhs)
    {}

    Expression* lhs() {return lhs_;}
    Expression* rhs() {return rhs_;}
    const Expression* lhs() const {return lhs_;}
    const Expression* rhs() const {return rhs_;}

    std::string to_string() const {
        return pprintf("(% % %)", colorize(token_string(op_),kBlue), lhs_->to_string(), rhs_->to_string());
    }
};

class AssignmentExpression : public BinaryExpression {
public:
    AssignmentExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_eq, lhs, rhs)
    {}
};

class AddBinaryExpression : public BinaryExpression {
public:
    AddBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_plus, lhs, rhs)
    {}
};

class SubBinaryExpression : public BinaryExpression {
public:
    SubBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_minus, lhs, rhs)
    {}
};

class MulBinaryExpression : public BinaryExpression {
public:
    MulBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_times, lhs, rhs)
    {}
};

class DivBinaryExpression : public BinaryExpression {
public:
    DivBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_divide, lhs, rhs)
    {}
};

class PowBinaryExpression : public BinaryExpression {
public:
    PowBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_pow, lhs, rhs)
    {}
};

