#pragma once

#include <sstream>

// is thing in list?
template <typename T, int N>
bool is_in(T thing, const T (&list)[N]) {
    for(auto item : list) {
        if(thing==item) {
            return true;
        }
    }
    return false;
}

static std::string pprintf(const char *s) {
    std::string errstring;
    while(*s) {
        if(*s == '%' && s[1]!='%') {
            // instead of throwing an exception, replace with ??
            //throw std::runtime_error("pprintf: the number of arguments did not match the format ");
            errstring += "<?>";
        }
        else {
            errstring += *s;
        }
        ++s;
    }
    return errstring;
}

// variadic printf for easy error messages
template <typename T, typename ... Args>
std::string pprintf(const char *s, T value, Args... args) {
    std::string errstring;
    while(*s) {
        if(*s == '%' && s[1]!='%') {
            std::stringstream str;
            str << value;
            errstring += str.str();
            errstring += pprintf(++s, args...);
            return errstring;
        }
        else {
            errstring += *s;
            ++s;
        }
    }
    return errstring;
}

enum stringColor {kWhite, kRed, kGreen, kBlue, kYellow};
static std::string colorize(std::string const& s, stringColor c) {
    switch(c) {
        case kWhite :
            return "\033[1;0m"  + s + "\033[0m";
        case kRed   :
            return "\033[1;31m" + s + "\033[0m";
        case kGreen :
            return "\033[1;32m" + s + "\033[0m";
        case kBlue  :
            return "\033[1;34m" + s + "\033[0m";
        case kYellow   :
            return "\033[1;33m" + s + "\033[0m";
    }
}
