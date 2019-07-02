# CICD Test Board.
2nd July 2019

This folder contains two Eagle PCB projects:

1. Feather Azure Sphere
1. Feather Azure Sphere Long Tail

Number 1: Feather Azure Sphere

Is intended to provide a flexible CI/CD host for the WF-MT620 module, connecting it with any Adafruit Feather compatible MCU board or Feather Wing board. The idea is that the Feather board can be used to emulate hardware devices for the purposes of testing Azure Sphere code, or be the actual devices on a Feather Wing.

Connections are stated on the PCB, and include ADC, SPI and I2C as well as control connections for CHPD (Feather MCU enable) and Feather Reset.

Jumper JP1 can connected the Feather 3v to the MT620 3v power line. This can be used to power both modules from a single power source. CARE MUST be taken not to connect both modules to independent power sources when this jumper is connected(set). No power protection is implemented to allow the unit to be powered from either module.

THIS IS A TEST DESIGN. TESTING IS CURRENTLY OUTSTANDING.
Test results will be documented here.

Number 2 Feather Azure Sphere Long Tail

Is the starting point of a design to have an Azure Sphere board more closely sized to the Adafruit Feather boards.

It is incomplete and untested at this point in time.
