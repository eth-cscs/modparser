#pragma once

/// indicate how a variable is accessed:
///      read, written, or both
/// the distinction between write only and read only is required because
/// if an external variable is to be written/updated, then it does not have
/// to be loaded before applying a kernel.
enum accessKind {
    k_read,
    k_write,
    k_readwrite
};

///
/// describes the scope of a variable
///
enum visibilityKind {
    k_local_visibility,
    k_global_visibility
};

///
/// describes the scope of a variable
///
enum countabilityKind {
    k_range,
    k_scalar
};

/// the linkage of a variable indicates whether the value/values of the variable are defined inside or outside of the module.
enum linkageKind {
    k_local_link,
    k_extern_link
};

enum ionKind {
    k_ion_Ca,       ///< calcium ion
    k_ion_Na,       ///< sodium ion
    k_ion_K,        ///< potassium ion
    k_ion_other     ///< other, user-defined ion
};

///
/// base class that defines a variable
///
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

    virtual bool is_ion()   const  {return false;};
    virtual bool is_range() const = 0;
    virtual bool is_state() const = 0;

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

///
/// a range variable
///
class RangeVariable  : public Variable {
public:
    RangeVariable(
        std::string const& name,
        accessKind      access      = k_readwrite,
        visibilityKind  visibility  = k_global_visibility,
        linkageKind     linkage     = k_local_link
    )
        : Variable(name, access, visibility, linkage)
    {}

    bool is_range() const override { return true;};

    // these need to be fixed
    bool is_state() const override { return true;};
};

///
/// a scalar variable
///
class ScalarVariable : public Variable {
public:
    ScalarVariable(
        std::string const& name,
        accessKind      access      = k_readwrite,
        visibilityKind  visibility  = k_global_visibility,
        linkageKind     linkage     = k_local_link
    )
        : Variable(name, access, visibility, linkage)
    {}

    bool is_range() const override { return false;};

    // these need to be fixed
    bool is_state() const override { return true;};
};

///
/// a variable associated with an ion channel
///
class IonVariable    : public Variable {
public:
    IonVariable(
        std::string const& name,
        ionKind ion_kind,
        accessKind      access      = k_readwrite,
        visibilityKind  visibility  = k_global_visibility,
        linkageKind     linkage     = k_local_link
    )
        : Variable(name, access, visibility, linkage),
          kind_(ion_kind)
    {}

    bool is_range() const override { return true; }
    bool is_ion()   const override { return true; }

    ionKind ion_kind() const { return kind_; }

    // these need to be fixed
    bool is_state() const override { return true;};
private:
    ionKind kind_;
};

