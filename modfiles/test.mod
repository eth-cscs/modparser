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
