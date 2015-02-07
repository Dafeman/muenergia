# μEnergia

**μEnergia: C++ Framework to Develop Embedded Software**

This is a software development platform for **MSP-EXP430G2 LaunchPad**, **TivaC Series EK-TM4C123GXL LaunchPad**, and **Tiva C Series TM4C129 ConnectedLaunchPad**. The framework is lightweight, flexible, and consumes minimum memory and computational resources to build applications and rational agents on microcontrollers that sense and actuate using add-on boards.

μEnergia consists of two parts:

1. **lm4f** : The core functionality is provided by extending Energia lm4f module. 
2. **uEnergia**: This code provides configuration of all the modules and representations that we used in the experiments. 

To install μEnergia, follow these instructions:

1. **lm4f**:
   1. Download Energia 13 from [http://energia.nu/download/](http://energia.nu/download/).
   2. cd to the **energia-0101E0013/hardware/lm4f/cores/lm4f**.
   3. Delete the **main.cpp**, **startup_gcc.c**, **WString.h**, and **WString.cpp** files. 
   4. git clone [https://github.com/samindaa/uEnergia.git](https://github.com/samindaa/uEnergia.git)
   5. Copy all the content inside **lm4f** directory to **energia-0101E0013/hardware/lm4f/cores/lm4f**.
   
2. **uEnergia**:
   1. Attach the sensor hub to BoosterPack 1, if you are using a Tiva C Series TM4C1294 Connected LaunchPad Evaluation Kit.
   2. Open Energia GUI and select **uEnergia.ino** within the **uEnergia** directory.
   3. Select the appropriate board configuration and upload the program.  



