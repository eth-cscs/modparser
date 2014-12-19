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

/// specifies special properties of a ProcedureExpression
enum procedureKind {
    k_proc,             // PROCEDURE
    k_proc_initial,     // INITIAL
    k_proc_net_receive, // NET_RECEIVE
    k_proc_breakpoint,  // BREAKPOINT
    k_proc_derivative   // DERIVATIVE
};

std::string to_string(procedureKind k);

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

    void error(std::string const& str) {
        error_        = true;
        error_string_ = str;
    }
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
    virtual BlockExpression*      is_block()          {return nullptr;}
    virtual IfExpression*         is_if()             {return nullptr;}
    virtual LocalExpression*      is_local_declaration() {return nullptr;}
    virtual FunctionExpression*   is_function()       {return nullptr;}
    virtual ProcedureExpression*  is_procedure()      {return nullptr;}
    virtual NetReceiveExpression* is_net_receive()    {return nullptr;}
    virtual PrototypeExpression*  is_prototype()      {return nullptr;}
    virtual IdentifierExpression* is_identifier()     {return nullptr;}
    virtual VariableExpression*   is_variable()       {return nullptr;}
    virtual NumberExpression*     is_number()         {return nullptr;}
    virtual BinaryExpression*     is_binary()         {return nullptr;}
    virtual UnaryExpression*      is_unary()          {return nullptr;}
    virtual AssignmentExpression* is_assignment()     {return nullptr;}
    virtual ConditionalExpression* is_conditional()   {return nullptr;}
    virtual InitialBlock*         is_initial_block()  {return nullptr;}

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

    VariableExpression* variable();

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
    LocalExpression(Location loc)
        : Expression(loc)
    {}
    LocalExpression(Location loc, std::string const& name)
        : Expression(loc)
    {
        Token tok(tok_identifier, name, loc);
        add_variable(tok);
    }

    std::string to_string() const override;

    bool add_variable(Token name);
    LocalExpression* is_local_declaration() override {return this;}
    void semantic(std::shared_ptr<Scope> scp) override;
    std::vector<Symbol>& symbols() {return symbols_;}
    std::map<std::string, Token>& variables() {return vars_;}
    ~LocalExpression() {}
    void accept(Visitor *v) override {v->visit(this);}
private:
    std::vector<Symbol> symbols_;
    // there has to be some pointer to a table of identifiers
    std::map<std::string, Token> vars_;
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

////////////////////////////////////////////////////////////////////////////////
// recursive if statement
// requires a BlockExpression that is a simple wrapper around a std::vector
// of Expressions surrounded by {}
////////////////////////////////////////////////////////////////////////////////

class BlockExpression : public Expression {
protected:
    std::vector<Expression*> body_;
    bool is_nested_ = false;

public:
    BlockExpression(
        Location loc,
        std::vector<Expression*> body,
        bool is_nested)
    : Expression(loc),
      body_(body),
      is_nested_(is_nested)
    {}

    BlockExpression* is_block() override {
        return this;
    }

    std::vector<Expression*>& body() {
        return body_;
    }

    // provide iterators for easy iteration over body
    auto begin() -> decltype(body_.begin()) {
        return body_.begin();
    }
    auto end() -> decltype(body_.end()) {
        return body_.end();
    }
    auto back() -> decltype(body_.back()) {
        return body_.back();
    }
    auto front() -> decltype(body_.front()) {
        return body_.front();
    }

    void semantic(std::shared_ptr<Scope> scp) override;
    void accept(Visitor* v) override {v->visit(this);};

    std::string to_string() const override;
};

class IfExpression : public Expression {
public:
    IfExpression(Location loc, Expression* con, Expression* tb, Expression* fb)
    : Expression(loc), condition_(con), true_branch_(tb), false_branch_(fb)
    {}

    IfExpression* is_if() override {
        return this;
    }
    Expression* condition() {
        return condition_;
    }
    Expression* true_branch() {
        return true_branch_;
    }
    Expression* false_branch() {
        return false_branch_;
    }

    std::string to_string() const override;
    void semantic(std::shared_ptr<Scope> scp) override;

    void accept(Visitor* v) override {v->visit(this);}
private:
    Expression *condition_;
    Expression *true_branch_;
    Expression *false_branch_;
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
    PrototypeExpression* is_prototype() override {return this;}

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
                         Expression* body,
                         procedureKind k=k_proc)
        : Expression(loc), name_(name), args_(args), kind_(k)
    {
        assert(body->is_block());
        body_ = body->is_block();
    }

    std::vector<Expression*>& args() {
        return args_;
    }
    BlockExpression* body() {
        return body_;
    }
    std::string const& name() const {
        return name_;
    }

    void semantic(Scope::symbol_map &scp) override;
    ProcedureExpression* is_procedure() override {return this;}
    std::string to_string() const override;
    void accept(Visitor *v) override {v->visit(this);}

    /// can be used to determine whether the procedure has been lowered
    /// from a special block, e.g. BREAKPOINT, INITIAL, NET_RECEIVE, etc
    procedureKind kind() const {return kind_;}

protected:
    std::shared_ptr<Scope> scope_;
    Symbol symbol_;

    std::string name_;
    std::vector<Expression *> args_;
    BlockExpression* body_;
    procedureKind kind_ = k_proc;
};

/// stores the INITIAL block in a NET_RECEIVE block, if there is one
/// should not be used anywhere but NET_RECEIVE
class InitialBlock : public BlockExpression {
public:
    InitialBlock(
        Location loc,
        std::vector<Expression*> body)
    : BlockExpression(loc, body, true)
    {}

    std::string to_string() const;
    // currently we use the semantic for a BlockExpression
    // this could be overriden to check for statements
    // specific to initialization of a net_receive block
    //void semantic() override;

    InitialBlock* is_initial_block() override {return this;}
};

/// handle NetReceiveExpressions as a special case of ProcedureExpression
class NetReceiveExpression : public ProcedureExpression {
public:
    NetReceiveExpression( Location loc,
                          std::string const& name,
                          std::vector<Expression*> const& args,
                          Expression* body)
        : ProcedureExpression(loc, name, args, body, k_proc_net_receive)
    {}

    void semantic(Scope::symbol_map &scp) override;
    NetReceiveExpression* is_net_receive() override {return this;}
    /// hard code the kind
    procedureKind kind() {return k_proc_net_receive;}

    void accept(Visitor *v) override {v->visit(this);}
    InitialBlock* initial_block() {return initial_block_;}
protected:
    InitialBlock*  initial_block_=nullptr;
};

class FunctionExpression : public Expression {
private:
    std::shared_ptr<Scope> scope_;
    Symbol symbol_;

    std::string name_;
    std::vector<Expression *> args_;
    BlockExpression* body_;

public:
    FunctionExpression( Location loc,
                         std::string const& name,
                         std::vector<Expression*> const& args,
                         Expression* body)
        : Expression(loc), name_(name), args_(args)
    {
        assert(body->is_block());
        body_ = body->is_block();
    }

    std::vector<Expression*>& args() {
        return args_;
    }
    BlockExpression* body() {
        return body_;
    }
    std::string const& name() const {
        return name_;
    }

    FunctionExpression* is_function() override {return this;}
    void semantic(Scope::symbol_map&) override;
    std::string to_string() const override;
    void accept(Visitor *v) override {v->visit(this);}
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

    void accept(Visitor *v) override {v->visit(this);}
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

class ConditionalExpression : public BinaryExpression {
public:
    ConditionalExpression(Location loc, TOK op, Expression* lhs, Expression* rhs)
        : BinaryExpression(loc, op, lhs, rhs)
    {}

    ConditionalExpression* is_conditional() override {return this;}

    void accept(Visitor *v) override {v->visit(this);}
};

