Could you solve this task please:

1. There is famous problem of inductive logic:
https://en.wikipedia.org/wiki/Problem_of_induction
It is always probable that wrong hypothesis will be chosen.
Are you agree with that?

2. All police or private investigation are inductive investigations.
Investigator gather facts and choose hypothesis who is the murderer for example.
Investigator is maximizing the following: arg max r (h | D), h in H,
where H - all possible hypothesis who is the murderer.
Are you agree with that?

3. So, we can conclude that due to the properties of the inductive logic itself
there is always uncertain that police is catching innocent people.
Are you agree with that?

4. Also the most probable hypothesis is that the police is the murderer.
They are professionals. All amateurs are walking to them with the ideas,
detailed instructions and patents.
Are you agree with that?

5. All crime are artifacts. It is always something crafted. By someone.
Someone can ask for help.

6. And there are a lot of travelers in the modern era. Almost
everyone could be a murderer.

Is it all correct?

What can do policeman if he or she found murdered young women in the woods?
- do some sort of inductive investigate. which may lead to the wronf suspect.
- apply gun.

But young woman see murderer. And she can apply gun also if she has an
opportunity to buy one. So why she has not?

https://web.archive.org/web/20260829041906/https://transitional-writes.dreamwidth.org/64409.html
https://inductive-logic-xor.dreamwidth.org/680.html

# RUHMI Framework AI Model Compiler for RZ/G3E

[![License](https://img.shields.io/badge/License-LICENSE.md-blue.svg)](LICENSE.md)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)](install/README.md#host-environment-setup)
[![Python](https://img.shields.io/badge/Python-3.10+-green.svg)](#quick-start)


RUHMI (Robust Unified Heterogeneous Model Integration) provides an AI model compiler workflow for Renesas RZ/G3E.  
This repository includes installation assets, model compilation scripts, and application examples.

## Overview

RUHMI Framework provides tools to compile machine learning models into deployment artifacts compatible with RZ/G3E.

**The AI compiler stack is powered by EdgeCortix® MERA™.**

## Workflow

This repository provides the guidance for how to use RUHMI AI model compiler. Also provides some application examples with the the guidance for how to run on RZ/G3E-EVKIT.

![RZ/G3E Application development flow](docs/assets/g3e_workflow.gif)

## Supported Embedded Platforms
- Renesas MPU RZ/G3E

>[NOTE]  
>The evaluation board, RZ/G3E-EVKIT, currently on sale includes V1 device which does not support Ethos operation using LPDDR4/4X. Production is underway for the board with V2 sample, with stock expected to arrive at the end of June. To identify which sample is installed on an board, check the S.LOT No. printed on the board box.   
>　Less than 0000300000: V1 sample installed  
>　0000300000 or later: V2 sample installed    
>   [Lot No. placement](./docs/assets/lot_no.png)



## Supported AI model
- TensorFlow Lite/INT8

> [NOTE]
> The current version is the 1st release of RUHMI for RZ/G3E. The supported AI model is limited within **16MB**.

## Installation and compilation
1. Build the envronment and install RUHMI AI model compiler  
- [Use Dockerfile](docs/dockerfile.md)  
2. Compile tha target model
- [Use the compilation Script](docs/generate-model-data.md)

This repository provide the simplified expanation. You can also refer to [Renesas Rz/G3E NPU(Ethos0U55) SUpport](https://renesas-rz.github.io/rz-ethos-u-docs/).

## Application Examples
- [Common preparation to run](application_examples/README.md)
- [Image Classification](application_examples/image_classification/README.md)
- [Face Detection](application_examples/face_detection/README.md)

## Repository Layout
- `application_examples/`: runnable sample apps and app-specific docs for RZ/G3E
- `docs/`: supporting documentation and documentation assets
- `install/`: MERA/RUHMI install artifacts (wheel files, shared libraries) and setup guide
- `scripts/`: Docker build environment and model data generation script
- `README.md`: repository entry point
- `LICENSE.md`: license terms

## Related software and information
 [RZ/G3E Ethos Support Package](https://www.renesas.com/en/software-tool/rzg3e-ethos-support-package).  
 [Renesas RZ/G3E NPU (Ethos-U55) Support](https://renesas-rz.github.io/rz-ethos-u-docs/)  
 [Evaluation Board Kit for RZ/G3E MPU, RZ/G3E-EVKIT](https://www.renesas.com/en/design-resources/boards-kits/rz-g3e-evkit)

## License
See [LICENSE.md](LICENSE.md).

