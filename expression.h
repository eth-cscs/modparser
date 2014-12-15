#pragma once

#include <iostream>
#include <memory>
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

    std::shared_ptr<Scope> scope() {return scope_;};

    bool has_error()   { return error_; }
    bool has_warning() { return warning_; }
    std::string const& error_message()   const { return error_string_;   }
    std::string const& warning_message() const { return warning_string_; }

    // perform semantic analysis
    virtual void semantic(std::shared_ptr<Scope>);
    virtual void semantic(Scope::symbol_map&) {assert(false);};

    // easy lookup of properties
    virtual CallExpression*       is_function_call()  {return nullptr;}
    virtual CallExpression*       is_procedure_call() {return nullptr;}
    virtual FunctionExpression*   is_function()       {return nullptr;}
    virtual ProcedureExpression*  is_procedure()      {return nullptr;}
    virtual IdentifierExpression* is_identifier()     {return nullptr;}
    virtual VariableExpression*   is_variable()       {return nullptr;}
    virtual NumberExpression*     is_number()         {return nullptr;}
    virtual BinaryExpression*     is_binary()         {return nullptr;}
    virtual UnaryExpression*      is_unary()          {return nullptr;}
    virtual AssignmentExpression* is_assignment()     {return nullptr;}

    virtual bool is_lvalue() {return false;}

    // force all derived classes to implement visitor
    // this might be a bad idea
    virtual void accept(Visitor *v) = 0;

protected:
    // these are used to flag errors when performing semantic analysis
    bool error_=false;
    bool warning_=false;
    std::string error_string_;
    std::string warning_string_;

    Location location_;

    std::shared_ptr<Scope> scope_;
    //Scope* scope_=nullptr;
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
        return yellow(pprintf("%", name_));
    }

    void semantic(std::shared_ptr<Scope> scp) override;

    Symbol symbol() { return symbol_; };

    void accept(Visitor *v) override {v->visit(this);}

    IdentifierExpression* is_identifier() override {return this;}

    VariableExpression* variable() {
        return symbol_.expression ? symbol_.expression->is_variable() : nullptr;
    }

    bool is_lvalue() override;

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
        return blue("diff") + "(" + yellow(name()) + ")";
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
        return purple(pprintf("%", value_));
    }

    // do nothing for number semantic analysis
    void semantic(std::shared_ptr<Scope> scp) override {};

    NumberExpression* is_number() override {return this;}

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
        return blue("local") + " " + yellow(name_);
    }

    void semantic(std::shared_ptr<Scope> scp) override;

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

    void accept(Visitor *v) override {v->visit(this);}
    VariableExpression* is_variable() override {return this;}

    ~VariableExpression() {}
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
    SolveExpression(
            Location loc,
            std::string const& name,
            solverMethod method)
    : Expression(loc), name_(name), method_(method), procedure_(nullptr)
    {}

    std::string to_string() const override {
        return blue("solve") + "(" + yellow(name_) + ", "
            + green(::to_string(method_)) + ")";
    }

    std::string const& name() const {
        return name_;
    }

    solverMethod method() const {
        return method_;
    }

    ProcedureExpression* procedure() const {
        return procedure_;
    }

    void procedure(ProcedureExpression *e) {
        procedure_ = e;
    }

    void semantic(std::shared_ptr<Scope> scp) override;

    void accept(Visitor *v) override {v->visit(this);}

    ~SolveExpression() {}
private:
    // there has to be some pointer to a table of identifiers
    std::string name_;
    solverMethod method_;

    ProcedureExpression* procedure_;
};

// a proceduce prototype
class PrototypeExpression : public Expression {
public:
    PrototypeExpression(
            Location loc,
            std::string const& name,
            std::vector<Expression*> const& args)
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

// marks a call site in the AST
// is used to mark both function and procedure calls
class CallExpression : public Expression {
public:
    CallExpression(Location loc, std::string const& name, std::vector<Expression*>const &args)
        : Expression(loc), name_(name), args_(args)
    {}

    std::vector<Expression*> const& args() { return args_; }
    std::string const& name() const { return name_; }

    void semantic(std::shared_ptr<Scope> scp) override;

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

    CallExpression* is_function_call()  override {
        return symbol_.kind == k_function ? this : nullptr;
    }
    CallExpression* is_procedure_call() override {
        return symbol_.kind == k_procedure ? this : nullptr;
    }
private:
    std::shared_ptr<Scope> scope_;
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

    ProcedureExpression* is_procedure() override {return this;}

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

private:
    std::shared_ptr<Scope> scope_;
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

    FunctionExpression* is_function() override {return this;}

    void semantic(Scope::symbol_map&) override;

    std::string to_string() const override;

    void accept(Visitor *v) override {v->visit(this);}

private:
    std::shared_ptr<Scope> scope_;
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
        return pprintf("(% %)", green(token_string(op_)), e_->to_string());
    }

    UnaryExpression* is_unary() override {return this;};

    Expression* expression() {return e_;}
    const Expression* expression() const {return e_;}

    void semantic(std::shared_ptr<Scope> scp) override;

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

    BinaryExpression* is_binary() override {return this;}

    void semantic(std::shared_ptr<Scope> scp) override;

    std::string to_string() const {
        return pprintf("(% % %)", blue(token_string(op_)), lhs_->to_string(), rhs_->to_string());
    }

    void accept(Visitor *v) override {v->visit(this);}
};

class AssignmentExpression : public BinaryExpression {
public:
    AssignmentExpression(Location loc, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, tok_eq, lhs, rhs)
    {}

    AssignmentExpression* is_assignment() override {return this;}

    void semantic(std::shared_ptr<Scope> scp) override;

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

