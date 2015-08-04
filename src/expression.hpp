#pragma once

#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "error.hpp"
#include "identifier.hpp"
#include "memop.hpp"
#include "scope.hpp"
#include "util.hpp"

class Visitor;

class Expression;
class IdentifierExpression;
class BlockExpression;
class InitialBlock;
class IfExpression;
class VariableExpression;
class IndexedVariable;
class NumberExpression;
class LocalDeclaration;
class ArgumentExpression;
class DerivativeExpression;
class PrototypeExpression;
class CallExpression;
class ProcedureExpression;
class NetReceiveExpression;
class APIMethod;
class FunctionExpression;
class UnaryExpression;
class NegUnaryExpression;
class ExpUnaryExpression;
class LogUnaryExpression;
class CosUnaryExpression;
class SinUnaryExpression;
class BinaryExpression;
class AssignmentExpression;
class AddBinaryExpression;
class SubBinaryExpression;
class MulBinaryExpression;
class DivBinaryExpression;
class PowBinaryExpression;
class ConditionalExpression;
class SolveExpression;
class Symbol;

using expression_ptr = std::unique_ptr<Expression>;
using symbol_ptr = std::unique_ptr<Symbol>;

template <typename T, typename... Args>
expression_ptr make_expression(Args&&... args) {
    return expression_ptr(new T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
symbol_ptr make_symbol(Args&&... args) {
    return symbol_ptr(new T(std::forward<Args>(args)...));
}

// helper functions for generating unary and binary expressions
expression_ptr unary_expression(Location, TOK, expression_ptr&&);
expression_ptr unary_expression(TOK, expression_ptr&&);
expression_ptr binary_expression(Location, TOK, expression_ptr&&, expression_ptr&&);
expression_ptr binary_expression(TOK, expression_ptr&&, expression_ptr&&);

/// specifies special properties of a ProcedureExpression
enum procedureKind {
    k_proc_normal,      ///< PROCEDURE
    k_proc_api,         ///< API PROCEDURE
    k_proc_initial,     ///< INITIAL
    k_proc_net_receive, ///< NET_RECEIVE
    k_proc_breakpoint,  ///< BREAKPOINT
    k_proc_derivative   ///< DERIVATIVE
};
std::string to_string(procedureKind k);

/// classification of different symbol kinds
enum symbolKind {
    k_symbol_function,     ///< function call
    k_symbol_procedure,    ///< procedure call
    k_symbol_variable,     ///< variable at module scope
    k_symbol_indexed_variable, ///< a variable that is indexed
    k_symbol_local,        ///< variable at local scope
    k_symbol_argument,     ///< argument variable
    k_symbol_ghost,        ///< variable used as ghost buffer
    k_symbol_none,         ///< no symbol kind (placeholder)
};
std::string to_string(symbolKind k);

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
    using scope_type = Scope<Symbol>;

    explicit Expression(Location location)
    :   location_(location)
    {}

    virtual ~Expression() {};

    // This printer should be implemented with a visitor pattern
    // expressions must provide a method for stringification
    virtual std::string to_string() const = 0;

    Location const& location() const {return location_;};

    std::shared_ptr<scope_type> scope() {return scope_;};

    void error(std::string const& str) {
        error_        = true;
        error_string_ = str;
    }
    bool has_error()   { return error_; }
    bool has_warning() { return warning_; }
    std::string const& error_message()   const { return error_string_;   }
    std::string const& warning_message() const { return warning_string_; }

    // perform semantic analysis
    virtual void semantic(std::shared_ptr<scope_type>);
    virtual void semantic(scope_type::symbol_map&) {
        throw compiler_exception("unable to perform semantic analysis for " + this->to_string(), location_);
    };

    // clone an expression
    virtual expression_ptr clone() const;

    // easy lookup of properties
    virtual CallExpression*        is_function_call()     {return nullptr;}
    virtual CallExpression*        is_procedure_call()    {return nullptr;}
    virtual BlockExpression*       is_block()             {return nullptr;}
    virtual IfExpression*          is_if()                {return nullptr;}
    virtual LocalDeclaration*      is_local_declaration() {return nullptr;}
    virtual ArgumentExpression*    is_argument()          {return nullptr;}
    virtual FunctionExpression*    is_function()          {return nullptr;}
    virtual DerivativeExpression*  is_derivative()        {return nullptr;}
    virtual PrototypeExpression*   is_prototype()         {return nullptr;}
    virtual IdentifierExpression*  is_identifier()        {return nullptr;}
    virtual NumberExpression*      is_number()            {return nullptr;}
    virtual BinaryExpression*      is_binary()            {return nullptr;}
    virtual UnaryExpression*       is_unary()             {return nullptr;}
    virtual AssignmentExpression*  is_assignment()        {return nullptr;}
    virtual ConditionalExpression* is_conditional()       {return nullptr;}
    virtual InitialBlock*          is_initial_block()     {return nullptr;}
    virtual SolveExpression*       is_solve_statement()   {return nullptr;}
    virtual Symbol*                is_symbol()            {return nullptr;}

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

    std::shared_ptr<scope_type> scope_;
};

class Symbol : public Expression {
public :
    Symbol(Location loc, std::string name, symbolKind kind)
    :   Expression(std::move(loc)),
        name_(std::move(name)),
        kind_(kind)
    {}

    std::string const& name() const {
        return name_;
    }

    symbolKind kind() const {
        return kind_;
    }

    void set_kind(symbolKind k) {
        kind_ = k;
    }

    Symbol* is_symbol() override {
        return this;
    }

    std::string to_string() const override;
    void accept(Visitor *v) override;

    virtual IndexedVariable*      is_indexed_variable()  {return nullptr;}
    virtual VariableExpression*   is_variable()          {return nullptr;}
    virtual ProcedureExpression*  is_procedure()         {return nullptr;}
    virtual NetReceiveExpression* is_net_receive()       {return nullptr;}
    virtual APIMethod*            is_api_method()        {return nullptr;}

private :
    std::string name_;

    symbolKind kind_;
};

// an identifier
class IdentifierExpression : public Expression {
public:
    IdentifierExpression(Location loc, std::string const& spelling)
    :   Expression(loc), spelling_(spelling)
    {}

    IdentifierExpression(IdentifierExpression const& other)
    :   Expression(other.location()), spelling_(other.spelling())
    {}

    IdentifierExpression(IdentifierExpression const* other)
    :   Expression(other->location()), spelling_(other->spelling())
    {}

    std::string const& spelling() const {
        return spelling_;
    }

    std::string to_string() const override {
        return yellow(pprintf("%", spelling_));
    }

    expression_ptr clone() const override;

    void semantic(std::shared_ptr<scope_type> scp) override;

    Symbol* symbol() { return symbol_; };

    void accept(Visitor *v) override;

    IdentifierExpression* is_identifier() override {return this;}

    bool is_lvalue() override;

    ~IdentifierExpression() {}

    std::string const& name() const {
        if(symbol_) return symbol_->name();
        throw compiler_exception(
            " attempt to look up name of identifier for which no symbol_ yet defined",
            location_);
    }

protected:
    Symbol* symbol_ = nullptr;

    // there has to be some pointer to a table of identifiers
    std::string spelling_;
};

// an identifier for a derivative
class DerivativeExpression : public IdentifierExpression {
public:
    DerivativeExpression(Location loc, std::string const& name)
    :   IdentifierExpression(loc, name)
    {}

    std::string to_string() const override {
        return blue("diff") + "(" + yellow(spelling()) + ")";
    }
    DerivativeExpression* is_derivative() override { return this; }

    ~DerivativeExpression() {}

    void accept(Visitor *v) override;
};

// a number
class NumberExpression : public Expression {
public:
    NumberExpression(Location loc, std::string const& value)
        : Expression(loc), value_(std::stod(value))
    {}

    NumberExpression(Location loc, long double value)
        : Expression(loc), value_(value)
    {}

    long double value() const {return value_;};

    std::string to_string() const override {
        return purple(pprintf("%", value_));
    }

    // do nothing for number semantic analysis
    void semantic(std::shared_ptr<scope_type> scp) override {};
    expression_ptr clone() const override;

    NumberExpression* is_number() override {return this;}

    ~NumberExpression() {}

    void accept(Visitor *v) override;
private:
    long double value_;
};

// declaration of a LOCAL variable
class LocalDeclaration : public Expression {
public:
    LocalDeclaration(Location loc)
    :   Expression(loc)
    {}
    LocalDeclaration(Location loc, std::string const& name)
    :   Expression(loc)
    {
        Token tok(tok_identifier, name, loc);
        add_variable(tok);
    }

    std::string to_string() const override;

    bool add_variable(Token name);
    LocalDeclaration* is_local_declaration() override {return this;}
    void semantic(std::shared_ptr<scope_type> scp) override;
    std::vector<Symbol*>& symbols() {return symbols_;}
    std::map<std::string, Token>& variables() {return vars_;}
    expression_ptr clone() const override;
    ~LocalDeclaration() {}
    void accept(Visitor *v) override;
private:
    std::vector<Symbol*> symbols_;
    // there has to be some pointer to a table of identifiers
    std::map<std::string, Token> vars_;
};

// declaration of an argument
class ArgumentExpression : public Expression {
public:
    ArgumentExpression(Location loc, Token const& tok)
    :   Expression(loc),
        token_(tok),
        name_(tok.spelling)
    {}

    std::string to_string() const override;

    bool add_variable(Token name);
    ArgumentExpression* is_argument() override {return this;}
    void semantic(std::shared_ptr<scope_type> scp) override;
    Token   token()  {return token_;}
    std::string const& name()  {return name_;}
    void set_name(std::string const& n) {
        name_ = n;
    }

    ~ArgumentExpression() {}
    void accept(Visitor *v) override;
private:
    Token token_;
    std::string name_;
};

// variable definition
class VariableExpression : public Symbol {
public:
    VariableExpression(Location loc, std::string name)
    :   Symbol(loc, std::move(name), k_symbol_variable)
    {}

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

    double value()       const {return value_;}
    void value(double v) {value_ = v;}

    void accept(Visitor *v) override;
    VariableExpression* is_variable() override {return this;}

    ~VariableExpression() {}
protected:

    bool           is_state_    = false;
    accessKind     access_      = k_readwrite;
    visibilityKind visibility_  = k_local_visibility;
    linkageKind    linkage_     = k_extern_link;
    rangeKind      range_kind_  = k_range;
    ionKind        ion_channel_ = k_ion_none;
    double         value_       = std::numeric_limits<double>::quiet_NaN();
};

// an indexed variable
class IndexedVariable : public Symbol {
public:
    IndexedVariable(Location loc, std::string name)
    :   Symbol(loc, std::move(name), k_symbol_indexed_variable)
    {}

    std::string to_string() const override;

    void access(accessKind a) {
        access_ = a;
    }
    void ion_channel(ionKind i) {
        ion_channel_ = i;
    }

    accessKind access() const {
        return access_;
    }
    ionKind ion_channel() const {
        return ion_channel_;
    }

    bool is_ion()       const {return ion_channel_ != k_ion_none;}
    bool is_readable()  const {return access_==k_read  || access_==k_readwrite;}
    bool is_writeable() const {return access_==k_write || access_==k_readwrite;}

    void accept(Visitor *v) override;
    IndexedVariable* is_indexed_variable() override {return this;}

    ~IndexedVariable() {}
protected:
    accessKind     access_      = k_readwrite;
    ionKind        ion_channel_ = k_ion_none;
};

// a SOLVE statement
class SolveExpression : public Expression {
public:
    SolveExpression(
            Location loc,
            std::string name,
            solverMethod method)
    :   Expression(loc), name_(std::move(name)), method_(method), procedure_(nullptr)
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

    SolveExpression* is_solve_statement() override {
        return this;
    }

    expression_ptr clone() const override;

    void semantic(std::shared_ptr<scope_type> scp) override;
    void accept(Visitor *v) override;

    ~SolveExpression() {}
private:
    /// pointer to the variable symbol for the state variable to be solved for
    std::string name_;
    solverMethod method_;

    ProcedureExpression* procedure_;
};

////////////////////////////////////////////////////////////////////////////////
// recursive if statement
// requires a BlockExpression that is a simple wrapper around a std::list
// of Expressions surrounded by {}
////////////////////////////////////////////////////////////////////////////////

class BlockExpression : public Expression {
protected:
    std::list<expression_ptr> body_;
    bool is_nested_ = false;

public:
    BlockExpression(
        Location loc,
        std::list<expression_ptr>&& body,
        bool is_nested)
    :   Expression(loc),
        body_(std::move(body)),
        is_nested_(is_nested)
    {}

    BlockExpression* is_block() override {
        return this;
    }

    std::list<expression_ptr>& body() {
        return body_;
    }

    expression_ptr clone() const override;

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
    bool is_nested() const {
        return is_nested_;
    }

    void semantic(std::shared_ptr<scope_type> scp) override;
    void accept(Visitor* v) override;

    std::string to_string() const override;
};

class IfExpression : public Expression {
public:
    IfExpression(Location loc, expression_ptr&& con, expression_ptr&& tb, expression_ptr&& fb)
    :   Expression(loc), condition_(std::move(con)), true_branch_(std::move(tb)), false_branch_(std::move(fb))
    {}

    IfExpression* is_if() override {
        return this;
    }
    Expression* condition() {
        return condition_.get();
    }
    Expression* true_branch() {
        return true_branch_.get();
    }
    Expression* false_branch() {
        return false_branch_.get();
    }

    std::string to_string() const override;
    void semantic(std::shared_ptr<scope_type> scp) override;

    void accept(Visitor* v) override;
private:
    expression_ptr condition_;
    expression_ptr true_branch_;
    expression_ptr false_branch_;
};

// a proceduce prototype
class PrototypeExpression : public Expression {
public:
    PrototypeExpression(
            Location loc,
            std::string const& name,
            std::vector<expression_ptr>&& args)
    :   Expression(loc), name_(name), args_(std::move(args))
    {}

    std::string const& name() const {return name_;}

    std::vector<expression_ptr>&      args()       {return args_;}
    std::vector<expression_ptr>const& args() const {return args_;}
    PrototypeExpression* is_prototype() override {return this;}

    // TODO: printing out the vector of unique pointers is an unsolved problem...
    std::string to_string() const override {
        return name_; //+ pprintf("(% args : %)", args_.size(), args_);
    }

    ~PrototypeExpression() {}

    void accept(Visitor *v) override;
private:
    std::string name_;
    std::vector<expression_ptr> args_;
};

// marks a call site in the AST
// is used to mark both function and procedure calls
class CallExpression : public Expression {
public:
    CallExpression(Location loc, std::string spelling, std::vector<expression_ptr>&& args)
    :   Expression(loc), spelling_(std::move(spelling)), args_(std::move(args))
    {}

    std::vector<expression_ptr>& args() { return args_; }
    std::vector<expression_ptr> const& args() const { return args_; }

    std::string& name() { return spelling_; }
    std::string const& name() const { return spelling_; }

    void semantic(std::shared_ptr<scope_type> scp) override;
    expression_ptr clone() const override;

    std::string to_string() const override;

    void accept(Visitor *v) override;

    CallExpression* is_function_call()  override {
        return symbol_->kind() == k_symbol_function ? this : nullptr;
    }
    CallExpression* is_procedure_call() override {
        return symbol_->kind() == k_symbol_procedure ? this : nullptr;
    }
private:
    Symbol* symbol_;

    std::string spelling_;
    std::vector<expression_ptr> args_;
};

class ProcedureExpression : public Symbol {
public:
    ProcedureExpression( Location loc,
                         std::string name,
                         std::vector<expression_ptr>&& args,
                         expression_ptr&& body,
                         procedureKind k=k_proc_normal)
    :   Symbol(loc, std::move(name), k_symbol_procedure), args_(std::move(args)), kind_(k)
    {
        if(!body->is_block()) {
            throw compiler_exception(
                " attempt to initialize ProcedureExpression with non-block expression, i.e.\n"
                + body->to_string(),
                location_);
        }
        body_ = std::move(body);
    }

    std::vector<expression_ptr>& args() {
        return args_;
    }
    BlockExpression* body() {
        return body_.get()->is_block();
    }

    void semantic(scope_type::symbol_map &scp) override;
    ProcedureExpression* is_procedure() override {return this;}
    std::string to_string() const override;
    void accept(Visitor *v) override;

    /// can be used to determine whether the procedure has been lowered
    /// from a special block, e.g. BREAKPOINT, INITIAL, NET_RECEIVE, etc
    procedureKind kind() const {return kind_;}

protected:
    Symbol* symbol_;

    std::vector<expression_ptr> args_;
    expression_ptr body_;
    procedureKind kind_ = k_proc_normal;
};

class APIMethod : public ProcedureExpression {
public:
    using memop_type = MemOp<Symbol>;

    APIMethod( Location loc,
               std::string name,
               std::vector<expression_ptr>&& args,
               expression_ptr&& body)
    :   ProcedureExpression(loc, std::move(name), std::move(args), std::move(body), k_proc_api)
    {}

    APIMethod* is_api_method() override {return this;}
    void accept(Visitor *v) override;

    std::vector<memop_type>& inputs() {
        return in_;
    }

    std::vector<memop_type>& outputs() {
        return out_;
    }
    std::string to_string() const override;

protected:
    /// lists the fields that have to be read in from an
    /// indexed array before the kernel can be executed
    /// e.g. voltage values from global voltage vector
    std::vector<memop_type> in_;

    /// lists the fields that have to be written via an
    /// indexed array after the kernel has been executed
    /// e.g. current update for RHS vector
    std::vector<memop_type> out_;
};

/// stores the INITIAL block in a NET_RECEIVE block, if there is one
/// should not be used anywhere but NET_RECEIVE
class InitialBlock : public BlockExpression {
public:
    InitialBlock(
        Location loc,
        std::list<expression_ptr>&& body)
    :   BlockExpression(loc, std::move(body), true)
    {}

    std::string to_string() const override;

    // currently we use the semantic for a BlockExpression
    // this could be overriden to check for statements
    // specific to initialization of a net_receive block
    //void semantic() override;

    void accept(Visitor *v) override;
    InitialBlock* is_initial_block() override {return this;}
};

/// handle NetReceiveExpressions as a special case of ProcedureExpression
class NetReceiveExpression : public ProcedureExpression {
public:
    NetReceiveExpression( Location loc,
                          std::string name,
                          std::vector<expression_ptr>&& args,
                          expression_ptr&& body)
    :   ProcedureExpression(loc, std::move(name), std::move(args), std::move(body), k_proc_net_receive)
    {}

    void semantic(scope_type::symbol_map &scp) override;
    NetReceiveExpression* is_net_receive() override {return this;}
    /// hard code the kind
    procedureKind kind() {return k_proc_net_receive;}

    void accept(Visitor *v) override;
    InitialBlock* initial_block() {return initial_block_;}
protected:
    InitialBlock* initial_block_;
};

class FunctionExpression : public Symbol {
public:
    FunctionExpression( Location loc,
                        std::string name,
                        std::vector<expression_ptr>&& args,
                        expression_ptr&& body)
    :   Symbol(loc, std::move(name), k_symbol_function),
        args_(std::move(args))
    {
        if(!body->is_block()) {
            throw compiler_exception(
                " attempt to initialize FunctionExpression with non-block expression, i.e.\n"
                + body->to_string(),
                location_);
        }
        body_ = std::move(body);
    }

    std::vector<expression_ptr>& args() {
        return args_;
    }
    BlockExpression* body() {
        return body_->is_block();
    }

    FunctionExpression* is_function() override {return this;}
    void semantic(scope_type::symbol_map&) override;
    std::string to_string() const override;
    void accept(Visitor *v) override;

private:
    Symbol* symbol_;

    std::vector<expression_ptr> args_;
    expression_ptr body_;
};

////////////////////////////////////////////////////////////
// unary expressions
////////////////////////////////////////////////////////////

/// Unary expression
class UnaryExpression : public Expression {
protected:
    expression_ptr expression_;
    TOK op_;
public:
    UnaryExpression(Location loc, TOK op, expression_ptr&& e)
    :   Expression(loc),
        expression_(std::move(e)),
        op_(op)
    {}

    std::string to_string() const override {
        return pprintf("(% %)", green(token_string(op_)), expression_->to_string());
    }

    expression_ptr clone() const override;

    TOK op() const {return op_;}
    UnaryExpression* is_unary() override {return this;};
    Expression* expression() {return expression_.get();}
    const Expression* expression() const {return expression_.get();}
    void semantic(std::shared_ptr<scope_type> scp) override;
    void accept(Visitor *v) override;
    void replace_expression(expression_ptr&& other);
};

/// negation unary expression, i.e. -x
class NegUnaryExpression : public UnaryExpression {
public:
    NegUnaryExpression(Location loc, expression_ptr e)
    :   UnaryExpression(loc, tok_minus, std::move(e))
    {}

    void accept(Visitor *v) override;
};

/// exponential unary expression, i.e. e^x or exp(x)
class ExpUnaryExpression : public UnaryExpression {
public:
    ExpUnaryExpression(Location loc, expression_ptr e)
    :   UnaryExpression(loc, tok_exp, std::move(e))
    {}

    void accept(Visitor *v) override;
};

// logarithm unary expression, i.e. log_10(x)
class LogUnaryExpression : public UnaryExpression {
public:
    LogUnaryExpression(Location loc, expression_ptr e)
    :   UnaryExpression(loc, tok_log, std::move(e))
    {}

    void accept(Visitor *v) override;
};

// cosine unary expression, i.e. cos(x)
class CosUnaryExpression : public UnaryExpression {
public:
    CosUnaryExpression(Location loc, expression_ptr e)
    :   UnaryExpression(loc, tok_cos, std::move(e))
    {}

    void accept(Visitor *v) override;
};

// sin unary expression, i.e. sin(x)
class SinUnaryExpression : public UnaryExpression {
public:
    SinUnaryExpression(Location loc, expression_ptr e)
    :   UnaryExpression(loc, tok_sin, std::move(e))
    {}

    void accept(Visitor *v) override;
};

////////////////////////////////////////////////////////////
// binary expressions

////////////////////////////////////////////////////////////

/// binary expression base class
/// never used directly in the AST, instead the specializations that derive from
/// it are inserted into the AST.
class BinaryExpression : public Expression {
protected:
    expression_ptr lhs_;
    expression_ptr rhs_;
    TOK op_;
public:
    BinaryExpression(Location loc, TOK op, expression_ptr&& lhs, expression_ptr&& rhs)
    :   Expression(loc),
        lhs_(std::move(lhs)),
        rhs_(std::move(rhs)),
        op_(op)
    {}

    TOK op() const {return op_;}
    Expression* lhs() {return lhs_.get();}
    Expression* rhs() {return rhs_.get();}
    const Expression* lhs() const {return lhs_.get();}
    const Expression* rhs() const {return rhs_.get();}
    BinaryExpression* is_binary() override {return this;}
    void semantic(std::shared_ptr<scope_type> scp) override;
    expression_ptr clone() const override;
    void replace_rhs(expression_ptr&& other);
    void replace_lhs(expression_ptr&& other);
    std::string to_string() const override;
    void accept(Visitor *v) override;
};

class AssignmentExpression : public BinaryExpression {
public:
    AssignmentExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_eq, std::move(lhs), std::move(rhs))
    {}

    AssignmentExpression* is_assignment() override {return this;}

    void semantic(std::shared_ptr<scope_type> scp) override;

    void accept(Visitor *v) override;
};

class AddBinaryExpression : public BinaryExpression {
public:
    AddBinaryExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_plus, std::move(lhs), std::move(rhs))
    {}

    void accept(Visitor *v) override;
};

class SubBinaryExpression : public BinaryExpression {
public:
    SubBinaryExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_minus, std::move(lhs), std::move(rhs))
    {}

    void accept(Visitor *v) override;
};

class MulBinaryExpression : public BinaryExpression {
public:
    MulBinaryExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_times, std::move(lhs), std::move(rhs))
    {}

    void accept(Visitor *v) override;
};

class DivBinaryExpression : public BinaryExpression {
public:
    DivBinaryExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_divide, std::move(lhs), std::move(rhs))
    {}

    void accept(Visitor *v) override;
};

class PowBinaryExpression : public BinaryExpression {
public:
    PowBinaryExpression(Location loc, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, tok_pow, std::move(lhs), std::move(rhs))
    {}

    void accept(Visitor *v) override;
};

class ConditionalExpression : public BinaryExpression {
public:
    ConditionalExpression(Location loc, TOK op, expression_ptr&& lhs, expression_ptr&& rhs)
    :   BinaryExpression(loc, op, std::move(lhs), std::move(rhs))
    {}

    ConditionalExpression* is_conditional() override {return this;}

    void accept(Visitor *v) override;
};

