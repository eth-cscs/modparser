TITLE Probabilistic AMPA and NMDA receptor with presynaptic short-term plasticity 

NEURON {
    THREADSAFE
        POINT_PROCESS ProbAMPANMDA_EMS
        RANGE tau_r_AMPA, tau_d_AMPA, tau_r_NMDA, tau_d_NMDA
        RANGE Use, u, Dep, Fac, u0, mg, Rstate, tsyn_fac, u
        RANGE i, i_AMPA, i_NMDA, g_AMPA, g_NMDA, g, e, NMDA_ratio
        NONSPECIFIC_CURRENT i
        :POINTER rng
        RANGE synapseID, verboseLevel
}

PARAMETER {
    tau_r_AMPA = 0.2   (ms)
    tau_d_AMPA = 1.7    (ms)
    tau_r_NMDA = 0.29   (ms)
    tau_d_NMDA = 43     (ms)
    Use = 1.0   (1)
    Dep = 100   (ms)
    Fac = 10   (ms)
    e = 0     (mV)
    mg = 1   (mM)
    gmax = .001 (uS)
    u0 = 0
    synapseID = 0
    verboseLevel = 0
    NMDA_ratio = 0.71 (1)
}

ASSIGNED {
    mggate
    v (mV)
    i (nA)
    i_AMPA (nA)
    i_NMDA (nA)
    g_AMPA (uS)
    g_NMDA (uS)
    g (uS)
    factor_AMPA
    factor_NMDA
    rng
    Rstate (1)
    tsyn_fac (ms)
    u (1)
}

STATE {
    A_AMPA
    B_AMPA
    A_NMDA
    B_NMDA
}

INITIAL{
    LOCAL tp_AMPA, tp_NMDA

    Rstate=1
    tsyn_fac=0
    u=u0

    A_AMPA = 0
    B_AMPA = 0

    A_NMDA = 0
    B_NMDA = 0

    tp_AMPA = (tau_r_AMPA*tau_d_AMPA)/(tau_d_AMPA-tau_r_AMPA)*log(tau_d_AMPA/tau_r_AMPA)
    tp_NMDA = (tau_r_NMDA*tau_d_NMDA)/(tau_d_NMDA-tau_r_NMDA)*log(tau_d_NMDA/tau_r_NMDA)

    factor_AMPA = -exp(-tp_AMPA/tau_r_AMPA)+exp(-tp_AMPA/tau_d_AMPA)
    factor_AMPA = 1/factor_AMPA

    factor_NMDA = -exp(-tp_NMDA/tau_r_NMDA)+exp(-tp_NMDA/tau_d_NMDA)
    factor_NMDA = 1/factor_NMDA
}

BREAKPOINT {
    SOLVE state METHOD cnexp
    mggate = 1 / (1 + exp(0.062 * (-v)) * (mg / 3.57))
    g_AMPA = gmax*(B_AMPA-A_AMPA)
    g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate
    g = g_AMPA + g_NMDA
    :i_AMPA = g_ampa*(v-e)
    :i_NMDA = g_NMDA*(v-e)
    :i = i_AMPA + i_NMDA
    i = (g_AMPA + g_NMDA)*(v-e)
}

DERIVATIVE state{
    A_AMPA' = -A_AMPA/tau_r_AMPA
    B_AMPA' = -B_AMPA/tau_d_AMPA
    A_NMDA' = -A_NMDA/tau_r_NMDA
    B_NMDA' = -B_NMDA/tau_d_NMDA
}


NET_RECEIVE (weight,weight_AMPA, weight_NMDA, Psurv, tsyn) {
    LOCAL result
    weight_AMPA = weight
    weight_NMDA = weight * NMDA_ratio
    INITIAL{
        tsyn=t
    }

    if (Fac > 0) {
        u = u*exp(-(t - tsyn_fac)/Fac) :update facilitation variable if Fac>0 Eq. 2 in Fuhrmann et al.
    } else {
        u = Use
    }
    if(Fac > 0){
        u = u + Use*(1-u) :update facilitation variable if Fac>0 Eq. 2 in Fuhrmann et al.
    }

    : tsyn_fac knows about all spikes, not only those that released
    : i.e. each spike can increase the u, regardless of recovered state.
    tsyn_fac = t

    : recovery

    if (Rstate == 0) {
        : probability of survival of unrecovered state based on Poisson recovery with rate 1/tau
        Psurv = exp(-(t-tsyn)/Dep)
        result = urand()
        if (result>Psurv) {
            Rstate = 1     : recover
        }
        else {
            tsyn = t
        }
    }

    if (Rstate == 1) {
        result = urand()
        if (result<u) {
            : release!
            tsyn = t
            Rstate = 0
            A_AMPA = A_AMPA + weight_AMPA*factor_AMPA
            B_AMPA = B_AMPA + weight_AMPA*factor_AMPA
            A_NMDA = A_NMDA + weight_NMDA*factor_NMDA
            B_NMDA = B_NMDA + weight_NMDA*factor_NMDA


        }
    }
}

PROCEDURE setRNG()
{ }

FUNCTION urand() {
    urand = 1.0
:VERBATIM
:    double value;
:    if (_p_rng) {
:            /*
:            :Supports separate independent but reproducible streams for
:            : each instance. However, the corresponding hoc Random
:            : distribution MUST be set to Random.negexp(1)
:            */
:            value = nrn_random_pick(_p_rng);
:            //printf("random stream for this simulation = %lf\n",value);
:            return value;
:    }else{
:ENDVERBATIM
:    : the old standby. Cannot use if reproducible parallel sim
:    : independent of nhost or which host this instance is on
:    : is desired, since each instance on this cpu draws from
:    : the same stream
:    value = scop_random(1)
:VERBATIM
:    }
:ENDVERBATIM
:    urand = value
}
