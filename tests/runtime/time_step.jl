function nrn_fixed_step(cell_data::CellData)

  ### other ###

  # form the matrix (D vector and RHS)
  setup_tree_matrix_minimal(cell_data)
  # solve the linear system
  nrn_solve_minimal(cell_data)
  # update current values for I/O (not used in calculations)
  second_order_cur(cell_data)
  # advance the solution
  update(cell_data)

  ### other ###

  # update states
  nonvint(cell_data)
  ### other ###
  return None
end

function setup_tree_matrix(cell_group)
  nrn_rhs(cell_group)
  nrn_lhs(cell_group)
end

function nrn_lhs(cell_group)
  # range of non-root nodes
  child_nodes = cell_group.ncells+1:cell_group.nnodes
  p = cell_group.parent_indices

  VEC_D[:] = 0.

  for mechanism in cell_group.mechanisms
    jacob(mechanism)
  end

  for i in child_nodes
    VEC_D[i]    -= VEC_B[i]
    VEC_D[p[i]] -= VEC_A[i]
  end
end

function nrn_rhs(cell_group)
  # range of non-root nodes
  child_nodes = cell_group.ncells+1:cell_group.nnodes
  p = cell_group.parent_indices

  # access to cell_group.VEC_* is implicit in the pseudocode
  VEC_RHS[:] = 0.

  for mechanism in cell_group.mechanisms
    current(mechanism)
  end

  for i in child_nodes
    dv             = VEC_V[p[i]] - VEC_V[i]
    VEC_RHS[i]    -= dv * VEC_B[i]
    VEC_RHS[p[i]] += dv * VEC_A[i]
  end
end

function nrn_solve_minimal(cell_group)
  ncells = cell_group.ncells
  nnodes = cell_group.nnodes
  child_nodes = ncells+1:nnodes

  # backward sweep
  for i in reverse(child_nodes)
    factor         = VEC_A[i] / VEC_D[i]
    VEC_D[p[i]]   -= factor * VEC_B[i]
    VEC_RHS[p[i]] -= factor * VEC_RHS[i]
  end

  # last step of backward sweep: apply to all root nodes
  for i in 1:ncells
    VEC_RHS[i] /= VEC_D[i];
  end

  # forward sweep
  for i in child_nodes
    VEC_RHS[i] -= VEC_B[i] * VEC_RHS[p[i]]
    VEC_RHS[i] /= VEC_D[i]
  end
end

# in /nrnoc/eion.c
function second_order_cur(cell_group)
  if secondorder == 2
    for mechanism in cell_group.mechanisms
      if is_ion(mechanism)
        ni   = mechanism.nodeindices
        data = mechanism.data
        for i in 1:mechanism.nodecount
          data.c[i] += data.dc[i] * VEC_RHS[ni[i]]
        end
      end
    end
  end
end

# in /nrnoc/fadvance.c
function update(cell_group)
  factor = 1.0
  if secondorder==true
    factor = 2.0
  end
  # vectorizable
  VEC_V[:] += factor*VEC_RHS[:]
  # apply to only the first mechanism
  nrn_capacity_current(cell_group.mechanisms[1]);
end

# in /nrnoc/capac.c
# called by update
function nrn_capacity_current(mechanism)
  data = mechanism.nodeindices
  ni   = mechanism.data
  cfac = .001 * cj;
  for i in 1:mechanism.nodecount
    data.i_cap[i] = cfac * data.cm[i] * VEC_RHS[ni[i]]
  end
end


function nonvint(cell_group)
  for mechanism in cell_group.mechanisms
    state(mechanism)
  end
end
