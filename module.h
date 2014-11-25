#pragma once

#include <string>

#include "blocks.h"

// wrapper around a .mod file
class Module {
public :
    Module(std::string const& fname);

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

    void neuron_block(NeuronBlock const &n) {neuron_block_ = n;}
    void state_block (StateBlock  const &s) {state_block_  = s;}
    void units_block (UnitsBlock  const &u) {units_block_  = u;}
private :
    std::string title_;
    std::string fname_;
    std::vector<char> buffer_; // character buffer loaded from file

    // blocks
    NeuronBlock neuron_block_;
    StateBlock  state_block_;
    UnitsBlock  units_block_;
};
