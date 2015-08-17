NEURON {
    THREADSAFE
    POINT_PROCESS ProbAMPANMDA_EMS
    RANGE tau_r_AMPA, tau_d_AMPA, tau_r_NMDA, tau_d_NMDA
    RANGE Use, u, Dep, Fac, u0, mg, Rstate, tsyn_fac, u
    RANGE e, NMDA_ratio
    RANGE A_AMPA_step, B_AMPA_step, A_NMDA_step, B_NMDA_step
    NONSPECIFIC_CURRENT i
    RANGE synapseID
}

PARAMETER {
    tau_r_AMPA = 0.2
    tau_d_AMPA = 1.7
    tau_r_NMDA = 0.29
    tau_d_NMDA = 43
    Use = 1.0
    Dep = 100
    Fac = 10
    e = 0
    mg = 1
    gmax = .001
    u0 = 0
    synapseID = 0
    NMDA_ratio = 0.71
}

ASSIGNED {
    v (mV)
    i_AMPA (nA)
    i_NMDA (nA)
    g_NMDA (uS)
    factor_AMPA
    factor_NMDA
    A_AMPA_step
    B_AMPA_step
    A_NMDA_step
    B_NMDA_step
    rng
    mggate
    Rstate
    tsyn_fac
    u
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

    tp_AMPA = (tau_r_AMPA*tau_d_AMPA)
             /(tau_d_AMPA-tau_r_AMPA)
             *log(tau_d_AMPA/tau_r_AMPA)

    tp_NMDA = (tau_r_NMDA*tau_d_NMDA)
             /(tau_d_NMDA-tau_r_NMDA)
             *log(tau_d_NMDA/tau_r_NMDA)

    factor_AMPA = -exp(-tp_AMPA/tau_r_AMPA)
                  +exp(-tp_AMPA/tau_d_AMPA)
    factor_AMPA = 1/factor_AMPA

    factor_NMDA = -exp(-tp_NMDA/tau_r_NMDA)
                  +exp(-tp_NMDA/tau_d_NMDA)
    factor_NMDA = 1/factor_NMDA

    A_AMPA_step = exp(dt*(( - 1.0 ) / tau_r_AMPA))
    B_AMPA_step = exp(dt*(( - 1.0 ) / tau_d_AMPA))
    A_NMDA_step = exp(dt*(( - 1.0 ) / tau_r_NMDA))
    B_NMDA_step = exp(dt*(( - 1.0 ) / tau_d_NMDA))
}

BREAKPOINT {
    LOCAL mggate, g_AMPA, g_NMDA, g, i_AMPA, i_NMDA
    : make g the conductance
    :CONDUCTANCE g_AMPA
    :CONDUCTANCE g_NMDA
    CONDUCTANCE g
    SOLVE state

    mggate = 1 / (1 + exp(-0.062 * v) * (mg / 3.57))
    g_AMPA = gmax*(B_AMPA-A_AMPA)
    g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate
    g = g_AMPA + g_NMDA

    : this is not the most efficient way to do this!
    :vve = (vv-e)
    :i_AMPA = g_AMPA*vve
    :i_NMDA = g_NMDA*vve
    :i = i_AMPA + i_NMDA

    : this is how to do it
    i = g*(v-e)

    : note that this is not calculating the same derivative
    : that was calculated before: mggate is dependent on v, which
    : in turn contributes to the g_AMPA and g_NMDA variables
}

PROCEDURE state() {
    A_AMPA = A_AMPA*A_AMPA_step
    B_AMPA = B_AMPA*B_AMPA_step
    A_NMDA = A_NMDA*A_NMDA_step
    B_NMDA = B_NMDA*B_NMDA_step
}


NET_RECEIVE (weight, weight_AMPA, weight_NMDA, Psurv, tsyn){
    LOCAL result
    weight_AMPA = weight
    weight_NMDA = weight * NMDA_ratio

    INITIAL{
        tsyn=t
    }

    if (Fac > 0) {
        u = u*exp(-(t - tsyn_fac)/Fac)
        u = u + Use*(1-u)
    } else {
        u = Use
    }

    tsyn_fac = t

    if (Rstate == 0) {
        Psurv = exp(-(t-tsyn)/Dep)
        result = urand()
        if (result>Psurv) {
            Rstate = 1
        }
        else {
            tsyn = t
        }
    }

    if (Rstate == 1) {
        result = urand()
        if (result<u) {
            tsyn = t
            Rstate = 0
            A_AMPA = A_AMPA + weight_AMPA*factor_AMPA
            B_AMPA = B_AMPA + weight_AMPA*factor_AMPA
            A_NMDA = A_NMDA + weight_NMDA*factor_NMDA
            B_NMDA = B_NMDA + weight_NMDA*factor_NMDA
        }
    }
}

PROCEDURE setRNG() {
}

FUNCTION urand() {
    urand=1
}

