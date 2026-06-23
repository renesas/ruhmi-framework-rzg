# Installation
You can use [dockerfile](../scripts/Dockerfile) for the installation and building the environments in your host.  

For your reference, the description below provides a brief explanation for each binary.
There are some binaries to be installed in the procedure.  Each binary file is explained below.

###  Installation package
- `mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl`

  This file is the installation file for Linux host. The dockerfile calls this file to install RUHMI AI model compiler on your host.

- `mera-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl`
- `mera2_runtime-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl`
  These two files are the installation files for your target system based on RZ/G3E. When building the board environment, these files should be used.

- libmera2_plan_io.so
- libmera2_runtime.so
  These binaries are MERA runtime libraries working on the target system. These should be deployed into the Linux-based environment on RZ/G3E.
