#include <cstring>

#include "expression.h"

std::string to_string(symbolKind k) {
    switch (k) {
        case k_symbol_variable:
            return std::string("variable");
        case k_symbol_indexed_variable:
            return std::string("indexed variable");
        case k_symbol_local:
            return std::string("local");
        case k_symbol_argument:
            return std::string("argument");
        case k_symbol_procedure:
            return std::string("procedure");
        case k_symbol_function:
            return std::string("function");
        default:
            return std::string("none");
    }
}


std::string to_string(procedureKind k) {
    switch(k) {
        case k_proc_normal             :
            return "procedure";
        case k_proc_api             :
            return "APIprocedure";
        case k_proc_initial     :
            return "initial";
        case k_proc_net_receive :
            return "net_receive";
        case k_proc_breakpoint  :
            return "breakpoint";
        case k_proc_derivative  :
            return "derivative";
        default :
            return "undefined";
    }
}

/*******************************************************************************
  Expression
*******************************************************************************/

void Expression::semantic(std::shared_ptr<scope_type>) {
    error_ = true;
    error_string_ = "semantic() has not been implemented for this expression";
}

expression_ptr Expression::clone() const {
    std::cerr << "clone() has not been implemented for " << this->to_string() << std::endl;
    assert(false);
    return nullptr;
}

/*******************************************************************************
  Symbol
*******************************************************************************/
std::string Symbol::to_string() const {
    return blue("Symbol") + " " + yellow(name_);
}

/*******************************************************************************
  IdentifierExpression
*******************************************************************************/

void IdentifierExpression::semantic(std::shared_ptr<scope_type> scp) {
    scope_ = scp;

    auto s = scope_->find(spelling_);

    if(s==nullptr) {
        error_ = true;
        error_string_ =
            pprintf("the variable '%' is undefined",
                    yellow(spelling_), location_);
        return;
    }
    if(s->kind() == k_symbol_procedure || s->kind() == k_symbol_function) {
        error_ = true;
        error_string_ =
            pprintf("the symbol '%' is a function/procedure, not a variable",
                    yellow(spelling_));
        return;
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
    if(symbol_->kind() == k_symbol_local || symbol_->kind() == k_symbol_argument ) return true;

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
        error_ = true;
        error_string_ = "the variable '" + yellow(tok.spelling) + "' is defined more than once";
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
        if(s==nullptr || (s && s->kind()==k_symbol_variable)) {
            scope_type::symbol_ptr symbol(new Symbol(location_, name, k_symbol_local));
            symbols_.push_back( scope_->add_local_symbol( name, std::move(symbol)) );
        }
        else {
            error_ = true;
            error_string_ =
                pprintf("the symbol '%' @ '%' is already defined", yellow(name), location());
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

    if(s==nullptr || (s && s->kind()==k_symbol_variable)) {
        scope_type::symbol_ptr symbol(new Symbol(location_, name_, k_symbol_argument));
        scope_->add_local_symbol(name_, std::move(symbol));
    }
    else {
        error_ = true;
        error_string_ =
            pprintf("the symbol '%' @ '%' is already defined", yellow(name_), location());
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
          + colorize("write", is_writeable() ? kGreen : kRed) + ", "
          + colorize("read", is_readable() ? kGreen : kRed)   + ", "
          + (is_range() ? "range" : "scalar")                 + ", "
          + "ion" + colorize(::to_string(ion_channel()),
                             (ion_channel()==k_ion_none) ? kRed : kGreen) + ", "
          + "vis "  + ::to_string(visibility()) + ", "
          + "link " + ::to_string(linkage())    + ", "
          + colorize("state", is_state() ? kGreen : kRed) + ")";
    return s;
}

/*******************************************************************************
  IndexedVariable
*******************************************************************************/

std::string IndexedVariable::to_string() const {
    char n[17];
    snprintf(n, 17, "%-10s", name().c_str());
    std::string
        s = blue("indexed") + "  " + yellow(n) + "("
          + (is_writeable() ? green("write") : red("write")) + ", "
          + (is_readable()  ? green("read")  : red("read")) + ", "
          + "ion" + colorize(::to_string(ion_channel()),
                             (ion_channel()==k_ion_none) ? kRed : kGreen) + ") ";
    return s;
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
        error_ = true;
        error_string_ = 
            pprintf("there is no function or procedure named '%' ",
                    yellow(spelling_));
    }
    if(s->kind()==k_symbol_local || s->kind()==k_symbol_variable) {
        error_ = true;
        error_string_ = 
            pprintf("the symbol '%' refers to a variable, but it is being called like a function",
                    yellow(spelling_));
    }

    // save the symbol
    symbol_ = s;

    // TODO: check that the number of arguments matches
    if( !error_ ) {
        // only analyze if the call was found
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
    assert(global_symbols.find(name()) != global_symbols.end());

    // create the scope for this procedure
    scope_ = std::make_shared<scope_type>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto& a : args_) {
        a->semantic(scope_);
    }

    // this loop could be used to then check the types of statements in the body
    // TODO : this could be making a mess
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

    str += blue("  loads ") + " : ";
    for(auto& in : in_) {
        str += namestr(in.local) + " <- ";
        str += namestr(in.external);
        str += ", ";
    }
    str += "\n";

    str += blue("  stores") + " : ";
    for(auto& out : out_) {
        str += namestr(out.local) + " -> ";
        str += namestr(out.external);
        str += ", ";
    }
    str += "\n";

    str += blue("  locals") + " : ";
    for(auto& var : scope_->locals()) {
        str += namestr(var.second.get());
        if(var.second->kind() == k_symbol_ghost) str += green("(ghost)");
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
    assert(global_symbols.find(name()) != global_symbols.end());

    // create the scope for this procedure
    scope_ = std::make_shared<scope_type>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto& a : args_) {
        a->semantic(scope_);
    }

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);
    // this loop could be used to then check the types of statements in the body
    // TODO : this could be making a mess
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
    assert(global_symbols.find(name()) != global_symbols.end());

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
        new Symbol(body_->location(), name(), k_symbol_local)
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
        warning_ = true;
        warning_string_ = "the last expression in function '"
                        + yellow(name())
                        + "' does not set the return value";
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
        error_ = true;
        error_string_ = "a procedure call can't be part of an expression";
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
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    if(rhs_->is_procedure_call() || lhs_->is_procedure_call()) {
        error_ = true;
        error_string_ = "procedure calls can't be made in an expression";
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
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    if(!lhs_->is_lvalue()) {
        // only flag an lvalue error if there was no error in the lhs expression
        // this ensures that we don't print redundant error messages when trying
        // to write to an undeclared variable
        if(!lhs_->has_error()) {
            error_ = true;
            error_string_ = "the left hand side of an assignment must be an lvalue";
        }
    }
    if(rhs_->is_procedure_call()) {
        error_ = true;
        error_string_ = "procedure calls can't be made in an expression";
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
    if(proc) {
        procedure_ = proc;
    }
    else {
        error_ = true;
        error_string_ = "'" + yellow(name_) + "' is not a valid procedure name"
                        " for computing the derivatives in a SOLVE statement";
    }
}

expression_ptr SolveExpression::clone() const {
    auto s = new SolveExpression(location_, name_, method_);
    s->procedure(procedure_);
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
        error_ = true;
        error_string_ = "not a valid conditional expression";
    }

    true_branch_->semantic(scp);

    if(false_branch_) {
        false_branch_->semantic(scp);
    }
}

#include "visitor.h"
/*
   Visitor hooks
*/
void Expression::accept(Visitor *v) {
    v->visit(this);
}
void Symbol::accept(Visitor *v) {
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

expression_ptr unary_expression( TOK op,
                                 expression_ptr&& e
                               )
{
    return unary_expression(op, std::move(e));
}

expression_ptr unary_expression( Location loc,
                                 TOK op,
                                 expression_ptr&& e
                               )
{
    switch(op) {
        case tok_minus :
            return make_expression<NegUnaryExpression>(loc, std::move(e));
        case tok_exp :
            return make_expression<ExpUnaryExpression>(loc, std::move(e));
        case tok_cos :
            return make_expression<CosUnaryExpression>(loc, std::move(e));
        case tok_sin :
            return make_expression<SinUnaryExpression>(loc, std::move(e));
        case tok_log :
            return make_expression<LogUnaryExpression>(loc, std::move(e));
       default :
            std::cerr << yellow(token_string(op))
                      << " is not a valid unary operator"
                      << std::endl;;
            return nullptr;
    }
    assert(false); // something catastrophic went wrong
    return nullptr;
}

expression_ptr binary_expression( TOK op,
                                  expression_ptr&& lhs,
                                  expression_ptr&& rhs
                                )
{
    return binary_expression(Location(), op, std::move(lhs), std::move(rhs));
}

expression_ptr binary_expression(Location loc,
                                 TOK op,
                                 expression_ptr&& lhs,
                                 expression_ptr&& rhs
                                )
{
    switch(op) {
        case tok_eq     :
            return make_expression<AssignmentExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_plus   :
            return make_expression<AddBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_minus  :
            return make_expression<SubBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_times  :
            return make_expression<MulBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_divide :
            return make_expression<DivBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_pow    :
            return make_expression<PowBinaryExpression>(loc, std::move(lhs), std::move(rhs));
        case tok_lt     :
        case tok_lte    :
        case tok_gt     :
        case tok_gte    :
        case tok_EQ     :
            return make_expression<ConditionalExpression>(loc, op, std::move(lhs), std::move(rhs));
        default         :
            std::cerr << yellow(token_string(op))
                      << " is not a valid binary operator"
                      << std::endl;
            return nullptr;
    }

    assert(false); // something catastrophic went wrong
    return nullptr;
}
