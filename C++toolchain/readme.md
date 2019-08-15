# Notes on C++ tool chain for Azure Sphere RTCore apps

It is possible to compile C++ as an RTCore library application. This enables the use of tensorflow lite for microcontrollers by compiling tf lite exmaples as static libraries with the example main entry point as an extern C presentation. 

Further work do enable C++ RTCore apps directly is on going.

Toolchain file: AzureSphereRTCoreToolchainCXX.cmake

Copy this file to your Azure Sphere SDK directory e.g.
``` 
C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles
```

Linker script: linker.ld

This example of a C++ linker script for RTCore enables the compilation and incorporation of the C++ runtime. Modify as required for your application. Copy this file to your application directory.