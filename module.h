#pragma once

#include <string>

//#include "blocks.h"

// wrapper around a .mod file
class Module {
public :
    Module(std::string const& fname);

    std::vector<char> const& buffer() const {
        return buffer_;
    }

private :
    std::string fname_;
    std::vector<char> buffer_;
};
