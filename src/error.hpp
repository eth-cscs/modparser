#pragma once

#include "location.hpp"

class compiler_exception : public std::exception {
public:
    compiler_exception(std::string m, Location location)
    :   message_(std::move(m)),
        location_(location)
    {}

    virtual const char* what() const throw() {
        return message_.c_str();
    }

    Location const& location() const {
        return location_;
    }

private:

    Location location_;
    std::string message_;
};

