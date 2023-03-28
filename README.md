[![Build Status](https://travis-ci.org/ijs-f8/Research-Reactor-Simulator.svg?branch=master)](https://travis-ci.org/ijs-f8/Research-Reactor-Simulator)

# Research Reactor Simulator

Research Reactor Simulator is a real-time graphical simulation code developed at Institut Jožef Stefan. It is meant to be used as a complementary teaching tool in classroom and to train future reactor operators, especially in institutions with limited access to a physical reactor.

## Citation
J. Malec, D. Toškan, and L. Snoj, [‘PC-based JSI research reactor simulator’, Annals of Nuclear Energy](https://www.sciencedirect.com/science/article/pii/S0306454920303285), vol. 146, p. 107630, Oct. 2020, doi: 10.1016/j.anucene.2020.107630.


## Building instruction
The project source code is located in the "nanogui" folder.
Use "cmake" to create build configuration in any folder.

On Ubuntu Linux, the following set of commands should build Research Reactor simulator:
```
apt update && apt -y install make git cmake xorg-dev libgl1-mesa-dev g++-multilib
mkdir build && cd build
cmake ..
make
```
