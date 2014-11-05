#pragma once

#include <string>
#include <vector>

// describes a relationship with an ion channel
struct IonDep {
    std::string name;          // name of ion channel
    std::vector<std::string> read;  // name of channels parameters to write
    std::vector<std::string> write; // name of channels parameters to read
};

// for now just store a range as string
typedef std::string Range;

// information stored in a NEURON {} block in mod file
struct NeuronBlock {
    std::string suffix;
    std::vector<IonDep> ions;
    std::vector<Range> ranges;
};
