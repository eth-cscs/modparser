#pragma once

#include <string>
#include <vector>

// describes a relationship with an ion channel
struct IonDep {
    std::string name;          // name of ion channel
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

static std::ostream& operator<< (std::ostream& os, NeuronBlock const& N) {
    os << "NeuronBlock::" << std::endl;
    os << "  name       : " << N.suffix << std::endl;
    os << "  threadsafe : " << (N.threadsafe ? "yes" : "no") << std::endl;
    os << "  ranges     : ";
    for(auto const& r : N.ranges)
        os << r << " ";
    os << std::endl;

    return os;
}
