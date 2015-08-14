:Comment :
:Reference : :      Kole,Hallermann,and Stuart, J. Neurosci. 2006

NEURON  {
    SUFFIX Ih
    NONSPECIFIC_CURRENT ihcn
    RANGE gIhbar
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
}

PARAMETER   {
    gIhbar = 0.00001 (S/cm2) 
    ehcn =  -45.0 (mV)
}

ASSIGNED    {
    v   (mV)
    :remove this from assigned, because it is local in BREAKPOINT
    :gIh (S/cm2)
}

STATE   { 
    m
}

BREAKPOINT  {
    LOCAL gIh
    CONDUCTANCE gIh
    SOLVE states METHOD cnexp
    gIh = gIhbar*m
    ihcn = gIh*(v-ehcn)
}

DERIVATIVE states   {
    LOCAL mAlpha, mBeta, mInf, mTau, lv

    :lv = v
    : this is not the sort of optimization we should be making
    : in user code: get the compiler do it.
    : this sort of thing just makes the big ball of mud even muddier
    :if(lv == -154.9){
    :    lv = lv + 0.0001
    :    v = lv
    :}

    mAlpha = mAlphaf(v)
    mBeta = mBetaf(v)
    mInf = mAlpha/(mAlpha + mBeta)
    mTau = 1/(mAlpha + mBeta)

    m' = (mInf-m)/mTau
}

INITIAL{

    LOCAL mAlpha, mBeta, mInf, lv

    :lv = v
    :if(lv == -154.9){
    :    lv = lv + 0.0001
    :    v = lv
    :}

    mAlpha = mAlphaf(v)
    mBeta = mBetaf(v)
    mInf = mAlpha/(mAlpha + mBeta)
    m = mInf
}

FUNCTION mAlphaf(v) {
    mAlphaf =  0.001*6.43*(v+154.9)/(exp((v+154.9)/11.9)-1)
}

FUNCTION mBetaf(v) {
    mBetaf  =  0.001*193*exp(v/33.1)
}
