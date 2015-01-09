# Mechanism Implementation
There are four functions exposed for each mechanism in the c files generated from NMODL
1. init
2. current
3. jacob
4. state

## init

## current
The current is implemented as `nrn_cur()`, and is is called for each mechanism from `nrn_rhs()`.  It computes the current `i`, which is a function of `v`. It also computes the derivative of voltage w.r.t time, i.e. `g`.

The total current is added to the `VEC_RHS` vector.

### location in NMODL file
The current contribution is defined in the `BREAKPOINT` block. For example, in the KdShu2007 model, it is a single statement (because only one current, that pertaiing to pottasium, is updated)
```
ik = gkbar * m*h*(v-ek)
```
which is translated into the `nrn_current()` helper function in the C code
```C
static double _nrn_current( blah blah blah) {
  double _current = 0.;
  v = _v;
  ik = gkbar * m * h * (v - ek); // this line matters!
  _current += ik;
  return _current;
}
```
At first glance, the generated c code appears to be overly-elaborate, with just one line for the computation of the current `ik` (this is espicially the case as this is called twice to calculate the derivative of the current w.r.t. voltage). However, for mechanisms that contribute to multiple currents (say both calcium and pottasium, or a private non-specific current), this routine will calculate the individual contribution to each current (`ik` in this case), and an accumulated current, i.e. `_current`.

There are some optimizations that I can see
1. when there is only one current being contributed too, there is no need to use the accumulator `current_`.
2. the `nrn_cur` function calculates the derivative of each current contribution w.r.t. voltage, however for the purpose of simulation only the derivative of the total current `g` is required when updating the diagonal of the Jacobi matrix in `nrn_jacobi`. The other currents could be made as optional outputs that can be calculated _on demand_.
3. in this case, the current is a linear function of `v`, because the values of all other parameters (`gkbar`, `m`, `h`, `ek`) are fixed when calculating the current `ik`. So it should be possible to remove the second call to current, by noting that in this case `g=gkbar*m*h`.
4. as noted below, the contribution to the jacobian is the same for all mechanisms: the value of `g` is added to the diagonal entry of the matrix. Could this be performed directly here? There would be issues with race conditions, but it is worth considering.

## jacob
The jacobi is implemented as `nrn_jacob()`. This routine is called in `nrn_lhs`, when updating the diagonal entries in the linear system. Recall that the off-diagonal entries are constant in time, and computed just once during initialization.

The `jacobi` function is not derived from the NMODL definition, instead it is the same for all mechanisms that contribute to the LHS. In pseudo code, it performs the following:
```
for i = 1:nodecount
  VEC_D[ni[i]] += g[i]
end
```

For this to be performed efficiently, the xxx, `g`, has to be computed and stored when calculating the current in `nrn_rhs()`. The function `setup_tree_matrix()` first updates the current in `nrn_rhs()`, which is where `g` is calcuted (`g` is the derivative of current w.r.t voltage, i.e. `di/dv`... I must be missing something because this quantity doesn't make much sense to me, but it is what it is).
```
function setup_tree_matrix ( cell_group )
  nrn_rhs ( cell_group )
  nrn_lhs ( cell_group )
end
```

With this in mind, the value of `g` might best be computed and stored in the call to `nrn_cur` in `nrn_rhs`, for subsequent use in `nrn_jacob`.

### location in NMODL file
As noted above, the `jacobi` function is not derived from the NMODL file.

## state
The state is implemented as `nrn_state()`.
