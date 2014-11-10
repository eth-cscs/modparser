#pragma once

#include <string>
#include <vector>

#include "lexer.h"
#include "util.h"

class Expression {
public:
    explicit Expression(Location location)
        : location_(location)
    {}

    virtual ~Expression();

    // expressions must provide a method for stringification
    virtual std::string to_string() = 0;

    Location const& location() const {return location_;};
protected:
    Location location_;
};

// an identifier
class IdentifierExpression : public Expression {
    IdentifierExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {}

    std::string to_string() override {
        return name_;
    }
private:
    std::string name_;
};

// a proceduce prototype
class PrototypeExpression : public Expression {
public:
    // todo: need to think about rvalue references for copying expression lists
    PrototypeExpression(Location loc, std::string const& name, std::vector<Expression*> const& args)
        : Expression(loc), name_(name), args_(args)
    {}

    std::string const& name() const {return name_;}

    std::string to_string() override {
        return name_ + pprintf("(% args)", args_.size());
    }
private:
    std::string name_;
    std::vector<Expression*> args_;
};

// a number
class NumberExpression : public Expression {
    NumberExpression(Location loc, std::string const& value)
        : Expression(loc), value_(std::stod(value))
    {}

    double value() const {return value_;};

    std::string to_string() override {
        return pprintf("%", value_);
    }

private:
    double value_;
};
