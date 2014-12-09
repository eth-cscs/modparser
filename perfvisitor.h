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
    void visit(Expression *e) override {
        std::cout << colorize("here base", kGreen) << std::endl;
    }

    // we have to traverse the statements in a procedure
    void visit(ProcedureExpression *e) override {}

    ////////////////////////////////////////////////////
    // specializations for each type of unary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(NegUnaryExpression *e) override {
        std::cout << colorize("here unary", kGreen) << std::endl;
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
        std::cout << colorize("here exp", kGreen) << std::endl;
        if(e->expression() != nullptr) {
            e->expression()->accept(this);
        }
        flops.exp++;
    }
    void visit(LogUnaryExpression *e) override {
        if(e->expression() != nullptr) {
            e->expression()->accept(this);
        }
        flops.log++;
    }
    void visit(CosUnaryExpression *e) override {
        if(e->expression() != nullptr) {
            e->expression()->accept(this);
        }
        flops.cos++;
    }
    void visit(SinUnaryExpression *e) override {
        if(e->expression() != nullptr) {
            e->expression()->accept(this);
        }
        flops.sin++;
    }

    ////////////////////////////////////////////////////
    // specializations for each type of binary expression
    // leave UnaryExpression to assert false, to catch
    // any missed specializations
    ////////////////////////////////////////////////////
    void visit(AssignmentExpression *e) {
        if(e->rhs() != nullptr) {
            e->rhs()->accept(this);
        }
    }
    void visit(AddBinaryExpression *e)  {
        if(e->lhs() != nullptr) {
            e->lhs()->accept(this);
        }
        if(e->rhs() != nullptr) {
            e->rhs()->accept(this);
        }
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

