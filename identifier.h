#pragma once

/// indicate how a variable is accessed
/// access is (read, written, or both)
/// the distinction between write only and read only is required because
/// if an external variable is to be written/updated, then it does not have
/// to be loaded before applying a kernel.
enum accessKind {
    k_read,
    k_write,
    k_readwrite
};

/// describes the scope of a variable
enum visibilityKind {
    k_local_visibility,
    k_global_visibility
};

/// describes the scope of a variable
enum rangeKind {
    k_range,
    k_scalar
};

/// the whether the variable value is defined inside or outside of the module.
enum linkageKind {
    k_local_link,
    k_extern_link
};

/// ion channel that the variable belongs to
enum ionKind {
    k_ion_none,     ///< not an ion variable
    k_ion_Ca,       ///< calcium ion
    k_ion_Na,       ///< sodium ion
    k_ion_K         ///< potassium ion
};

enum symbolKind {
    k_function,     ///< function call
    k_procedure,    ///< procedure call
    k_variable,     ///< variable at class scope
    k_local         ///< variable at local scope
};

static std::string yesno(bool val) {
    return std::string(val ? "yes" : "no");
};

// to_string functions
static std::string to_string(ionKind i) {
    switch(i) {
        case k_ion_none : return std::string("none");
        case k_ion_Ca   : return std::string("calcium");
        case k_ion_Na   : return std::string("sodium");
        case k_ion_K    : return std::string("potassium");
    }
    return std::string("<error : undefined ionKind>");
}

static std::string to_string(visibilityKind v) {
    switch(v) {
        case k_local_visibility : return std::string("local");
        case k_global_visibility: return std::string("global");
    }
    return std::string("<error : undefined visibilityKind>");
}

static std::string to_string(linkageKind v) {
    switch(v) {
        case k_local_link : return std::string("local");
        case k_extern_link: return std::string("external");
    }
    return std::string("<error : undefined visibilityKind>");
}

// ostream writers
static std::ostream& operator<< (std::ostream& os, ionKind i) {
    return os << to_string(i);
}

static std::ostream& operator<< (std::ostream& os, visibilityKind v) {
    return os << to_string(v);
}

static std::ostream& operator<< (std::ostream& os, linkageKind l) {
    return os << to_string(l);
}

///
/// base class for all identifier types (variables and functions)
///
/*
class Identifier {
public:
    Identifier(std::string const& name, Expression* e=nullptr)
    :   name_(name), expression_(e)
    {}

    virtual bool is_variable()  const = 0;
    virtual bool is_procedure() const = 0;
    virtual bool is_function()  const = 0;

    std::string const& name() const {return name_;}

    Expression* expression() { return expression_; }
    const Expression* expression() const { return expression_; }

protected:
    std::string name_;
    Expression *expression_;
};

///
/// base class for call types (functions and procedures)
///
class Call : public Identifier {
public:
    Call(std::string const& name, callKind type, Expression* e)
        : Identifier(name, e), type_(type)
    {
        // assert that the expression is actually an appropriate call expression
        if(is_procedure()) {
            assert(dynamic_cast<ProcedureExpression*>(e));
        }
        else {
            assert(dynamic_cast<FunctionExpression*>(e));
        }
    }

    bool is_variable() const override {return false;}
    bool is_procedure() const {return type_==k_procedure;}
    bool is_function()  const {return type_==k_function;}

protected:
    callKind type_;
};

/// definition of a variable
class Variable : public Identifier {
public:
    Variable( std::string const& name)
        :   Identifier(name) {}

    void set_access(accessKind a) {
        access_ = a;
    }
    void set_visibility(visibilityKind v) {
        visibility_ = v;
    }
    void set_linkage(linkageKind l) {
        linkage_ = l;
    }
    void set_range(rangeKind r) {
        range_kind_ = r;
    }
    void set_ion_channel(ionKind i) {
        ion_channel_ = i;
    }
    void set_state(bool s) {
        is_state_ = s;
    }

    accessKind access() const {
        return access_;
    }
    visibilityKind visibility() const {
        return visibility_;
    }
    linkageKind linkage() const {
        return linkage_;
    }
    ionKind ion_channel() const {
        return ion_channel_;
    }

    bool is_ion()   const {return ion_channel_ != k_ion_none;}
    bool is_state() const {return is_state_;}

    bool is_range() const {return range_kind_  == k_range;}
    bool is_scalar()const {return !is_range();}

    bool is_readable()  const {return access_==k_read  || access_==k_readwrite;}
    bool is_writeable() const {return access_==k_write || access_==k_readwrite;}

    bool is_variable() const override {return true;}
    bool is_function() const override {return false;}
    bool is_procedure() const override {return false;}
protected:
    Variable();

    bool           is_state_    = false;
    accessKind     access_      = k_readwrite;
    visibilityKind visibility_  = k_local_visibility;
    linkageKind    linkage_     = k_extern_link;
    rangeKind      range_kind_  = k_range;
    ionKind        ion_channel_ = k_ion_none;
};

static std::ostream& operator<< (std::ostream& os, Variable const& V) {
    char name[17];
    snprintf(name, 17, "%-10s", V.name().c_str());
    os << colorize("variable",kBlue) << " " << colorize(name, kYellow) << "(";
    os << colorize("write", V.is_writeable() ? kGreen : kRed)    << ", ";
    os << colorize("read", V.is_readable() ? kGreen : kRed)    << ", ";
    os << (V.is_range() ? "range" : "scalar")   << ", ";
    os << "ion" << colorize(to_string(V.ion_channel()), V.ion_channel()==k_ion_none ? kRed : kGreen)<< ", ";
    os << "vis " << V.visibility()       << ", ";
    os << "link "    << V.linkage()          << ", ";
    os << colorize("state", V.is_state() ? kGreen : kRed)    << ")";
    return os;
}
*/
