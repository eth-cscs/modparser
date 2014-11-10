: sample file

NEURON  {
    THREADSAFE
    SUFFIX KdShu2007
    USEION k WRITE ik, ig READ ip
    RANGE  gkbar, ik, ek
    GLOBAL minf, mtau, hinf, htau
}

STATE {
    h
    m r
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
    (molar) = (1/liter)
    (mM) = (milli/liter)
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

ASSIGNED {
    ik      (mA/cm2)
    minf    mtau (ms)
    hinf    htau (ms)
}

BREAKPOINT {
    SOLVE states METHOD cnexp
    ik = gkbar * m*h*(v-ek)
}

INITIAL {
    trates(v)
    m=minf
    h=hinf
}

: the 'states' in the definition is giving the derivative a name
: this name is then used in the SOLVE statement above
: should states be a procedure with special declaration syntax (takes no arguments by default)?

:DERIVATIVE states {
:    trates(v)
:    m' = (minf-m)/mtau
:    h' = (hinf-h)/htau
:}

:PROCEDURE trates(v) {
:    LOCAL qt
:    qt=q10^((celsius-22)/10)
:    minf=1-1/(1+exp((v-vhalfm)/km))
:    hinf=1/(1+exp((v-vhalfh)/kh))
:
:    mtau = 0.6
:    htau = 1500
:}

