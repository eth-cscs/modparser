#pragma once

#include "util.h"

// forward declarations
class Expression;
class VariableExpression;
class NumberExpression;
class LocalExpression;
class PrototypeExpression;
class CallExpression;
class ProcedureExpression;
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

/// visitor base class
/// The visitors for all AST nodes throw an assertion
/// by default, with node types calling the default visitor for a parent
/// For example, all BinaryExpression types call the visitor defined for
/// BinaryExpression, so by overriding just the base implementation, all of the
/// children get that implementation for free, which might be useful for some
/// use cases.
///
/// heavily inspired by the DMD D compiler : github.com/D-Programming-Language/dmd
class Visitor {
public:
    virtual void visit(Expression *e)           { assert(false);                }
    virtual void visit(VariableExpression *e)   { visit((Expression*) e);       }
    virtual void visit(NumberExpression *e)     { visit((Expression*) e);       }
    virtual void visit(LocalExpression *e)      { visit((Expression*) e);       }
    virtual void visit(PrototypeExpression *e)  { visit((Expression*) e);       }
    virtual void visit(CallExpression *e)       { visit((Expression*) e);       }
    virtual void visit(ProcedureExpression *e)  { visit((Expression*) e);       }
    virtual void visit(FunctionExpression *e)   { visit((Expression*) e);       }

    virtual void visit(UnaryExpression *e)      { assert(false);                }
    virtual void visit(NegUnaryExpression *e)   { visit((UnaryExpression*) e);  }
    virtual void visit(ExpUnaryExpression *e)   { visit((UnaryExpression*) e);  }
    virtual void visit(LogUnaryExpression *e)   { visit((UnaryExpression*) e);  }
    virtual void visit(CosUnaryExpression *e)   { visit((UnaryExpression*) e);  }
    virtual void visit(SinUnaryExpression *e)   { visit((UnaryExpression*) e);  }

    virtual void visit(BinaryExpression *e)     { assert(false);                }
    virtual void visit(AssignmentExpression *e) { visit((BinaryExpression*) e); }
    virtual void visit(AddBinaryExpression *e)  { visit((BinaryExpression*) e); }
    virtual void visit(SubBinaryExpression *e)  { visit((BinaryExpression*) e); }
    virtual void visit(MulBinaryExpression *e)  { visit((BinaryExpression*) e); }
    virtual void visit(DivBinaryExpression *e)  { visit((BinaryExpression*) e); }
    virtual void visit(PowBinaryExpression *e)  { visit((BinaryExpression*) e); }
};

/// A visitor with boolean state that can be used to terminate traversal
class StoppableVisitor : public Visitor {
public:
    StoppableVisitor() : stop_(false) {}
    bool stop() const {
        return stop_;
    }
    void set_stop(bool s) {
        stop_=s;
    }
private:
    bool stop_;
};

