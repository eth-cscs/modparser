#State of the art

Some reasons why Neuron is **not HPC**
* poor utilization of possible FLOPs (according to roofline)
* wrong data layout (SoA vs. AoS)
* excessive use of memory
* not performance portable (it isn't possible to run on GPU, and not possible to port in current form)

The team at BBP have done a good job of stripping away the functionality that isn't required to perform the simulation tasks in the HBP. **However, to get a code that is HPC ready, just modify the code and 'getting one more iteration' out of it won't work**.

###Mechanisms: _state_ and _kernels_
Each mechanism has state, which is a set of values defined at each point of a cell discretizatio at which the mechanism is present. Mechanisms also have kernels, which do the following
* modify mechanism state
* modify global state
  * update the system matrix for the cell
  * update current at affected sites

Mechanisms modify state by applying kerenels. Applying the mechanism kernels (`state`, `current`, etc.) dominate time to solution (98% for the PCP benchmarks that I did a while back).

###Mechanisms: performance
We can speed up kernels
* reducing DRAM pressure
  * kernels are bandwidth bound, and there are lots of redundant loads and stores, that could be removed.
  * use _Structure of Arrays_, to  ensure that cache lines only contain useful data
* improve vectorization
  * use _Structure of Arrays_.
* remove redundant flops
  *
* increase work-per-loop
  * reduce memory footprint
* make it possible for the compiler to reason about possible optimizations


###