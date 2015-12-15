TITLE GABAAB receptor with presynaptic short-term plasticity 

NEURON {
    THREADSAFE
    POINT_PROCESS ProbGABAAB_EMS
    RANGE tau_r_GABAA, tau_d_GABAA, tau_r_GABAB, tau_d_GABAB 
    RANGE Use, u, Dep, Fac, u0, Rstate, tsyn_fac, u
    RANGE e_GABAA, e_GABAB, GABAB_ratio
    RANGE A_GABAA_step, B_GABAA_step, A_GABAB_step, B_GABAB_step
    NONSPECIFIC_CURRENT i
    RANGE synapseID, verboseLevel
}

PARAMETER {
    tau_r_GABAA  = 0.2   (ms)
    tau_d_GABAA = 8   (ms)
    tau_r_GABAB  = 3.5   (ms)
    tau_d_GABAB = 260.9   (ms)
    Use        = 1.0   (1)
    Dep   = 100   (ms)
    Fac   = 10   (ms)
    e_GABAA    = -80     (mV)
    e_GABAB    = -97     (mV)
    gmax = .001 (uS)
    u0 = 0
    synapseID = 0
    verboseLevel = 0
    GABAB_ratio = 0 (1)
}


ASSIGNED {
    v (mV)
    i_GABAA (nA)
    i_GABAB (nA)
    g_GABAA (uS)
    g_GABAB (uS)
    A_GABAA_step
    B_GABAA_step
    A_GABAB_step
    B_GABAB_step
    g (uS)
    factor_GABAA
    factor_GABAB
    rng
    Rstate (1)
    tsyn_fac (ms)
    u (1)
}

STATE {
    A_GABAA
    B_GABAA
    A_GABAB
    B_GABAB
}

INITIAL{
    LOCAL tp_GABAA, tp_GABAB

    Rstate=1
    tsyn_fac=0
    u=u0
    A_GABAA = 0
    B_GABAA = 0
    A_GABAB = 0
    B_GABAB = 0
    tp_GABAA = (tau_r_GABAA*tau_d_GABAA)
              /(tau_d_GABAA-tau_r_GABAA)
              *log(tau_d_GABAA/tau_r_GABAA)
    tp_GABAB = (tau_r_GABAB*tau_d_GABAB)
              /(tau_d_GABAB-tau_r_GABAB)
              *log(tau_d_GABAB/tau_r_GABAB)

    factor_GABAA = -exp(-tp_GABAA/tau_r_GABAA)
                   +exp(-tp_GABAA/tau_d_GABAA)
    factor_GABAA = 1/factor_GABAA
    factor_GABAB = -exp(-tp_GABAB/tau_r_GABAB)
                   +exp(-tp_GABAB/tau_d_GABAB)
    factor_GABAB = 1/factor_GABAB

    A_GABAA_step = exp(dt*(( - 1.0 ) / tau_r_GABAA))
    B_GABAA_step = exp(dt*(( - 1.0 ) / tau_d_GABAA))
    A_GABAB_step = exp(dt*(( - 1.0 ) / tau_r_GABAB))
    B_GABAB_step = exp(dt*(( - 1.0 ) / tau_d_GABAB))
}

BREAKPOINT {
    LOCAL g_GABAA, g_GABAB, g
    CONDUCTANCE g_GABAA
    CONDUCTANCE g_GABAB

    SOLVE state
    g_GABAA = gmax*(B_GABAA-A_GABAA)
    g_GABAB = gmax*(B_GABAB-A_GABAB)
    i = g_GABAA*(v-e_GABAA) + g_GABAB*(v-e_GABAB)
}

PROCEDURE state() {
    A_GABAA = A_GABAA*A_GABAA_step
    B_GABAA = B_GABAA*B_GABAA_step
    A_GABAB = A_GABAB*A_GABAB_step
    B_GABAB = B_GABAB*B_GABAB_step
}


NET_RECEIVE (weight, weight_GABAA, weight_GABAB, Psurv, tsyn)
{
    LOCAL result
    weight_GABAA = weight
    weight_GABAB = weight*GABAB_ratio


    INITIAL{
        tsyn=t
    }

    if (Fac > 0) {
        u = u*exp(-(t - tsyn_fac)/Fac)
    } else {
        u = Use
    }
    if(Fac > 0){
        u = u + Use*(1-u)
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

           A_GABAA = A_GABAA + weight_GABAA*factor_GABAA
           B_GABAA = B_GABAA + weight_GABAA*factor_GABAA
           A_GABAB = A_GABAB + weight_GABAB*factor_GABAB
           B_GABAB = B_GABAB + weight_GABAB*factor_GABAB
      }
    }
}

PROCEDURE setRNG() {
}

FUNCTION urand() {
    urand = 1.0
}

