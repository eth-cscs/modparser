#pragma once

// indicate how a variable is accessed
//      read, written, or both
// the distinction between write only and read only is required because
// if an external variable is to be written/updated, then it does not have
// to be loaded before applying a kernel.
enum accessKind {
    k_read,
    k_write,
    k_readwrite
};

enum visibilityKind {
    k_local_visibility,
    k_global_visibility
};

// the linkage of a variable indicates whether the value/values of the variable
// are defined inside or outside of the module.
enum linkageKind {
    k_local_link,
    k_extern_link
};

enum ionKind {
    k_ion_Ca,       // calcium
    k_ion_Na,       // sodium
    k_ion_K,        // potassium
    k_ion_other
};

class Variable {
public:
    Variable(
        std::string const& name,
        accessKind      access      = k_readwrite,
        visibilityKind  visibility  = k_global_visibility,
        linkageKind     linkage     = k_local_link
    )
    :   name_(name),
        access_(access),
        visibility_(visibility),
        linkage_(linkage)
    {}

    virtual bool is_ion()       {return false;};
    bool is_state();

    void access(accessKind         a) {access_     = a;}
    void visibility(visibilityKind v) {visibility_ = v;}
    void linkage(linkageKind       l) {linkage_    = l;}

    accessKind     access()     const {return access_;}
    visibilityKind visibility() const {return visibility_;}
    linkageKind    linkage()    const {return linkage_;}

    bool is_readable()  const {return access_==k_read  || access_==k_readwrite;}
    bool is_writeable() const {return access_==k_write || access_==k_readwrite;}

    std::string const& name() const { return name_;}
protected:
    Variable();

    accessKind    access_;
    visibilityKind visibility_;
    linkageKind   linkage_;

    std::string name_;
};

class RangeVariable  : public Variable {};
class ScalarVariable : public Variable {};

class IonVariable    : public Variable {
public:
    bool is_ion() override {return true;};
};

