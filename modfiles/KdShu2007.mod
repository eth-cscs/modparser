TITLE K-D
: K-D current for prefrontal cortical neuron ------Yuguo Yu  2007

NEURON {
    THREADSAFE
    SUFFIX KdShu2007
    USEION k WRITE ik
    RANGE  gkbar, ik, ek
    GLOBAL minf, mtau, hinf, htau
}

PARAMETER {
    gkbar   = 0.1  (mho/cm2)
    celsius
    ek      = -100 (mV)    : must be explicitly def. in hoc
    v       (mV)
    vhalfm  =-43   (mV)
    km      =8
    vhalfh  =-67   (mV)
    kh      =7.3
    q10     =2.3
}

UNITS {
    (mA) = (milliamp)
    (mV) = (millivolt)
    (pS) = (picosiemens)
    (um) = (micron)
}

: we can 
ASSIGNED {
    : would an approach for these be to make the user supply a function that describes how to
    : compute these as a function of actual mathematical state (m, h, v, celsius, area)
    : note that it would be good to differentiate between "global state" that varies constantly like
    : v, and that which "may but almost certainly won't" like celcius
    ik      (mA/cm2)
    minf    mtau (ms)
    hinf    htau (ms)
}

STATE {
    m h
}

BREAKPOINT {
    SOLVE states METHOD cnexp
    : the solve expression above is required
    : to ensure that values required to compute current contribution ik
    : below, i.e these are (m, h)
    : the states SOLVE is to ensure that m, h are currect at model time t

    ik = gkbar * m*h*(v-ek)
}

INITIAL {
    trates(v)
    m=minf
    h=hinf
}

DERIVATIVE states {
    trates(v)
    m' = (minf-m)/mtau
    h' = (hinf-h)/htau

    :~ mc <-> m (alpha_m(v), beta_m(v))
}

PROCEDURE trates(v) {
    LOCAL qt
    qt=q10^((celsius-22)/10)
    minf=1-1/(1+exp((v-vhalfm)/km))
    hinf=1/(1+exp((v-vhalfh)/kh))

    mtau = 0.6
    htau = 1500
}

