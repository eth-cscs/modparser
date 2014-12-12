#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "util.h"
#include "identifier.h"
#include "lexer.h"
#include "scope.h"
#include "visitor.h"

/// methods for time stepping state
enum solverMethod {
    k_cnexp // the only method we have at the moment
};

static std::string to_string(solverMethod m) {
    switch(m) {
        case k_cnexp : return std::string("cnexp");
    }
    return std::string("<error : undefined solverMethod>");
}

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

    Scope* scope() {return scope_;};

    bool has_error() { return error_; }
    std::string const& error_message() const { return error_string_; }

    // perform semantic analysis
    virtual void semantic(Scope*);
    virtual void semantic(Scope::symbol_map&) {assert(false);};

    // easy lookup of properties
    virtual bool is_function_call()  const {return false;}
    virtual bool is_procedure_call() const {return false;}
    virtual bool is_identifier()     const {return false;}
    virtual bool is_number()         const {return false;}
    virtual bool is_binary()         const {return false;}
    virtual bool is_unary()          const {return false;}
    virtual bool is_assignment()     const {return false;}

    // force all derived classes to implement visitor
    // this might be a bad idea
    virtual void accept(Visitor *v) = 0;

protected:
    // these are used to flag errors when performing semantic analysis
    bool error_=false;
    std::string error_string_;

    Location location_;

    Scope* scope_=nullptr;
};

// an identifier
class IdentifierExpression : public Expression {
public:
    IdentifierExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {}

    std::string const& name() const {
        return name_;
    }

    std::string to_string() const override {
        return colorize(pprintf("%", name_), kYellow);
    }

    void semantic(Scope* scp) override;

    Symbol symbol() { return symbol_; };

    void accept(Visitor *v) override {v->visit(this);}

    bool is_identifier() const override {return true;}

    ~IdentifierExpression() {}
protected:
    Symbol symbol_;

    // there has to be some pointer to a table of identifiers
    std::string name_;
};

// an identifier for a derivative
class DerivativeExpression : public IdentifierExpression {
public:
    DerivativeExpression(Location loc, std::string const& name)
        : IdentifierExpression(loc, name)
    {}

    std::string to_string() const override {
        return colorize("diff",kBlue) + "(" + colorize(name(), kYellow) + ")";
    }

    ~DerivativeExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
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

    // do nothing for number semantic analysis
    void semantic(Scope* scp) override {};

    bool is_number() const override {return true;}

    ~NumberExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
private:
    double value_;
};

// declaration of a LOCAL variable
class LocalExpression : public Expression {
public:
    LocalExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {}

    std::string to_string() const override {
        return colorize("local", kBlue) + " " + colorize(name_,kYellow);
    }

    void semantic(Scope* scp) override;

    Symbol symbol() {return symbol_;}

    ~LocalExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
private:
    Symbol symbol_;
    // there has to be some pointer to a table of identifiers
    std::string name_;
};

// variable definition
class VariableExpression : public Expression {
public:
    VariableExpression(Location loc, std::string const& name)
        : Expression(loc), name_(name)
    {}

    std::string const& name() const {
        return name_;
    }

    std::string to_string() const override;

    void access(accessKind a) {
        access_ = a;
    }
    void visibility(visibilityKind v) {
        visibility_ = v;
    }
    void linkage(linkageKind l) {
        linkage_ = l;
    }
    void range(rangeKind r) {
        range_kind_ = r;
    }
    void ion_channel(ionKind i) {
        ion_channel_ = i;
    }
    void state(bool s) {
        is_state_ = s;
    }

    accessKind access() const {
        return access_;
    }
    visibilityKind visibility() const {
        return visibility_;
    }
    linkageKind linkage() const {
        return linkage_;
    }
    ionKind ion_channel() const {
        return ion_channel_;
    }

    bool is_ion()       const {return ion_channel_ != k_ion_none;}
    bool is_state()     const {return is_state_;}
    bool is_range()     const {return range_kind_  == k_range;}
    bool is_scalar()    const {return !is_range();}
    bool is_readable()  const {return access_==k_read  || access_==k_readwrite;}
    bool is_writeable() const {return access_==k_write || access_==k_readwrite;}

    ~VariableExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
protected:
    // there has to be some pointer to a table of identifiers
    std::string name_;

    bool           is_state_    = false;
    accessKind     access_      = k_readwrite;
    visibilityKind visibility_  = k_local_visibility;
    linkageKind    linkage_     = k_extern_link;
    rangeKind      range_kind_  = k_range;
    ionKind        ion_channel_ = k_ion_none;
};


// a SOLVE statement
class SolveExpression : public Expression {
public:
    SolveExpression(Location loc, std::string const& name, solverMethod method)
        : Expression(loc), name_(name), method_(method), derivative_expression_(nullptr)
    {}

    std::string to_string() const override {
        return colorize("solve", kBlue) + "(" + colorize(name_,kYellow) + ", " + colorize(::to_string(method_),kGreen) + ")";
    }

    std::string const& name() const {
        return name_;
    }

    solverMethod method() const {
        return method_;
    }

    Expression* derivative_expression() const {
        return derivative_expression_;
    }

    void derivative_expression(Expression *e) {
        derivative_expression_ = e;
    }

    ~SolveExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
private:
    // there has to be some pointer to a table of identifiers
    std::string name_;
    solverMethod method_;

    Expression *derivative_expression_;
};

// a proceduce prototype
class PrototypeExpression : public Expression {
public:
    PrototypeExpression(Location loc, std::string const& name, std::vector<Expression*> const& args)
        : Expression(loc), name_(name), args_(args)
    {}

    std::string const& name() const {return name_;}

    std::vector<Expression*>&      args()       {return args_;}
    std::vector<Expression*>const& args() const {return args_;}

    std::string to_string() const override {
        return name_ + pprintf("(% args : %)", args_.size(), args_);
    }

    ~PrototypeExpression() {}

    void accept(Visitor *v) override {v->visit(this);}
private:
    std::string name_;
    std::vector<Expression*> args_;
};

class CallExpression : public Expression {
public:
    CallExpression(Location loc, std::string const& name, std::vector<Expression*>const &args)
        : Expression(loc), name_(name), args_(args)
    {}

    std::vector<Expression*> const& args() { return args_; }
    std::string const& name() const { return name_; }

    void semantic(Scope* scp) override;

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

    bool is_function_call()  const override {return symbol_.kind == k_function;}
    bool is_procedure_call() const override {return symbol_.kind == k_procedure;}
private:
    Scope* scope_;
    Symbol symbol_;

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

    void semantic(Scope::symbol_map &scp) override;

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

private:
    Scope* scope_;
    Symbol symbol_;

    std::string name_;
    std::vector<Expression *> args_;
    std::vector<Expression *> body_;
};

class FunctionExpression : public Expression {
public:
    FunctionExpression( Location loc,
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

    void semantic(Scope::symbol_map&) override;

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

private:
    Scope* scope_;
    Symbol symbol_;

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

    bool is_unary() const override {return true;};

    Expression* expression() {return e_;}
    const Expression* expression() const {return e_;}

    void semantic(Scope* scp) override;

    void accept(Visitor *v) override {std::cout << "here unary\n"; v->visit(this);}
};

/// negation unary expression, i.e. -x
class NegUnaryExpression : public UnaryExpression {
public:
    NegUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_minus, e)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

/// exponential unary expression, i.e. e^x or exp(x)
class ExpUnaryExpression : public UnaryExpression {
public:
    ExpUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_exp, e)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

// logarithm unary expression, i.e. log_10(x)
class LogUnaryExpression : public UnaryExpression {
public:
    LogUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_log, e)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

// cosine unary expression, i.e. cos(x)
class CosUnaryExpression : public UnaryExpression {
public:
    CosUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_cos, e)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

// sin unary expression, i.e. sin(x)
class SinUnaryExpression : public UnaryExpression {
public:
    SinUnaryExpression(Location loc, Expression* e)
        : UnaryExpression(loc, tok_sin, e)
    {}

    void accept(Visitor *v) override {v->visit(this);}
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

    bool is_binary() const override {return true;}

    void semantic(Scope* scp) override;

    std::string to_string() const {
        return pprintf("(% % %)", colorize(token_string(op_),kBlue), lhs_->to_string(), rhs_->to_string());
    }

    void accept(Visitor *v) override {v->visit(this);}
};

class AssignmentExpression : public BinaryExpression {
public:
    AssignmentExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_eq, lhs, rhs)
    {}

    bool is_assignment() const override {return true;}

    void semantic(Scope* scp) override;

    void accept(Visitor *v) override {v->visit(this);}
};

class AddBinaryExpression : public BinaryExpression {
public:
    AddBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_plus, lhs, rhs)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

class SubBinaryExpression : public BinaryExpression {
public:
    SubBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_minus, lhs, rhs)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

class MulBinaryExpression : public BinaryExpression {
public:
    MulBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_times, lhs, rhs)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

class DivBinaryExpression : public BinaryExpression {
public:
    DivBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_divide, lhs, rhs)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

class PowBinaryExpression : public BinaryExpression {
public:
    PowBinaryExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_pow, lhs, rhs)
    {}

    void accept(Visitor *v) override {v->visit(this);}
};

