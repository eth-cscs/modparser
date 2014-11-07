#pragma once

#include <string>
#include <vector>

#include "lexer.h"

// describes a relationship with an ion channel
struct IonDep {
    std::string name;               // name of ion channel
    std::vector<std::string> read;  // name of channels parameters to write
    std::vector<std::string> write; // name of channels parameters to read
};

// information stored in a NEURON {} block in mod file
struct NeuronBlock {
    bool threadsafe = false;
    std::string suffix;
    std::vector<IonDep> ions;
    std::vector<std::string> ranges;
    std::vector<std::string> globals;
};

// information stored in a NEURON {} block in mod file
struct StateBlock {
    std::vector<std::string> state_variables;
};

// information stored in a NEURON {} block in mod file
struct UnitsBlock {
    typedef std::vector<Token> unit_type;
    typedef std::pair<unit_type, unit_type> units_pair;
    std::vector<units_pair> unit_aliases;
};

////////////////////////////////////////////////
// helpers for pretty printing block information
////////////////////////////////////////////////
template <typename T>
static std::ostream& operator<< (std::ostream& os, std::vector<T> const& V); // forward declaration

static std::ostream& operator<< (std::ostream& os, Token const& t) {
    return os << "(" << t.name << ", " << t.name << ")";
}

static std::ostream& operator<< (std::ostream& os, UnitsBlock::units_pair const& p) {
    return os << "(" << p.first << ", " << p.second << ")";
}

template <typename T>
static std::ostream& operator<< (std::ostream& os, std::vector<T> const& V) {
    os << "[";
    for(auto const& v: V)
        os << " " << v;
    return os << " ]";
}

static std::ostream& operator<< (std::ostream& os, IonDep const& I) {
    return os << "{" << I.name << ", read " << I.read << " write " << I.write << "}";
}

static std::ostream& operator<< (std::ostream& os, NeuronBlock const& N) {
    os << "NeuronBlock"     << std::endl;
    os << "  name       : " << N.suffix  << std::endl;
    os << "  threadsafe : " << (N.threadsafe ? "yes" : "no") << std::endl;
    os << "  ranges     : " << N.ranges  << std::endl;
    os << "  globals    : " << N.globals << std::endl;
    os << "  ions       : " << N.ions    << std::endl;

    return os;
}

static std::ostream& operator<< (std::ostream& os, StateBlock const& B) {
    os << "StateBlock"      << std::endl;
    return os << "  variables  : " << B.state_variables << std::endl;

}

static std::ostream& operator<< (std::ostream& os, UnitsBlock const& U) {
    os << "UnitsBlock"       << std::endl;
    os << "  aliases    : "  << U.unit_aliases << std::endl;

    return os;
}

