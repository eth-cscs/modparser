#include <cassert>
#include <cstring>

#include "expression.h"

std::string to_string(procedureKind k) {
    switch(k) {
        case k_proc             :
            return "PROCEDURE";
        case k_proc_initial     :
            return "INITIAL";
        case k_proc_net_receive :
            return "NET_RECEIVE";
        case k_proc_breakpoint  :
            return "BREAKPOINT";
        case k_proc_derivative  :
            return "DERIVATIVE";
    }
}

/*******************************************************************************
  Expression
*******************************************************************************/

void Expression::semantic(std::shared_ptr<Scope>) {
    error_ = true;
    error_string_ =
        pprintf("semantic() has not been implemented for this expression");
}

/*******************************************************************************
  IdentifierExpression
*******************************************************************************/

void IdentifierExpression::semantic(std::shared_ptr<Scope> scp) {
    scope_ = scp;

    Symbol s = scope_->find(name_);

    if(s.expression==nullptr) {
        error_ = true;
        error_string_ =
            pprintf("the variable '%' is undefined",
                    yellow(name_), location_);
        return;
    }
    if(s.kind == k_procedure || s.kind == k_function) {
        error_ = true;
        error_string_ =
            pprintf("the symbol '%' is a function/procedure, not a variable",
                    yellow(name_));
        return;
    }

    // save the symbol
    symbol_ = s;
}

VariableExpression* IdentifierExpression::variable() {
    // can we just look at symbol_.kind and lookup based on that?
    return symbol_.expression ? symbol_.expression->is_variable() : nullptr;
}

bool IdentifierExpression::is_lvalue() {
    // check for global variable that is writeable
    auto var = variable();
    if(var) return var->is_writeable();

    // else look for local symbol
    if(symbol_.kind == k_local ) return true;

    return false;
}

/*******************************************************************************
  LocalExpression
*******************************************************************************/

std::string LocalExpression::to_string() const {
    std::string str = blue("local");
    for(auto v : vars_) {
        str += " " + yellow(v.first);
    }
    return str;
}

bool LocalExpression::add_variable(Token tok) {
    if(vars_.find(tok.name)!=vars_.end()) {
        error_ = true;
        error_string_ = "the variable '" + yellow(tok.name) + "' is defined more than once";
        return false;
    }

    vars_[tok.name] = tok;
    return true;
}

void LocalExpression::semantic(std::shared_ptr<Scope> scp) {
    scope_ = scp;

    // loop over the variables declared in this LOCAL statement
    for(auto &v : vars_) {
        auto &name = v.first;
        Symbol s = scope_->find(name);

        // First check that the variable is undefined
        // Note that we allow for local variables with the same name as
        // class scope variables (globals), in which case the local variable
        // name will be used for lookup
        if(s.expression==nullptr || (s.expression && s.kind==k_variable)) {
            symbols_.push_back( scope_->add_local_symbol(name, this) );
        }
        else {
            error_ = true;
            error_string_ =
                pprintf("the symbol '%' is already defined", yellow(name));
        }
    }
}

/*******************************************************************************
  VariableExpression
*******************************************************************************/

std::string VariableExpression::to_string() const {
    char name[17];
    snprintf(name, 17, "%-10s", name_.c_str());
    std::string
        s = colorize("variable",kBlue) + " " + colorize(name, kYellow) + "("
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
  CallExpression
*******************************************************************************/

std::string CallExpression::to_string() const {
    std::string str = colorize("call", kBlue) + " " + colorize(name_, kYellow) + " (";
    for(auto arg : args_)
        str += arg->to_string() + ", ";
    str += ")";

    return str;
}

void CallExpression::semantic(std::shared_ptr<Scope> scp) {
    scope_ = scp;

    // look up to see if symbol is defined
    Symbol s = scope_->find(name_);

    // either undefined or refers to a variable
    if(s.expression==0) {
        error_ = true;
        error_string_ = 
            pprintf("there is no function or procedure named '%' ",
                    colorize(name_, kYellow));
    }
    if(s.kind==k_local || s.kind==k_variable) {
        error_ = true;
        error_string_ = 
            pprintf("the symbol '%' refers to a variable, but it is being called like a function",
                    colorize(name_, kYellow));
    }

    // save the symbol
    symbol_ = s;

    // TODO: check that the number of arguments matches
    if( !error_ ) {
        // only analyze if the call was found
    }

    // perform semantic analysis on the arguments
    for(auto a : args_) {
        a->semantic(scp);
    }
}

/*******************************************************************************
  ProcedureExpression
*******************************************************************************/

std::string ProcedureExpression::to_string() const {
    std::string str = colorize("procedure", kBlue) + " " + colorize(name_, kYellow) + "\n";
    str += colorize("  special",kBlue) + " : " + ::to_string(kind_) + "\n";
    str += colorize("  args",kBlue) + "    : ";
    for(auto arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+colorize("body", kBlue)+" :";
    str += body_->to_string();

    return str;
}

void ProcedureExpression::semantic(Scope::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    assert(global_symbols.find(name_) != global_symbols.end());

    // create the scope for this procedure
    scope_ = std::make_shared<Scope>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto a : args_) {
        a->semantic(scope_);
    }

    // this loop could be used to then check the types of statements in the body
    for(auto e : *body_) {
        if(e->is_initial_block())
            error("INITIAL block not allowed inside "+::to_string(kind_)+" definition");
    }

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = global_symbols.find(name_)->second;
}

/*******************************************************************************
  InitialBlock
*******************************************************************************/

std::string InitialBlock::to_string() const {
    std::string str = green("[[initial");
    for(auto ex : body_) {
       str += "\n   " + ex->to_string();
    }
    str += green("\n  ]]");
    return str;
}

/*******************************************************************************
  NetReceiveExpression
*******************************************************************************/

void NetReceiveExpression::semantic(Scope::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    assert(global_symbols.find(name_) != global_symbols.end());

    // create the scope for this procedure
    scope_ = std::make_shared<Scope>(global_symbols);

    // add the argumemts to the list of local variables
    // TODO : this does not appear to be working
    for(auto a : args_) {
        a->semantic(scope_);
    }

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);
    // this loop could be used to then check the types of statements in the body
    for(auto e : *body_) {
        if(e->is_initial_block()) {
            if(initial_block_) {
                error("only one INITIAL block is permitted per NET_RECEIVE block");
            }
            initial_block_ = e->is_initial_block();
        }
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = global_symbols.find(name_)->second;
}

/*******************************************************************************
  FunctionExpression
*******************************************************************************/

std::string FunctionExpression::to_string() const {
    std::string str = colorize("function", kBlue) + " " + colorize(name_, kYellow) + "\n";
    str += colorize("  args",kBlue) + " : ";
    for(auto arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+colorize("body", kBlue)+" :";
    str += body_->to_string();

    return str;
}

void FunctionExpression::semantic(Scope::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    assert(global_symbols.find(name_) != global_symbols.end());

    // create the scope for this procedure
    scope_ = std::make_shared<Scope>(global_symbols);

    // add the argumemts to the list of local variables
    for(auto a : args_) {
        a->semantic(scope_);
    }

    // Add a variable that has the same name as the function,
    // which acts as a placeholder for the return value
    // Make its location correspond to that of the first line of the function,
    // for want of a better location
    auto return_var = new LocalExpression(body_->location(), name_);
    scope_->add_local_symbol(name_, return_var);

    // perform semantic analysis for each expression in the body
    body_->semantic(scope_);
    // this loop could be used to then check the types of statements in the body
    for(auto e : *body_) {
        if(e->is_initial_block()) error("INITIAL block not allowed inside FUNCTION definition");
    }

    // check that the last expression in the body was an assignment to
    // the return placeholder
    bool last_expr_is_assign = false;
    auto tail = body_->back()->is_assignment();
    if(tail) {
        // we know that the tail is an assignemnt expression so reinterpret away
        auto lhs = tail->lhs()->is_identifier();
        // use nullptr check followed by lazy name lookup
        if(lhs && lhs->name()==name_) {
            last_expr_is_assign = true;
        }
    }
    if(!last_expr_is_assign) {
        warning_ = true;
        warning_string_ = "the last expression in function '"
                        + yellow(name_)
                        + "' does not set the return value";
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = global_symbols.find(name_)->second;
}

/*******************************************************************************
  UnaryExpression
*******************************************************************************/
void UnaryExpression::semantic(std::shared_ptr<Scope> scp) {
    expression_->semantic(scp);

    if(expression_->is_procedure_call()) {
        error_ = true;
        error_string_ = "a procedure call can't be part of an expression";
    }
}

void UnaryExpression::replace_expression(Expression* other) {
    expression_ = other;
}

/*******************************************************************************
  BinaryExpression
*******************************************************************************/
void BinaryExpression::semantic(std::shared_ptr<Scope> scp) {
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    if(rhs_->is_procedure_call() || lhs_->is_procedure_call()) {
        error_ = true;
        error_string_ = "procedure calls can't be made in an expression";
    }
}

void BinaryExpression::replace_lhs(Expression* other) {
    lhs_ = other;
}

void BinaryExpression::replace_rhs(Expression* other) {
    rhs_ = other;
}

std::string BinaryExpression::to_string() const {
    return pprintf("(% % %)", blue(token_string(op_)), lhs_->to_string(), rhs_->to_string());
    //return pprintf("(% % %)", lhs_->to_string(), blue(token_string(op_)), rhs_->to_string());
}

/*******************************************************************************
  AssignmentExpression
*******************************************************************************/

void AssignmentExpression::semantic(std::shared_ptr<Scope> scp) {
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

void SolveExpression::semantic(std::shared_ptr<Scope> scp) {
    auto e = scp->find(name_).expression;
    auto proc = e ? e->is_procedure() : nullptr;

    // this is optimistic: it simply looks for a procedure,
    // it should also evaluate the procedure to see whether it contains the derivatives
    if(proc) {
        // another test like :
        //if(proc->is_derivative_block())
        procedure_ = proc;
    }
    else {
        error_ = true;
        error_string_ = "'" + yellow(name_)
            + "' is not a valid procedure name for computing the derivatives in a SOLVE statement";
    }
}

/*******************************************************************************
  BlockExpression
*******************************************************************************/

std::string BlockExpression::to_string() const {
    std::string str = green("[[");
    for(auto ex : body_) {
       str += "\n   " + ex->to_string();
    }
    str += green("\n  ]]");
    return str;
}

void BlockExpression::semantic(std::shared_ptr<Scope> scp) {
    scope_ = scp;
    for(auto e : body_) {
        e->semantic(scope_);
    }
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

void IfExpression::semantic(std::shared_ptr<Scope> scp) {
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
void NumberExpression::accept(Visitor *v) {
    v->visit(this);
}
void LocalExpression::accept(Visitor *v) {
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


Expression* unary_expression( Location loc,
                              TOK op,
                              Expression* e) {
    switch(op) {
        case tok_minus :
            return new NegUnaryExpression(loc, e);
        case tok_exp :
            return new ExpUnaryExpression(loc, e);
        case tok_cos :
            return new CosUnaryExpression(loc, e);
        case tok_sin :
            return new SinUnaryExpression(loc, e);
        case tok_log :
            return new LogUnaryExpression(loc, e);
       default :
            std::cerr << yellow(token_string(op))
                      << " is not a valid unary operator"
                      << std::endl;;
            return nullptr;
    }
    assert(false); // something catastrophic went wrong
    return nullptr;
}

Expression* binary_expression( Location loc,
                               TOK op,
                               Expression* lhs,
                               Expression* rhs) {
    switch(op) {
        case tok_eq     :
            return new AssignmentExpression(loc, lhs, rhs);
        case tok_plus   :
            return new AddBinaryExpression(loc, lhs, rhs);
        case tok_minus  :
            return new SubBinaryExpression(loc, lhs, rhs);
        case tok_times  :
            return new MulBinaryExpression(loc, lhs, rhs);
        case tok_divide :
            return new DivBinaryExpression(loc, lhs, rhs);
        case tok_pow    :
            return new PowBinaryExpression(loc, lhs, rhs);
        case tok_lt     :
        case tok_lte    :
        case tok_gt     :
        case tok_gte    :
        case tok_EQ     :
            return new ConditionalExpression(loc, op, lhs, rhs);
        default         :
            std::cerr << yellow(token_string(op))
                      << " is not a valid binary operator"
                      << std::endl;
            return nullptr;
    }

    assert(false); // something catastrophic went wrong
    return nullptr;
}
