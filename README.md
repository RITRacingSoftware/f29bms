# RIT Formula SAE Battery Management System Firmware
Welcome! This is the home of all source for the BMS master board stm32 microcontroller.
This code accomplishes the following responsibilities:
1. Balance + Monitor Series Cell Voltages
2. Monitor Battery Pack Temperatures
3. Facilitate Charging of the Battery Pack
4. Report data such as State of Charge over CAN
5. Indicate faults by triggering f29's shutdown circuit

## Installation
We utilize Docker to build anywhere. Install it from [docker.com](https://www.docker.com/).

If you would like to build on Windows, there are additional steps, see [Docker's windows instructions](https://docs.docker.com/docker-for-windows/install-windows-home/) and [here](#Windows)

## Building Everything/Running all tests
Once docker is installed, use `docker-build.sh` to build the docker image.

From now on, you can just use `docker-run-build.sh` to run the firmware build in a docker container based on the above image.

If you need to update your docker image, use `docker-remove-image.sh` then build it again with `docker-build.sh`.

Sometimes, these scripts don't work on Windows. If not, see [below](#building-specific-targets).

## Testing
Two types of tests exist for the codebase: Unit Testing, and Software in the Loop (SIL) testing.

Unit tests validate single modules, using CMock to mock out dependencies.

SIL tests build f29bms for linux and run it in a child process. 
Python tests using a sockets interface feed the program input and validate its output.

## Building Specific Targets
Use `enter-docker.sh` to get a shell inside the f29bms docker container. From here, you can run `build.sh` to build everything, just like `docker-run-build.sh`.

Specific targets can also be built using SCons commands from the docker container shell:
- `scons bin/f29bms.bin`
    -build stm32 binary for loading onto the BMS
- `scons unit-tests`
    -build and run application module unit tests
- `scons open-loop` 
    -build and run SIL tests
- `scons sim`
    -build f29bms linux program for SIL testing
- `scons sim-interface`
    -build interface library for SIL testing
- `scons test-apps`
    -build driver module test application hexes for loading onto an stm32 development board
- `scons memchecks`
    -run valgrind memcheck tool on unit tests
- `scons bin/test_STM32_<driver module name>.hex`
    -build a specific driver test application hex
- `scons src/app/<app module name>/unit_test_results.txt`
    -unit test a specific application module
- `scons src/app/<app module name>/memcheck_results.txt`
    -run valgrind memcheck on a specific application unit test runner

You can speed up builds with multithreading by adding `-j<number of cores>` to any scons command

## Custom SCons Build Options
- `--dbg`
    -adds `-g` to linux build commands, enabling debugging symbols (helps track valgrind error origins)

See [SCons](https://github.com/SCons/scons/wiki) and our `sconstruct.py` (top level) for more details on the build system.

## Source Code Structure
Each C module has its own folder where its header (.h), source (.c), and test source (test_\<module name>.c) files are located.

Application logic is implemented in hardware agnostic application modules under src/app. The test source for these modules contains unit tests that run automatically using Unity/CMock.

Driver code is implemented in hardware specific driver modules under src/driver. There are two source files for each driver module: SIM_HAL_\<module name>.c, drivers for interfacing with a simulation running on linux, and STM32_HAL_\<module name>.c, drivers for the stm32 microcontroller. The test source for driver modules is a program made for running on an stm32 evaluation board for testing the drivers.

## Windows 

Follow guide above on installing Docker 


With powershell:
- CD to directory of f29bms (dir with the dockerfile)
- run ``` docker build -t bms-container .```  you can name this whatever
- when done run ``` docker run -it bms-container .```
- To run scripts, make sure you are in the shell for the container and run  ``` bash something.sh``` (eg. build.sh)


 To get the bin file to flash on to the BMS: easiest way is open up docker desktop, find the container and under the files tab, navigate to /f29bms/bin, and rightclick save

Additional reading/help:
[https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-containers](https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-containers) 

With VScode:
- Install Docker, WSL etc
- Clone this project and open that folder 
- Hit the >< looking thing bottom left corner of VS code and connect to WSL
- First time run: click the >< again, select "new dev container"
- Select and create C++ container
- idk this didn't work just use powershell lol
      
