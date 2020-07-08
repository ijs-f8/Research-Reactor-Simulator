# Research Reactor Simulator

Research Reactor Simulator is a real-time graphical simulation code developed at Institut Jožef Stefan. It is meant to be used as a complementary teaching tool in classroom and to train future reactor operators, especially in institutions with limited access to a physical reactor.

## Building instruction
The project source code is located in the "nanogui" folder.
Use "cmake" to create build configuration in any folder.

On Ubuntu Linux, the following set of commands should build Research Reactor simulator:
```
apt update && apt update && apt -y install make git cmake xorg-dev libgl1-mesa-dev g++-multilib
mkdir nanogui/build && cd nanogui/build
cmake ..
make
```