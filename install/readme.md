# Installation
You can use [dockerfile](../scripts/Dockerfile) for the installation and building the environments in your host.  

For your reference, the discription below provides some briefe expalaniton for each binary.
There are some binaries to be installed in the procedure.  Each binariy file is explained bewlo.

*** Installation package
- mera-2.5.0+pkg.3782-cp310-cp310-manylinux_2_27_x86_64.whl
  This file is the isntallation file for linux host. The dockerfile calls this file to install  RUHMI AI model compiler in your host. 

- mera-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl
- mera2_runtime-2.5.0+rzg3e.24-cp312-cp312-manylinux_2_27_aarch64.whl
  These two files is the isntallation files for your target system based on RZ/G3E. When building the board environment, these files sould be used.

- libmera2_plan_io.so
- libmera2_runtime.so
  These binariies are MERA runtime working on the target system. These ones should be inp;emented in the Linux-basis environment on RZ/G3E.
