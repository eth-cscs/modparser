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

    // expressions must provide a method for stringification
    virtual std::string to_string() = 0;

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
        //std::cout << colorize("IdentifierExpression", kGreen) << std::endl;
    }

    std::string to_string() override {
        return name_;
    }

    ~IdentifierExpression() {
        //std::cout << colorize("~IdentifierExpression", kYellow) << std::endl;
    }
private:
    // there has to be some pointer to a table of identifiers
    std::string name_;
};


// a number
class NumberExpression : public Expression {
public:
    NumberExpression(Location loc, std::string const& value)
        : Expression(loc), value_(std::stod(value))
    {}

    double value() const {return value_;};

    std::string to_string() override {
        return pprintf("%", value_);
    }

    ~NumberExpression() {}
private:
    double value_;
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

    std::string to_string() override {
        return name_ + pprintf("(% args : %)", args_.size(), args_);
    }

    ~PrototypeExpression() {
        //std::cout << colorize("~PrototypeExpression", kYellow) << std::endl;
    }
private:
    std::string name_;
    std::vector<Expression*> args_;
};

class ProcedureExpression : public Expression {
public:
    ProcedureExpression(Location loc, PrototypeExpression* proto)
        : Expression(loc), prototype_(proto)
    {}

    PrototypeExpression* prototype() {
        return prototype_;
    }
    std::string const& name() const {
        return prototype_->name();
    }

private:
    std::vector<std::string> locals_;
    PrototypeExpression* prototype_;
    std::vector<Expression *> nodes_;
};
