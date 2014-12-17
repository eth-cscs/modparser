NEURON {
    THREADSAFE
    POINT_PROCESS ProbAMPANMDA_EMS
    RANGE tau_r_AMPA, tau_d_AMPA, tau_r_NMDA, tau_d_NMDA
    RANGE Use, u, Dep, Fac, u0, mg, Rstate, tsyn_fac, u
    RANGE i, i_AMPA, i_NMDA, g_AMPA, g_NMDA, g, e, NMDA_ratio
    RANGE A_AMPA_step, B_AMPA_step, A_NMDA_step, B_NMDA_step
    :NONSPECIFIC_CURRENT i
    :BBCOREPOINTER rng
    RANGE synapseID, verboseLevel
}

PARAMETER {
    tau_r_AMPA = 0.2   (ms)  : dual-exponential conductance profile
    tau_d_AMPA = 1.7    (ms)  : IMPORTANT: tau_r < tau_d
    tau_r_NMDA = 0.29   (ms) : dual-exponential conductance profile
    tau_d_NMDA = 43     (ms) : IMPORTANT: tau_r < tau_d
    Use = 1.0   (1)
    Dep = 100   (ms)  : relaxation time constant from depression
    Fac = 10   (ms)  :  relaxation time constant from facilitation
    e = 0     (mV)  : AMPA and NMDA reversal potential
    mg = 1   (mM)  : initial concentration of mg2+
    mggate
    gmax = .001 (uS) : weight conversion factor (from nS to uS)
    u0 = 0 :initial value of u, which is the running value of release probability
    synapseID = 0
    verboseLevel = 0
    NMDA_ratio = 0.71 (1) : The ratio of NMDA to AMPA
}

:VERBATIM
:#include "nrnran123.h"
:ENDVERBATIM

ASSIGNED {
    v (mV)
    i (nA)
    i_AMPA (nA)
    i_NMDA (nA)
    g_AMPA (uS)
    g_NMDA (uS)
    g (uS)
    factor_AMPA
    factor_NMDA
    A_AMPA_step
    B_AMPA_step
    A_NMDA_step
    B_NMDA_step
    rng
    Rstate (1)      : recovered state {0=unrecovered, 1=recovered}
    tsyn_fac (ms)   : the time of the last spike
    u (1)           : running release probability
}

STATE {
    A_AMPA
    B_AMPA
    A_NMDA
    B_NMDA
}

INITIAL{
    : todo: add parsing of multiple locals on the same line
    :LOCAL tp_AMPA, tp_NMDA
    LOCAL tp_AMPA
    LOCAL tp_NMDA

:    VERBATIM
:      if (_p_rng)
:      {
:        nrnran123_setseq((nrnran123_State*)_p_rng, 0, 0);
:      }
:    ENDVERBATIM

    Rstate=1
    tsyn_fac=0
    u=u0

    A_AMPA = 0
    B_AMPA = 0

    A_NMDA = 0
    B_NMDA = 0

    tp_AMPA = (tau_r_AMPA*tau_d_AMPA) / (tau_d_AMPA-tau_r_AMPA) * log(tau_d_AMPA/tau_r_AMPA)
    tp_NMDA = (tau_r_NMDA*tau_d_NMDA) / (tau_d_NMDA-tau_r_NMDA) * log(tau_d_NMDA/tau_r_NMDA)

    factor_AMPA = -exp(-tp_AMPA/tau_r_AMPA)+exp(-tp_AMPA/tau_d_AMPA)
    factor_AMPA = 1/factor_AMPA

    factor_NMDA = -exp(-tp_NMDA/tau_r_NMDA)+exp(-tp_NMDA/tau_d_NMDA)
    factor_NMDA = 1/factor_NMDA

    A_AMPA_step = exp(dt*(( - 1.0 ) / tau_r_AMPA))
    B_AMPA_step = exp(dt*(( - 1.0 ) / tau_d_AMPA))
    A_NMDA_step = exp(dt*(( - 1.0 ) / tau_r_NMDA))
    B_NMDA_step = exp(dt*(( - 1.0 ) / tau_d_NMDA))
}

BREAKPOINT {
    : todo : handling of SOLVE with no METHOD
    :SOLVE state
    SOLVE state METHOD cnexp

    mggate = 1 / (1 + exp(0.062  * -(v)) * (mg / 3.57 ))

    g_AMPA = gmax*(B_AMPA-A_AMPA)
    g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate
    g      = g_AMPA + g_NMDA

    i_AMPA = g_AMPA*(v-e)
    i_NMDA = g_NMDA*(v-e)
    i      = i_AMPA + i_NMDA
}

: integration with a procedure: naughty
: has to be modified to work with DERIVATIVE block
PROCEDURE state() {
   : this need
   :A_AMPA' = -rate*A_AMPA
    A_AMPA = A_AMPA*A_AMPA_step
    B_AMPA = B_AMPA*B_AMPA_step
    A_NMDA = A_NMDA*A_NMDA_step
    B_NMDA = B_NMDA*B_NMDA_step
}

FUNCTION urand() {
    urand = 0.5
}

: the parameters, weight, weight_AMPA, weight_NDMA, etc are state that 
: is specific to the NET_RECEIVE

NET_RECEIVE (weight,weight_AMPA, weight_NMDA, Psurv, tsyn){
    LOCAL result
    weight_AMPA = weight
    weight_NMDA = weight * NMDA_ratio

    : todo
    : called at t==0
    :INITIAL{
    :    tsyn=t
    :}
    if (Fac > 0) {
        u = u*exp(-(t - tsyn_fac)/Fac)
    }
    :else {
        u = Use
    :}
    :if(Fac > 0){
        u = u + Use*(1-u)
    :}

    tsyn_fac = t
    :if (Rstate == 0) {
        Psurv = exp(-(t-tsyn)/Dep)
        result = urand()
        :if (result>Psurv) {
            Rstate = 1
        :}
        :else {
            tsyn = t
        :}
    :}

    :if (Rstate == 1) {
        result = urand()
        :if (result<u) {
            tsyn = t
            Rstate = 0
            A_AMPA = A_AMPA + weight_AMPA*factor_AMPA
            B_AMPA = B_AMPA + weight_AMPA*factor_AMPA
            A_NMDA = A_NMDA + weight_NMDA*factor_NMDA
            B_NMDA = B_NMDA + weight_NMDA*factor_NMDA
        :}
    :}
}

: hoc calls this (it isn't called anywhere else in the mod file)
: providing an external interface... why we need bbcore bbcoreread
:PROCEDURE setRNG() {
:    VERBATIM
:    {
:        #if !NRNBBCORE
:        nrnran123_State** pv = (nrnran123_State**)(&_p_rng);
:        if (*pv) {
:            nrnran123_deletestream(*pv);
:            *pv = (nrnran123_State*)0;
:        } 
:        if (ifarg(2)) {
:            *pv = nrnran123_newstream((uint32_t)*getarg(1), (uint32_t)*getarg(2));
:        }
:        #endif
:    }
:    ENDVERBATIM
:}

