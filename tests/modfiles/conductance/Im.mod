NEURON  {
    SUFFIX Im
    USEION k READ ek WRITE ik
    RANGE gImbar
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
}

PARAMETER   {
    gImbar = 0.00001 (S/cm2) 
}

ASSIGNED    {
}

STATE {
    m
}

BREAKPOINT  {
    LOCAL gIm
    SOLVE states METHOD cnexp
    gIm = gImbar*m
    ik = gIm*(v-ek)
}

DERIVATIVE states   {
    LOCAL mAlpha, mBeta, mInf, mTau, qt, lv

    qt = 2.3^((34-21)/10)
    lv = v

    mAlpha = mAlphaf(lv)
    mBeta = mBetaf(lv)
    mInf = mAlpha/(mAlpha + mBeta)
    mTau = (1/(mAlpha + mBeta))/qt

    m' = (mInf-m)/mTau
}

INITIAL{
    LOCAL mAlpha, mBeta, mInf, lv

    lv = v

    mAlpha = mAlphaf(lv)
    mBeta = mBetaf(lv)
    mInf = mAlpha/(mAlpha + mBeta)

    m = mInf
}

FUNCTION mAlphaf(v) {
    mAlphaf = 3.3e-3*exp(2.5*0.04*(v - -35))
}

FUNCTION mBetaf(v) {
    mBetaf = 3.3e-3*exp(-2.5*0.04*(v - -35)) 
}
