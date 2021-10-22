# Compiling firmware

1. Copy either `duty_cycle` to `glitch` your workspace
2. Copy the files from `common/` to the root of your project
3. Download the [PRU support package](https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree?h=master)
4. Extract `include/` to a known location
5. Include the folders `include/` and `include/am335x` in your compilation if you're using CCS
6. Compile! (Preferrably with -O4 and speed optimizations, remoteproc is slower than UIO-PRUSS)
