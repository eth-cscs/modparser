:Reference : :      Adams et al. 1982 - M-currents and other potassium currents in bullfrog sympathetic neurones
:Comment: corrected rates using q10 = 2.3, target temperature 34, orginal 21

NEURON  {
    SUFFIX Im
    USEION k READ ek WRITE ik
    RANGE gImbar, gIm, ik
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
    v   (mV)
    ek  (mV)
    ik  (mA/cm2)
    gIm (S/cm2)
    mInf
    mTau
    mAlpha
    mBeta
}

STATE   { 
    m
}

BREAKPOINT  {
    SOLVE states METHOD cnexp
    gIm = gImbar*m
    ik = gIm*(v-ek)
}

DERIVATIVE states   {
    rates()
    m' = (mInf-m)/mTau
}

INITIAL{
    rates()
    m = mInf
}

PROCEDURE rates(){
  LOCAL qt
  qt = 2.3^((34-21)/10)

  mAlpha = 0.0033*exp(2.5*0.04*(v - -35))
  mBeta = 0.0033*exp(-2.5*0.04*(v - -35))
  mInf = mAlpha/(mAlpha + mBeta)
  mTau = (1/(mAlpha + mBeta))/qt
}
