#pragma once

#include <string>
#include <vector>

#include "blocks.h"

// wrapper around a .mod file
class Module {
public :
    Module(std::string const& fname);
    Module(std::vector<char> const& buffer);

    std::vector<char> const& buffer() const {
        return buffer_;
    }

    std::string const& name()  const {return fname_;}

    void               title(const std::string& t) {title_ = t;}
    std::string const& title() const          {return title_;}

    NeuronBlock &      neuron_block()       {return neuron_block_;}
    NeuronBlock const& neuron_block() const {return neuron_block_;}

    StateBlock &       state_block()        {return state_block_;}
    StateBlock const&  state_block()  const {return state_block_;}

    UnitsBlock &       units_block()        {return units_block_;}
    UnitsBlock const&  units_block()  const {return units_block_;}

    ParameterBlock &       parameter_block()        {return parameter_block_;}
    ParameterBlock const&  parameter_block()  const {return parameter_block_;}

    AssignedBlock &       assigned_block()        {return assigned_block_;}
    AssignedBlock const&  assigned_block()  const {return assigned_block_;}

    void neuron_block(NeuronBlock const &n) {neuron_block_ = n;}
    void state_block (StateBlock  const &s) {state_block_  = s;}
    void units_block (UnitsBlock  const &u) {units_block_  = u;}
    void parameter_block (ParameterBlock  const &p) {parameter_block_  = p;}
    void assigned_block (AssignedBlock  const &a) {assigned_block_  = a;}

    // access to the AST
    std::vector<Expression*>&      procedures();
    std::vector<Expression*>const& procedures() const;

    std::vector<Expression*>&      functions();
    std::vector<Expression*>const& functions() const;

    std::unordered_map<std::string, Symbol>&      symbols();
    std::unordered_map<std::string, Symbol>const& symbols() const;

    // error handling
    void error(std::string const& msg, Location loc);
    LStat status() const {
        return status_;
    }

    // perform semantic analysis
    void add_variables_to_symbols();
    bool semantic();
private :
    std::string title_;
    std::string fname_;
    std::vector<char> buffer_; // character buffer loaded from file

    // error handling
    std::string error_string_;
    LStat status_;

    // AST storage
    std::vector<Expression *> procedures_;
    std::vector<Expression *> functions_;

    // hash table for lookup of variable and call names
    std::unordered_map<std::string, Symbol> symbols_;

    // blocks
    NeuronBlock neuron_block_;
    StateBlock  state_block_;
    UnitsBlock  units_block_;
    ParameterBlock parameter_block_;
    AssignedBlock assigned_block_;
};
