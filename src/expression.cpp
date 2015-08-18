#include <cstring>

#include "expression.hpp"

std::string to_string(symbolKind k) {
    switch (k) {
        case symbolKind::variable:
            return std::string("variable");
        case symbolKind::indexed_variable:
            return std::string("indexed variable");
        case symbolKind::local_variable:
            return std::string("local");
        case symbolKind::procedure:
            return std::string("procedure");
        case symbolKind::function:
            return std::string("function");
    }
    return "";
}


std::string to_string(procedureKind k) {
    switch(k) {
        case procedureKind::normal             :
            return "procedure";
        case procedureKind::api             :
            return "APIprocedure";
        case procedureKind::initial     :
            return "initial";
        case procedureKind::net_receive :
            return "net_receive";
        case procedureKind::breakpoint  :
            return "breakpoint";
        case procedureKind::derivative  :
            return "derivative";
        default :
            return "undefined";
    }
}

/*******************************************************************************
  Expression
*******************************************************************************/

void Expression::semantic(std::shared_ptr<scope_type>) {
    error("semantic() has not been implemented for this expression");
}

expression_ptr Expression::clone() const {
    throw compiler_exception(
        "clone() has not been implemented for " + this->to_string(),
        location_);
}

/*******************************************************************************
  Symbol
*******************************************************************************/

std::string Symbol::to_string() const {
    return blue("Symbol") + " " + yellow(name_);
}

/*******************************************************************************
  LocalVariable
*******************************************************************************/

std::string LocalVariable::to_string() const {
    std::string s = blue("Local Variable") + " " + yellow(name());
    if(is_indexed()) {
        s += " ->(" + token_string(external_->op()) + ") " + yellow(external_->index_name());
    }
    return s;
}

/*******************************************************************************
  IdentifierExpression
*******************************************************************************/

void IdentifierExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;

    auto s = scope_->find(spelling_);

    if(s==nullptr) {
        error( pprintf("the variable '%' is undefined",
                        yellow(spelling_), location_));
        return;
    }
    if(s->kind() == symbolKind::procedure || s->kind() == symbolKind::function) {
        error( pprintf("the symbol '%' is a function/procedure, not a variable",
                       yellow(spelling_)));
        return;
    }
    // if the symbol is an indexed variable, this is the first time that the
    // indexed variable is used in this procedure. In which case, we create
    // a local variable which refers to the indexed variable, which will be
    // found for any subsequent variable lookup inside the procedure
    if(auto sym = s->is_indexed_variable()) {
        auto var = new LocalVariable(location_, spelling_);
        var->external_variable(sym);
        s = scope_->add_local_symbol(spelling_, scope_type::symbol_ptr{var});
    }

    // save the symbol
    symbol_ = s;
}

expression_ptr IdentifierExpression::clone() const {
    return make_expression<IdentifierExpression>(location_, spelling_);
}

bool IdentifierExpression::is_lvalue() {
    // check for global variable that is writeable
    auto var = symbol_->is_variable();
    if(var) return var->is_writeable();

    // else look for local symbol
    if( symbol_->kind() == symbolKind::local_variable ) {
        return true;
    }

    return false;
}

/*******************************************************************************
  NumberExpression
********************************************************************************/

expression_ptr NumberExpression::clone() const {
    return make_expression<NumberExpression>(location_, value_);
}

/*******************************************************************************
  LocalDeclaration
*******************************************************************************/

std::string LocalDeclaration::to_string() const {
    std::string str = blue("local");
    for(auto v : vars_) {
        str += " " + yellow(v.first);
    }
    return str;
}

expression_ptr LocalDeclaration::clone() const {
    auto local = new LocalDeclaration(location());
    for(auto &v : vars_) {
        local->add_variable(v.second);
    }
    return expression_ptr{local};
}

bool LocalDeclaration::add_variable(Token tok) {
    if(vars_.find(tok.spelling)!=vars_.end()) {
        error( "the variable '" + yellow(tok.spelling) + "' is defined more than once");
        return false;
    }

    vars_[tok.spelling] = tok;
    return true;
}

void LocalDeclaration::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;

    // loop over the variables declared in this LOCAL statement
    for(auto &v : vars_) {
        auto &name = v.first;
        auto s = scope_->find(name);

        // First check that the variable is undefined
        // Note that we allow for local variables with the same name as
        // class scope variables (globals), in which case the local variable
        // name will be used for lookup
        if(   s==nullptr    // symbol has not been defined yet
           || s->kind()==symbolKind::variable  // symbol is defined at global scope
           || s->kind()==symbolKind::indexed_variable)
        {
            if(s && s->kind()==symbolKind::indexed_variable) {
                warning(pprintf("The local variable '%' clashes with the indexed"
                                " variable defined at %, which will be ignored."
                                " Remove the local definition of this variable"
                                " if the previously defined variable was intended.",
                                 yellow(name), s->location() ));
            }
            auto symbol = make_symbol<LocalVariable>(location_, name);
            symbols_.push_back( scope_->add_local_symbol(name, std::move(symbol)) );
        }
        else {
            error(pprintf("the symbol '%' has already been defined at %",
                          yellow(name), s->location() ));
        }
    }
}

/*******************************************************************************
  ArgumentExpression
*******************************************************************************/
std::string ArgumentExpression::to_string() const {
    return blue("arg") + " " + yellow(name_);
}

void ArgumentExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;

    auto s = scope_->find(name_);

    if(s==nullptr || s->kind()==symbolKind::variable || s->kind()==symbolKind::indexed_variable) {
        auto symbol = make_symbol<LocalVariable>( location_, name_, localVariableKind::argument );
        scope_->add_local_symbol(name_, std::move(symbol));
    }
    else {
        error(pprintf("the symbol '%' has already been defined at %",
                      yellow(name_), s->location() ));
    }
}


/*******************************************************************************
  VariableExpression
*******************************************************************************/

std::string VariableExpression::to_string() const {
    char n[17];
    snprintf(n, 17, "%-10s", name().c_str());
    std::string
        s = blue("variable") + " " + yellow(n) + "("
          + colorize("write", is_writeable() ? stringColor::green : stringColor::red) + ", "
          + colorize("read", is_readable() ? stringColor::green : stringColor::red)   + ", "
          + (is_range() ? "range" : "scalar")                 + ", "
          + "ion" + colorize(::to_string(ion_channel()),
                             (ion_channel()==ionKind::none) ? stringColor::red : stringColor::green) + ", "
          + "vis "  + ::to_string(visibility()) + ", "
          + "link " + ::to_string(linkage())    + ", "
          + colorize("state", is_state() ? stringColor::green : stringColor::red) + ")";
    return s;
}

/*******************************************************************************
  IndexedVariable
*******************************************************************************/

std::string IndexedVariable::to_string() const {
    auto ch = ::to_string(ion_channel());
    return
        blue("indexed") + " " + yellow(name()) + "->" + yellow(index_name()) + "("
        + (is_write() ? " write-only" : " read-only")
        + ", ion" + (ion_channel()==ionKind::none ? red(ch) : green(ch)) + ") ";
}

/*******************************************************************************
  CallExpression
*******************************************************************************/

std::string CallExpression::to_string() const {
    std::string str = blue("call") + " " + yellow(spelling_) + " (";
    for(auto& arg : args_)
        str += arg->to_string() + ", ";
    str += ")";

    return str;
}

void CallExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;

    // look up to see if symbol is defined
    // restrict search to global namespace
    auto s = scope_->find_global(spelling_);

    // either undefined or refers to a variable
    if(!s) {
        error(pprintf("there is no function or procedure named '%' ",
                      yellow(spelling_)));
    }
    if(s->kind()==symbolKind::local_variable || s->kind()==symbolKind::variable) {
        error(pprintf("the symbol '%' refers to a variable, but it is being"
                      " called like a function", yellow(spelling_) ));
    }

    // save the symbol
    symbol_ = s;

    // check that the number of passed arguments matches
    if( !has_error() ) { // only analyze if the call was found
        int expected_args;
        if(auto f = function()) {
            expected_args = f->args().size();
        }
        else {
            expected_args = procedure()->args().size();
        }
        if(args_.size() != expected_args) {
            error(pprintf("call has the wrong number of arguments: expected %"
                          ", received %", expected_args, args_.size()));
        }
    }

    // perform semantic analysis on the arguments
    for(auto& a : args_) {
        a->semantic(scp);
    }
}

expression_ptr CallExpression::clone() const {
    // clone the arguments
    std::vector<expression_ptr> cloned_args;
    for(auto& a: args_) {
        cloned_args.emplace_back(a->clone());
    }

    return make_expression<CallExpression>(location_, spelling_, std::move(cloned_args));
}

/*******************************************************************************
  ProcedureExpression
*******************************************************************************/

std::string ProcedureExpression::to_string() const {
    std::string str = blue("procedure") + " " + yellow(name()) + "\n";
    str += blue("  special") + " : " + ::to_string(kind_) + "\n";
    str += blue("  args") + "    : ";
    for(auto& arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+blue("body")+" :";
    str += body_->to_string();

    return str;
}

void ProcedureExpression::semantic(scope_type::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    if(global_symbols.find(name()) == global_symbols.end()) {
        throw compiler_exception(
            "attempt to perform semantic analysis for procedure '"
            + yellow(name())
            + "' which has not been added to global symbol table",
            location_);
    }

    // create the scope for this procedure
    scope_ = std::make_shared<scope_type>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto& a : args_) {
        a->semantic(scope_);
    }

    // this loop could be used to then check the types of statements in the body
    for(auto& e : *(body_->is_block())) {
        if(e->is_initial_block())
            error("INITIAL block not allowed inside "+::to_string(kind_)+" definition");
    }

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);

    // the symbol for this expression is itself
    symbol_ = scope_->find_global(name());
}

/*******************************************************************************
  APIMethod
*******************************************************************************/

std::string APIMethod::to_string() const {
    auto namestr = [] (Symbol* e) -> std::string {
        return yellow(e->name());
        return "";
    };
    std::string str = blue("API method") + " " + yellow(name()) + "\n";

    str += blue("  locals") + " : ";
    for(auto& var : scope_->locals()) {
        str += namestr(var.second.get());
        str += ", ";
    }
    str += "\n";

    str += "  "+blue("body  ")+" : ";
    str += body_->to_string();

    return str;
}

/*******************************************************************************
  InitialBlock
*******************************************************************************/

std::string InitialBlock::to_string() const {
    std::string str = green("[[initial");
    for(auto& ex : body_) {
       str += "\n   " + ex->to_string();
    }
    str += green("\n  ]]");
    return str;
}

/*******************************************************************************
  NetReceiveExpression
*******************************************************************************/

void NetReceiveExpression::semantic(scope_type::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    if(global_symbols.find(name()) == global_symbols.end()) {
        throw compiler_exception(
            "attempt to perform semantic analysis for procedure '"
            + yellow(name())
            + "' which has not been added to global symbol table",
            location_);
    }

    // create the scope for this procedure
    scope_ = std::make_shared<scope_type>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto& a : args_) {
        a->semantic(scope_);
    }

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);
    // this loop could be used to then check the types of statements in the body
    for(auto& e : *(body_->is_block())) {
        if(e->is_initial_block()) {
            if(initial_block_) {
                error("only one INITIAL block is permitted per NET_RECEIVE block");
            }
            initial_block_ = e->is_initial_block();
        }
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = scope_->find_global(name());
}

/*******************************************************************************
  FunctionExpression
*******************************************************************************/

std::string FunctionExpression::to_string() const {
    std::string str = blue("function") + " " + yellow(name()) + "\n";
    str += blue("  args") + " : ";
    for(auto& arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+blue("body")+" :";
    str += body_->to_string();

    return str;
}

void FunctionExpression::semantic(scope_type::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    if(global_symbols.find(name()) == global_symbols.end()) {
        throw compiler_exception(
            "attempt to perform semantic analysis for procedure '"
            + yellow(name())
            + "' which has not been added to global symbol table",
            location_);
    }

    // create the scope for this procedure
    scope_ = std::make_shared<scope_type>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto& a : args_) {
        a->semantic(scope_);
    }

    // Add a variable that has the same name as the function,
    // which acts as a placeholder for the return value
    // Make its location correspond to that of the first line of the function,
    // for want of a better location
    auto return_var = scope_type::symbol_ptr(
        new Symbol(body_->location(), name(), symbolKind::local_variable)
    );
    scope_->add_local_symbol(name(), std::move(return_var));

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);
    // this loop could be used to then check the types of statements in the body
    for(auto& e : *(body())) {
        if(e->is_initial_block()) error("INITIAL block not allowed inside FUNCTION definition");
    }

    // check that the last expression in the body was an assignment to
    // the return placeholder
    bool last_expr_is_assign = false;
    auto tail = body()->back()->is_assignment();
    if(tail) {
        // we know that the tail is an assignment expression
        auto lhs = tail->lhs()->is_identifier();
        // use nullptr check followed by lazy name lookup
        if(lhs && lhs->name()==name()) {
            last_expr_is_assign = true;
        }
    }
    if(!last_expr_is_assign) {
        warning("the last expression in function '"
                + yellow(name())
                + "' does not set the return value");
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = scope_->find_global(name());
}

/*******************************************************************************
  UnaryExpression
*******************************************************************************/
void UnaryExpression::semantic(std::shared_ptr<scope_type> scp) {
    expression_->semantic(scp);

    if(expression_->is_procedure_call()) {
        error("a procedure call can't be part of an expression");
    }
}

void UnaryExpression::replace_expression(expression_ptr&& other) {
    std::swap(expression_, other);
}

expression_ptr UnaryExpression::clone() const {
    return unary_expression(location_, op_, expression_->clone());
}

/*******************************************************************************
  BinaryExpression
*******************************************************************************/
void BinaryExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    if(rhs_->is_procedure_call() || lhs_->is_procedure_call()) {
        error("procedure calls can't be made in an expression");
    }
}

expression_ptr BinaryExpression::clone() const {
    return binary_expression(location_, op_, lhs_->clone(), rhs_->clone());
}

void BinaryExpression::replace_lhs(expression_ptr&& other) {
    std::swap(lhs_, other);
}

void BinaryExpression::replace_rhs(expression_ptr&& other) {
    std::swap(rhs_, other);
}

std::string BinaryExpression::to_string() const {
    //return pprintf("(% % %)", blue(token_string(op_)), lhs_->to_string(), rhs_->to_string());
    return pprintf("(% % %)", lhs_->to_string(), blue(token_string(op_)), rhs_->to_string());
}

/*******************************************************************************
  AssignmentExpression
*******************************************************************************/

void AssignmentExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    // only flag an lvalue error if there was no error in the lhs expression
    // this ensures that we don't print redundant error messages when trying
    // to write to an undeclared variable
    if(!lhs_->has_error() && !lhs_->is_lvalue()) {
        error("the left hand side of an assignment must be an lvalue");
    }
    if(rhs_->is_procedure_call()) {
        error("procedure calls can't be made in an expression");
    }
}

/*******************************************************************************
  SolveExpression
*******************************************************************************/

void SolveExpression::semantic(std::shared_ptr<scope_type> scp) {
    auto e = scp->find(name());
    auto proc = e ? e->is_procedure() : nullptr;

    // this is optimistic: it simply looks for a procedure,
    // it should also evaluate the procedure to see whether it contains the derivatives
    // if an integration method has been specified (i.e. cnexp)
    if(proc) {
        procedure_ = proc;
    }
    else {
        error( "'" + yellow(name_) + "' is not a valid procedure name"
                        " for computing the derivatives in a SOLVE statement");
    }
}

expression_ptr SolveExpression::clone() const {
    auto s = new SolveExpression(location_, name_, method_);
    s->procedure(procedure_);
    return expression_ptr{s};
}

/*******************************************************************************
  ConductanceExpression
*******************************************************************************/

void ConductanceExpression::semantic(std::shared_ptr<scope_type> scp) {
    // For now do nothing with the CONDUCTANCE statement, because it is not needed
    // to optimize conductance calculation.
    // Semantic analysis would involve
    //   -  check that name identifies a valid variable
    //   -  check that the ion channel is an ion channel for which current
    //      is to be updated
    /*
    auto e = scp->find(name());
    auto var = e ? e->is_variable() : nullptr;
    */
}

expression_ptr ConductanceExpression::clone() const {
    auto s = new ConductanceExpression(location_, name_, ion_channel_);
    //s->procedure(procedure_);
    return expression_ptr{s};
}

/*******************************************************************************
  BlockExpression
*******************************************************************************/

std::string BlockExpression::to_string() const {
    std::string str = green("[[");
    for(auto& ex : body_) {
       str += "\n   " + ex->to_string();
    }
    str += green("\n  ]]");
    return str;
}

void BlockExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;
    for(auto& e : body_) {
        e->semantic(scope_);
    }
}

expression_ptr BlockExpression::clone() const {
    std::list<expression_ptr> body;
    for(auto& e: body_) {
        body.emplace_back(e->clone());
    }
    return make_expression<BlockExpression>(location_, std::move(body), is_nested_);
}

/*******************************************************************************
  IfExpression
*******************************************************************************/

std::string IfExpression::to_string() const {
    std::string s = blue("if") + " :";
    s += "\n  " + white("condition") + "    : " + condition_->to_string();
    s += "\n  " + white("true branch") + "  :\n" + true_branch_->to_string();
    s += "\n  " + white("false branch") + " :";
    s += (false_branch_ ? "\n" + false_branch_->to_string() : "");
    return s;
}

void IfExpression::semantic(std::shared_ptr<scope_type> scp) {
    condition_->semantic(scp);

    auto cond = condition_->is_conditional();
    if(!cond) {
        error("not a valid conditional expression");
    }

    true_branch_->semantic(scp);

    if(false_branch_) {
        false_branch_->semantic(scp);
    }
}

expression_ptr IfExpression::clone() const {
    return make_expression<IfExpression>(
            location_,
            condition_->clone(),
            true_branch_->clone(),
            false_branch_? false_branch_->clone() : nullptr
    );
}

#include "visitor.hpp"
/*
   Visitor hooks
*/
void Expression::accept(Visitor *v) {
    v->visit(this);
}
void Symbol::accept(Visitor *v) {
    v->visit(this);
}
void LocalVariable::accept(Visitor *v) {
    v->visit(this);
}
void IdentifierExpression::accept(Visitor *v) {
    v->visit(this);
}
void BlockExpression::accept(Visitor *v) {
    v->visit(this);
}
void InitialBlock::accept(Visitor *v) {
    v->visit(this);
}
void IfExpression::accept(Visitor *v) {
    v->visit(this);
}
void SolveExpression::accept(Visitor *v) {
    v->visit(this);
}
void ConductanceExpression::accept(Visitor *v) {
    v->visit(this);
}
void DerivativeExpression::accept(Visitor *v) {
    v->visit(this);
}
void VariableExpression::accept(Visitor *v) {
    v->visit(this);
}
void IndexedVariable::accept(Visitor *v) {
    v->visit(this);
}
void NumberExpression::accept(Visitor *v) {
    v->visit(this);
}
void LocalDeclaration::accept(Visitor *v) {
    v->visit(this);
}
void ArgumentExpression::accept(Visitor *v) {
    v->visit(this);
}
void PrototypeExpression::accept(Visitor *v) {
    v->visit(this);
}
void CallExpression::accept(Visitor *v) {
    v->visit(this);
}
void ProcedureExpression::accept(Visitor *v) {
    v->visit(this);
}
void NetReceiveExpression::accept(Visitor *v) {
    v->visit(this);
}
void APIMethod::accept(Visitor *v) {
    v->visit(this);
}
void FunctionExpression::accept(Visitor *v) {
    v->visit(this);
}
void UnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void NegUnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void ExpUnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void LogUnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void CosUnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void SinUnaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void BinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void AssignmentExpression::accept(Visitor *v) {
    v->visit(this);
}
void AddBinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void SubBinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void MulBinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void DivBinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void PowBinaryExpression::accept(Visitor *v) {
    v->visit(this);
}
void ConditionalExpression::accept(Visitor *v) {
    v->visit(this);
}

expression_ptr unary_expression( tok op,
                                 expression_ptr&& e
                               )
{
    return unary_expression(op, std::move(e));
}

expression_ptr unary_expression( Location loc,
                                 tok op,
                                 expression_ptr&& e
                               )
{
    switch(op) {
        case tok::minus :
            return make_expression<NegUnaryExpression>(loc, std::move(e));
        case tok::exp :
            return make_expression<ExpUnaryExpression>(loc, std::move(e));
        case tok::cos :
            return make_expression<CosUnaryExpression>(loc, std::move(e));
        case tok::sin :
            return make_expression<SinUnaryExpression>(loc, std::move(e));
        case tok::log :
            return make_expression<LogUnaryExpression>(loc, std::move(e));
       default :
            std::cerr << yellow(token_string(op))
                      << " is not a valid unary operator"
                      << std::endl;;
            return nullptr;
    }
    return nullptr;
}

expression_ptr binary_expression( tok op,
                                  expression_ptr&& lhs,
                                  expression_ptr&& rhs
                                )
{
    return binary_expression(Location(), op, std::move(lhs), std::move(rhs));
}

expression_ptr binary_expression(Location loc,
                                 tok op,
                                 expression_ptr&& lhs,
                                 expression_ptr&& rhs
                                )
{
    switch(op) {
        case tok::eq     :
            return make_expression<AssignmentExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::plus   :
            return make_expression<AddBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::minus  :
            return make_expression<SubBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::times  :
            return make_expression<MulBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::divide :
            return make_expression<DivBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::pow    :
            return make_expression<PowBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok::lt     :
        case tok::lte    :
        case tok::gt     :
        case tok::gte    :
        case tok::EQ     :
            return make_expression<ConditionalExpression>(loc, op, std::move(lhs), std::move(rhs));
        default         :
            std::cerr << yellow(token_string(op))
                      << " is not a valid binary operator"
                      << std::endl;
            return nullptr;
    }
    return nullptr;
}
