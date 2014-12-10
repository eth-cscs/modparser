#pragma once

#include <cstdio>

#include "visitor.h"

struct FlopAccumulator {
    int add=0;
    int sub=0;
    int mul=0;
    int div=0;
    int exp=0;
    int sin=0;
    int cos=0;
    int log=0;
    int pow=0;

    void reset() {
        add = sub = mul = div = exp = sin = cos = log = 0;
    }
};

static std::ostream& operator << (std::ostream& os, FlopAccumulator const& f) {
    char buffer[512];
    snprintf(buffer,
             512,
             "   add   sub   mul   div   exp   sin   cos   log   pow\n%6d%6d%6d%6d%6d%6d%6d%6d%6d",
             f.add, f.sub, f.mul, f.div, f.exp, f.sin, f.cos, f.log, f.pow);
    return os << buffer;
}

class FlopVisitor : public Visitor {
public:
    void visit(Expression *e) override {}

    // traverse the statements in a procedure
    void visit(ProcedureExpression *e) override {
        for(auto expression : e->body()) {
            expression->accept(this);
        }
    }

    ////////////////////////////////////////////////////
    // specializations for each type of unary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(NegUnaryExpression *e) override {
        std::cout << colorize("unary neg", kGreen) << std::endl;
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
        // there is something catasrophically wrong if there is a nullptr
        // for the expression in an unary expression, so assert instead of
        // simply testing with if
        assert(e->expression());
        e->expression()->accept(this);
        flops.exp++;
    }
    void visit(LogUnaryExpression *e) override {
        assert(e->expression());
        e->expression()->accept(this);
        flops.log++;
    }
    void visit(CosUnaryExpression *e) override {
        assert(e->expression());
        e->expression()->accept(this);
        flops.cos++;
    }
    void visit(SinUnaryExpression *e) override {
        assert(e->expression());
        e->expression()->accept(this);
        flops.sin++;
    }

    ////////////////////////////////////////////////////
    // specializations for each type of binary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(BinaryExpression *e) {
        // there must be a specialization of the flops counter for every type
        // of binary expression: if we get here there has been an attempt to
        // visit a binary expression for which no visitor is implemented
        assert(false);
    }
    void visit(AssignmentExpression *e) {
        //std::cout << colorize("bin ass", kGreen) << std::endl;

        // there is something catasrophically wrong if there is a nullptr on
        // either side of a binary expression, so assert instead of simply
        // testing with if
        assert(e->rhs());
        e->rhs()->accept(this);
    }
    void visit(AddBinaryExpression *e)  {
        //std::cout << colorize("bin add", kGreen) << std::endl;

        assert(e->rhs()); assert(e->lhs());
        e->lhs()->accept(this);
        e->rhs()->accept(this);
        flops.add++;
    }
    void visit(SubBinaryExpression *e)  {
        //std::cout << colorize("bin sub", kGreen) << std::endl;

        assert(e->rhs()); assert(e->lhs());
        e->lhs()->accept(this);
        e->rhs()->accept(this);
        flops.sub++;
    }
    void visit(MulBinaryExpression *e)  {
        //std::cout << colorize("bin mul", kGreen) << std::endl;

        assert(e->rhs()); assert(e->lhs());
        e->lhs()->accept(this);
        e->rhs()->accept(this);
        flops.mul++;
    }
    void visit(DivBinaryExpression *e)  {
        //std::cout << colorize("bin div", kGreen) << std::endl;

        assert(e->rhs()); assert(e->lhs());
        e->lhs()->accept(this);
        e->rhs()->accept(this);
        flops.div++;
    }
    void visit(PowBinaryExpression *e)  {
        //std::cout << colorize("bin pow", kGreen) << std::endl;

        assert(e->rhs()); assert(e->lhs());
        e->lhs()->accept(this);
        e->rhs()->accept(this);
        flops.pow++;
    }

    FlopAccumulator flops;
};

