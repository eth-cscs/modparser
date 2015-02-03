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
    k_ion_nonspecific,  ///< nonspecific current
    k_ion_Ca,       ///< calcium ion
    k_ion_Na,       ///< sodium ion
    k_ion_K         ///< potassium ion
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
        case k_ion_nonspecific : return std::string("nonspecific");
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

