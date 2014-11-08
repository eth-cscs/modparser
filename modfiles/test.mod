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

: TODO handle the - signs below
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
