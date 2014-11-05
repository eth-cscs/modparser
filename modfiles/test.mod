NEURON  {
    SUFFIX Ca
    USEION ca READ eca WRITE ica
    RANGE gCabar, gCa, ica 
}

UNITS   {
    (S) = (siemens)
    (mV) = (millivolt)
    (mA) = (milliamp)
}
