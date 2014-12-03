#include "visitor.h"

struct FlopAccumulator {
    size_t add=0;
    size_t sub=0;
    size_t mul=0;
    size_t div=0;
    size_t exp=0;
    size_t sin=0;
    size_t cos=0;
    size_t log=0;

    size_t raw_flops() {
        return sub + add + mul + div;
    }

    void reset() {
        add = sub = mul = div = exp = sin = cos = log = 0;
    }
}

class FlopVisitor : public Visitor {
public:
    void visit(Expression *e) {}

    // we have to traverse the statements in a procedure
    void visit(ProcedureExpression *e);

    ////////////////////////////////////////////////////
    // specializations for each type of unary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(NegUnaryExpression *e) override {
        // this is a simplification
        // we would have to perform analysis of parent nodes to ensure that
        // the negation actually translates into an operation
        //  :: x - - x      // not counted
        //  :: x * -exp(3)  // should be counted
        //  :: x / -exp(3)  // should be counted
        //  :: x / - -exp(3)// should be counted only once
        flops.sub++;
    }
    void visit(ExpUnaryExpression *e) override {
        flops.exp++;
    }
    void visit(LogUnaryExpression *e) override {
        flops.log++;
    }
    void visit(CosUnaryExpression *e) override {
        flops.cos++;
    }
    void visit(SinUnaryExpression *e) override {
        flops.sin++;
    }

    ////////////////////////////////////////////////////
    // specializations for each type of binary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(AssignmentExpression *e) {} // do nothing for assignment
    void visit(AddBinaryExpression *e)  {
        flops.add++;
    }
    void visit(SubBinaryExpression *e)  {
        flops.sub++;
    }
    void visit(MulBinaryExpression *e)  {
        flops.mul++;
    }
    void visit(DivBinaryExpression *e)  {
        flops.div++;
    }
    void visit(PowBinaryExpression *e)  {
        flops.pow++;
    }

    FlopAccumulator flops;
};

