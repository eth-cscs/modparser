NEURON  {
    SUFFIX NaTs2_t
    USEION na READ ena WRITE ina
    RANGE gNaTs2_tbar
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
}

PARAMETER   {
    gNaTs2_tbar = 0.00001 (S/cm2)
}

ASSIGNED    {
}

STATE   {
    m
    h
}

BREAKPOINT  {
    LOCAL gNaTs2_t
    SOLVE states METHOD cnexp
    gNaTs2_t = gNaTs2_tbar*m*m*m*h
    ina = gNaTs2_t*(v-ena)
}

INITIAL{
    LOCAL mAlpha, mBeta, mInf, hAlpha, hBeta, hInf, lv

    lv = v

    if(lv == -32){
        lv = lv+0.0001
    }

    mAlpha = mAlphaf(lv)
    mBeta = mBetaf(lv)
    mInf = mAlpha/(mAlpha+mBeta)
    m = mInf 

    if(lv == -60){
        lv = lv+0.0001
    }

    hAlpha = hAlphaf(lv)
    hBeta = hBetaf(lv)
    hInf = hAlpha/(hAlpha+hBeta)
    h = hInf

    v = lv
}

DERIVATIVE states   {
    LOCAL mAlpha, mBeta, mInf, mTau, hAlpha, hBeta, hInf, hTau, lv, qt

    qt = 2.3^((34-21)/10)

    lv = v

    if(lv == -32){
        lv = lv+0.0001
    }

    mAlpha = mAlphaf(lv)
    mBeta = mBetaf(lv)
    mInf = mAlpha/(mAlpha+mBeta)
    mTau = (1/(mAlpha+mBeta))/qt
    m' = (-m + mInf)/mTau

    if(lv == -60){
        lv = lv+0.0001
    }

    hAlpha = hAlphaf(lv)
    hBeta = hBetaf(lv)
    hInf = hAlpha/(hAlpha+hBeta)
    hTau = (1/(hAlpha+hBeta))/qt
    h' = (hInf-h)/hTau

    v = lv
}

FUNCTION mAlphaf(v) {
    mAlphaf = (0.182 * (v- -32))/(1-(exp(-(v- -32)/6)))
}

FUNCTION mBetaf(v) {
    mBetaf  = (0.124 * (-v -32))/(1-(exp(-(-v -32)/6)))
}

FUNCTION hAlphaf(v) {
    hAlphaf = (-0.015 * (v- -60))/(1-(exp((v- -60)/6)))
}

FUNCTION hBetaf(v) {
    hBetaf = (-0.015 * (-v -60))/(1-(exp((-v -60)/6)))
}
