NEURON  {
    SUFFIX SKv3_1
    USEION k READ ek WRITE ik
    RANGE gSKv3_1bar
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
}

PARAMETER {
    gSKv3_1bar = 0.00001 (S/cm2)
}

ASSIGNED {
}

STATE {
    m
}

BREAKPOINT  {
    LOCAL gSKv3_1
    CONDUCTANCE gSKv3_1 USEION k

    SOLVE states METHOD cnexp
    gSKv3_1 = gSKv3_1bar*m
    ik = gSKv3_1*(v-ek)
}

DERIVATIVE states   {
    m' = (mInf(v)-m)/mTau(v)
}

INITIAL{
    m = mInf(v)
}

FUNCTION mInf(voltage) {
    mInf =  1/(1+exp((voltage-18.7)/-9.7))
}

FUNCTION mTau(voltage) {
    mTau =  0.2*20.000/(1+exp(((voltage -(-46.560))/(-44.140))))
}

