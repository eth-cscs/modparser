#include <fstream>
#include <iostream>

#include "module.h"

Module::Module(std::string const& fname)
: fname_(fname)
{
    // open the file at the end
    std::ifstream fid;
    fid.open(fname.c_str(), std::ios::binary | std::ios::ate);
    if(!fid.is_open()) { // return if no file opened
        return;
    }

    // determine size of file
    size_t size = fid.tellg();
    fid.seekg(0, std::ios::beg);

    // allocate space for storage and read
    buffer_.resize(size+1);
    fid.read(buffer_.data(), size);
    buffer_[size] = 0; // append \0 to terminate string
}

Module::Module(std::vector<char> const& buffer)
{
    buffer_ = buffer;

    // add \0 to end of buffer if not already present
    if(buffer_[buffer_.size()-1] != 0)
        buffer_.push_back(0);
}

