import SCons

import os
from pathlib import Path

from site_scons import scons_constants

"""
SCons entry point for f29bms project.
Contains build instructions for everything.

author: Chris Blust
"""
# setup static directory structure
SRC_DIR = REPO_ROOT_DIR.Dir('src')
COMMON_DIR = SRC_DIR.Dir('common')
APP_DIR = SRC_DIR.Dir('app')
DRIVER_DIR = SRC_DIR.Dir('driver')
BIN_DIR = REPO_ROOT_DIR.Dir('bin')
LIBS_DIR = REPO_ROOT_DIR.Dir('libs')
STM32_LIB_DIR = LIBS_DIR.Dir('stm32libs/STM32F0xx_StdPeriph_Driver')
STM32_CMSIS_DIR = LIBS_DIR.Dir('stm32libs/CMSIS')
SIM_DIR = SRC_DIR.Dir('sim')
OPEN_LOOP_TESTS_DIR = REPO_ROOT_DIR.Dir('sim_tests/open_loop/')
CMOCK_ROOT_DIR = REPO_ROOT_DIR.Dir('libs/cmock/')

LINKER_FILE = REPO_ROOT_DIR.File('stm32f091.ld')

# command line options
AddOption('--dbg',
          dest='dbg',
          action='store_true',
          help='whether to include -g when compiling linux binaries')

# Need an environment for executing commands on the system
# This is only needed because the default environment uses sh instead of bash
command_env = Environment(
    SHELL='bash'
)

# module names are determined by folder name under app/ or driver/
# application module directories go under APP_DIR, and driver modules under DRIVER_DIR

# Get list of tuples for application modules: (name, directory path)
app_modules = []
for directory in (d for d in Path(str(APP_DIR)).iterdir() if d.is_dir()):
    module_path = REPO_ROOT_DIR.Dir(str(directory))
    module_name = module_path.abspath.split('/')[-1]
    app_modules.append((module_name, module_path))
# Get list of tuples for driver modules: (name, directory path)
driver_modules = []
for directory in (d for d in Path(str(DRIVER_DIR)).iterdir() if d.is_dir()):
    module_path = REPO_ROOT_DIR.Dir(str(directory))
    module_name = module_path.abspath.split('/')[-1]
    driver_modules.append((module_name, module_path))
# Get list of tuples for common module: (name, directory path)
common_modules = []
for directory in (d for d in Path(str(COMMON_DIR)).iterdir() if d.is_dir()):
    module_path = REPO_ROOT_DIR.Dir(str(directory))
    module_name = module_path.abspath.split('/')[-1]
    common_modules.append((module_name, module_path))
# Posix Simulation related modules
sim_modules = []
sim_includes = []
for directory in (d for d in Path(str(SIM_DIR)).iterdir() if d.is_dir()):
    module_path = REPO_ROOT_DIR.Dir(str(directory))
    sim_includes.append(module_path)
    module_name = module_path.abspath.split('/')[-1]
    sim_modules.append((module_name, module_path))

modules = app_modules + driver_modules + common_modules

"""
cmock generation. Generates mocks for each module and unit test runner source for each application module.
"""
# this essentially "includes" the cmock tool from site_scons/site_init.py
cmock_env = Environment(
    tools=[TOOL_CMOCK]
)

# Instructions for generating cmocked modules
# list of each module's 'mocks' dir (where the mocks are stored)
mock_modules = []
cmock_generated_headers = []
for module_name, module_dir in (app_modules + driver_modules):
    mocks_dir = module_dir.Dir('mocks')
    # later source in this file will need these directories
    mock_modules.append(mocks_dir)
    cmock_header = cmock_env.GenerateMocks(
           # only input is the module's header file
           source=module_dir.File(module_name + '.h'),
           target=[mocks_dir.File('Mock{}.c'.format(module_name)), mocks_dir.File('Mock{}.h'.format(module_name))]
    )
    cmock_generated_headers += cmock_header
    Clean(cmock_header, mocks_dir) # tell scons to clean these up when --clean is specified

Alias('cmock-headers', cmock_generated_headers)

# Instructions for generating unit test runner source
cmock_testrunner_src = []
for module_name, module_dir in app_modules:
    mocks_dir = module_dir.Dir('mocks')
    testrunner = cmock_env.GenerateTestRunner(
        # input is just the file containing the unit tests
        source=module_dir.File('test_{}.c'.format(module_name)),
        target=mocks_dir.File('testrunner_{}.c'.format(module_name))
    )
    cmock_testrunner_src += testrunner

Alias('cmock-testrunner-src', cmock_testrunner_src)

"""
Application/Driver Module compilation instructions.
Supports Linux and stm32 compilation targets.
Dependency differentiation is based on filename: stm32 objects end in .stm32.o
"""

# set up an environment to compile for linux
tool_paths = []
tool_paths.append(REPO_ROOT_DIR.Dir('libs/cmock/vendor/unity/src'))
tool_paths.append(REPO_ROOT_DIR.Dir('libs/cmock/src'))
tool_paths.append(STM32_LIB_DIR.Dir('inc'))
tool_paths.append(STM32_CMSIS_DIR.Dir('Device/ST/STM32F0xx/Include'))
tool_paths.append(STM32_CMSIS_DIR.Dir('Include'))
tool_paths.append('libs/nanopb/')
tool_paths.append('libs/FreeRTOS/Source/include')
tool_paths.append('libs/FreeRTOS/Source/portable/ThirdParty/GCC/Posix')
module_path_names = []
for mod_name, mod_path in modules:
    module_path_names.append(mod_path)

linux_comp_flags = ['-lm']
if GetOption('dbg'): # set by command line option
    print("---Compiling linux objects with debug symbols---")
    linux_comp_flags.append('-g')

linux_comp_env = Environment(
    CC='gcc',
    CPPPATH=module_path_names + mock_modules + sim_modules + tool_paths + [SRC_DIR.abspath, APP_DIR.abspath, COMMON_DIR.abspath, SIM_DIR.abspath],
    CCFLAGS=linux_comp_flags,
    LIBS=['m']
)

# First need instructions for building FreeRTOS
FREERTOS_DIR = LIBS_DIR.Dir("FreeRTOS")

freertos_include = [
    SRC_DIR, # (FreeRTOSConfig.h is located here)
    FREERTOS_DIR,
    FREERTOS_DIR.Dir('Source/include'),
    FREERTOS_DIR.Dir('Source/portable/ThirdParty/GCC/Posix'),
    FREERTOS_DIR.Dir('Source/portable/ThirdParty/GCC/Posix/utils'),
]

stm32_freertos_include = [
    SRC_DIR, # (FreeRTOSConfig.h is located here)
    FREERTOS_DIR,
    FREERTOS_DIR.Dir('Source/include'),
    FREERTOS_DIR.Dir('Source/portable/ThirdParty/GCC/ARM_CM0'),
]

stm32_comp_env = Environment(
    tools=[TOOL_ARM_ELF_HEX, 'gcc', 'as'],
    CC=scons_constants.ARM_CC,
    AS=scons_constants.ARM_AS,
    LD=scons_constants.ARM_LD,
    CPPPATH=stm32_freertos_include + module_path_names + tool_paths + [COMMON_DIR.abspath, SRC_DIR.Dir('app').abspath, SRC_DIR.abspath],
    CPPDEFINES=['STM32F091', 'USE_STDPERIPH_DRIVER'],
    CCFLAGS=['-ggdb','-mcpu=cortex-m0', '-mthumb', '-lm'],
    ASFLAGS=['-mthumb', '-I{}'.format(STM32_LIB_DIR.Dir('inc').abspath), '-I{}'.format(STM32_CMSIS_DIR.Dir('Include').abspath)],
    LDFLAGS=['-T{}'.format(LINKER_FILE.abspath), '-mcpu=cortex-m0', '-mthumb', '-Wall', '--specs=nosys.specs', '-lm']
)

# instructions to compile every module
linux_app_objects = {}
stm32_app_objects = {}
linux_driver_objects = {}
stm32_driver_objects = {}
linux_common_objects = {}
stm32_common_objects = {}
for module_name, module_dir in app_modules:
    # intructions for linux compilation
    linux_app_objects[module_name] = linux_comp_env.Object(module_dir.File(module_name + '.c'))
    # instructions for stm32 compilation
    stm32_app_objects[module_name] = stm32_comp_env.Object(
        source=module_dir.File(module_name + '.c'),
        target=module_dir.File('STM32_' + module_name + '.o')
    )
    # need this since we use a custom target name
    Clean(stm32_app_objects[module_name], module_dir.File(module_name + '.stm32.o'))

for module_name, module_dir in driver_modules:
    linux_driver_objects[module_name] = linux_comp_env.Object(module_dir.File('SIM_' + module_name + '.c'))
    stm32_driver_objects[module_name] = stm32_comp_env.Object(
        source=module_dir.File('STM32_' + module_name + '.c'),
        target=module_dir.File('STM32_' + module_name + '.o')
    )

    # need this since we use a custom target name
    Clean(stm32_driver_objects[module_name], module_dir.File('STM32_' + module_name + '.o'))

for module_name, module_dir in common_modules:
    linux_common_objects[module_name] = linux_comp_env.Object(module_dir.File(module_name + '.c'))
    stm32_common_objects[module_name] = stm32_comp_env.Object(
        source=module_dir.File(module_name + '.c'),
        target=module_dir.File('STM32_' + module_name + '.o')
    )

    # need this since we use a custom target name
    Clean(stm32_common_objects[module_name], module_dir.File('STM32_' + module_name + '.o'))

# Compile stm32 provided hardware libraries
stm32_lib_objs = []
for source in Glob(os.path.join(STM32_LIB_DIR.Dir('src').abspath, '*.c')):
    stm32_lib_objs += stm32_comp_env.Object(source)

#env = Environment()
#env.Object('stm32libs/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc_ride7/startup_stm32f0xx.s')

stm32_lib_objs += stm32_comp_env.Object(LIBS_DIR.File('stm32libs/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.c'))

# instructions to compile every mock module
mock_objects = {}
for module_name, module_dir in (app_modules + driver_modules):
    mock_objects[module_name] = linux_comp_env.Object(module_dir.File('mocks/Mock{}.c'.format(module_name)))

"""
CAN module depends on generated CAN code.
This section defines this dependency and provides instructions for generating this code
using cantools, a python module.
"""
DBC_DIR = APP_DIR.Dir('CAN')
DBC_NAME = 'f29bms_dbc'

# instructions to generate can packing source code
can_gen_env = Environment(
    tools=[TOOL_CANTOOLS]
)

generated_dbc_source = can_gen_env.GenerateDbcSource(
    source=DBC_DIR.File(DBC_NAME + '.dbc'),
    target=DBC_DIR.File(DBC_NAME + '.c')
)

# anything that depends on the generated c file depends on the generated h file
# anything that depends on CAN.h depends on the generated h file
Depends(DBC_DIR.File(DBC_NAME + '.h'), generated_dbc_source)
Depends(DBC_DIR.File('CAN.h'), generated_dbc_source)

Clean(DBC_DIR.File(DBC_NAME + '.c'), DBC_DIR.File(DBC_NAME + '.h'))

Clean(generated_dbc_source, DBC_DIR.File(DBC_NAME + 'h'))

# instructions for compiling can packing source code
linux_dbc_gen_obj = linux_comp_env.Object(DBC_DIR.File(DBC_NAME + '.c'))
stm32_dbc_gen_obj = stm32_comp_env.Object(
    source=DBC_DIR.File(DBC_NAME + '.c'),
    target=DBC_DIR.File('STM32_' + DBC_NAME + '.o')
)

# establish explicit dependency of compiled CAN module on generated source
Depends(linux_app_objects['CAN'], generated_dbc_source)
Depends(stm32_app_objects['CAN'], generated_dbc_source)
Depends(mock_objects['CAN'], generated_dbc_source)

"""
Instructions for Unit test compilation.
For each module, creates a linux binary from the test_<Module Name>.c file
found in the module directory.
"""
# compiles the cmock library
# also compiles unity (unit testing framework, submodule of cmock lib)
CMOCK_ROOT_DIR = REPO_ROOT_DIR.Dir('libs/cmock/')
cmock_libs = Command(
    [CMOCK_ROOT_DIR.File('build/meson-out/libcmock.a.p/cmock.c.o'), CMOCK_ROOT_DIR.File('build/meson-out/libunity.a.p/unity.c.o')],
    [],
    'cd {} && meson build && cd build && meson compile'.format(CMOCK_ROOT_DIR.abspath)
)
Clean(cmock_libs, CMOCK_ROOT_DIR.Dir('build'))

cmock_libs = [CMOCK_ROOT_DIR.File('build/meson-out/libcmock.a.p/cmock.c.o'), CMOCK_ROOT_DIR.File('build/meson-out/libunity.a.p/unity.c.o')]

unit_tested_modules = []
for i in range(len(app_modules)):
    app = app_modules[i]
    # CAN doesn't do enough to need to be unit tested
    # And uses some FreeRTOS stuff which isn't worth mocking out
    if app[0] != 'CAN':
        unit_tested_modules.append(app)

# instructions for compiling each module's unit test source
test_objects = {}
for module_name, module_dir in unit_tested_modules:    
    test_objects[module_name] = linux_comp_env.Object(module_dir.File('test_{}.c'.format(module_name)))

# instructions for compiling test runner executables
app_test_runners = []
for module_name, module_dir in unit_tested_modules:
    mocks_dir = module_dir.Dir('mocks')

    # shallow copy mock_objects and remove the module under test's mock
    # this is so we aren't testing mocked functions and instead test the real ones
    req_mocks = mock_objects.copy()
    del req_mocks[module_name]

    test_runner = linux_comp_env.Program(
        target=module_dir.File('testrunner_' + module_name),
        source=[
            mocks_dir.File('testrunner_' + module_name + '.c'), 
            cmock_libs,
            req_mocks.values(),
            linux_common_objects.values(),
            linux_dbc_gen_obj,
            test_objects[module_name], 
            linux_app_objects[module_name]]
    )
    app_test_runners.append(test_runner)

Alias('testrunners', app_test_runners)

"""
Instructions for running unit tests with/without memory checks.
If a unit test fails, the build fails.
"""

valgrind_env = Environment(
    tools=[TOOL_VALGRIND]
)

# contains scons nodes signifying unit test runs
unit_test_results = []
valgrind_test_results = []
for module_name, module_dir in unit_tested_modules:
    # executable to run
    testrunner = module_dir.File('testrunner_' + module_name)
    # results file to print to
    test_result_file = module_dir.File('unit_test_results.txt')
    unit_test_results += command_env.Command(
        source=testrunner,
        target=test_result_file,
        # TODO remove janky bash workaround and get Command() to use bash (sh test dont work)
        # Run the executable, tee (split) the output between stdout and the results file,
        # then check the status of the testrunner executable to fail the build if needed.
        # This is necessary if we want result files for unit tests (since the executables dont write to files)
        action='{} | tee {} && test $$PIPESTATUS -eq 0'.format(testrunner.abspath, test_result_file.abspath)
    )

    valgrind_test_results += valgrind_env.MemCheck(
        source=testrunner,
        target=module_dir.File('memcheck_results.txt')
    )

Alias('memchecks', valgrind_test_results)
Alias('unit-tests', unit_test_results)
Default(unit_test_results)

"""
Instructions for compiling driver test applications.
"""

stm32_freertos_source = []
stm32_freertos_source += Glob(FREERTOS_DIR.Dir('Source').abspath + '/*.c')
stm32_freertos_source.append(FREERTOS_DIR.File('Source/portable/MemMang/heap_3.c'))
stm32_freertos_source.append(FREERTOS_DIR.File('Source/portable/ThirdParty/GCC/ARM_CM0/port.c'))


stm32_freertos_objs = []
for source_file in stm32_freertos_source:
    stm32_freertos_objs += stm32_comp_env.Object(source=source_file, target=source_file.abspath + '.stm32.o')

# Depends(freertos_objs, generated_dbc_source)

# stm32_freertos_objs += stm32_comp_env.Object(source=SRC_DIR.File('main.c'), target=SRC_DIR.File('main.stm32.o'))

test_apps = {}
for module_name, module_dir in driver_modules:
    # Compilation of test application main
    test_app_obj = stm32_comp_env.Object(module_dir.File('test_STM32_' + module_name + '.c'))
    
    # need list of strings not nodes for the next step
    # Which is to build the elf, which requires all the object files
    objs = [test_app_obj]
    for driver_obj in stm32_driver_objects.values():
        objs.append(driver_obj)
    for common_obj in stm32_common_objects.values():
        objs.append(common_obj)
    for app_obj in stm32_app_objects.values():
        objs.append(app_obj)
    objs.extend(stm32_comp_env.Object(DRIVER_DIR.File('test_app_startup.s')))

    objs.extend(stm32_lib_objs)
    objs.append(stm32_dbc_gen_obj)
    objs.append(stm32_freertos_objs)
    # objs.append(stm32_app_objects.values())


    # stm32 elf generation
    stm32_elf = stm32_comp_env.BuildElf(
        source=objs,
        target=module_dir.File('test_STM32_' + module_name + '.elf')
    )

    Clean(stm32_elf, REPO_ROOT_DIR.File('f29bms.map'))

    # stm32 hex generation
    test_apps[module_name] = stm32_comp_env.BuildHex(
        source=stm32_elf,
        target=BIN_DIR.File('test_STM32_' + module_name + '.bin')
    )

Alias('test-apps', test_apps.values())


"""
Instructions for building the Posix FreeRTOS program.
"""

freertos_source = []
freertos_source += Glob(FREERTOS_DIR.Dir('Source').abspath + '/*.c')
freertos_source.append(FREERTOS_DIR.File('Source/portable/MemMang/heap_3.c'))
freertos_source.append(FREERTOS_DIR.File('Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c'))
freertos_source.append(FREERTOS_DIR.File('Source/portable/ThirdParty/GCC/Posix/port.c'))

for module_name, module_dir in sim_modules:
    if module_name not in ['BmsSimHandle', 'BlfWriter']:
        freertos_source.append(module_dir.File(module_name + '.c'))

cpp_env = Environment(
    CC='g++',
    CPPPATH=LIBS_DIR.Dir('vector_blf/src/').abspath
)


freertos_comp_env = Environment(
    CC='gcc',
    CPPPATH=[sim_includes, freertos_include, module_path_names, APP_DIR.abspath, tool_paths, SIM_DIR.abspath, COMMON_DIR.abspath, DBC_DIR.abspath],
    CPPDEFINES=['projCOVERAGE_TEST=0', 'SIMULATION'],
    CPPFLAGS=['-O0', '-lm'],
    LINKFLAGS=['-pthread'],
    LIBS=['m']
    
)

# use the protobuf compiler to generate .pb.c and .pb.h files from the .proto file
# this is just like generating encoding/decoding code from a dbc
protoc_env = Environment(
    TOOLS=[TOOL_PROTOC]
)

bms_sim_proto = SIM_DIR.File('BmsSim.proto')

bms_sim_pb_source = protoc_env.GeneratePbSource(
    source=bms_sim_proto,
    target=SIM_DIR.File('BmsSim.pb.c')
)
Clean(bms_sim_pb_source, SIM_DIR.File('BmsSim.pb.h'))

freertos_objs = []
for source_file in freertos_source + Glob(LIBS_DIR.Dir('nanopb').abspath + '/*.c'):
    obj = freertos_comp_env.Object(source_file)
    freertos_objs += obj
    Depends(obj, bms_sim_pb_source)

#Depends(freertos_objs, generated_dbc_source)

freertos_objs += freertos_comp_env.Object(SIM_DIR.File('BmsSim.pb.c'))

freertos_objs += freertos_comp_env.Object(SRC_DIR.File('main.c'))

# (Binary constructed below)


"""
Instructions for building the f29bms program interface library.
"""

nanopb_source = Glob(LIBS_DIR.Dir('nanopb').abspath + '/*.c')

bms_sim_objs = {}
for module_name, module_dir in sim_modules:
    if module_name != 'BlfWriter': # BlfWriter is C++
        bms_sim_objs[module_name] = freertos_comp_env.SharedObject(module_dir.File(module_name + '.c'))
        Depends(bms_sim_objs[module_name], bms_sim_pb_source)

Depends(bms_sim_objs['BmsSimHandle'], generated_dbc_source)

# build C++ library wrapper (BlfWriter) and c++ library (vector_blf)

vector_blf_so = File('/usr/local/lib/libVector_BLF.so.2')

vector_blf_lib = Command(
    [vector_blf_so],
    [],
    ["cd libs/vector_blf && mkdir -p build && cd build && cmake .. && make && make install DESTDIR=.. && make install && /usr/sbin/ldconfig"]
)

bms_sim_objs['BlfWriter'] = cpp_env.SharedObject(SIM_DIR.File('BlfWriter/BlfWriter.cpp'))
Depends(bms_sim_objs['BlfWriter'], vector_blf_so)

# janky
freertos_comp_env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME']=1

# build the library
bms_sim_so = freertos_comp_env.SharedLibrary(
    source=[bms_sim_pb_source, bms_sim_objs.values(), nanopb_source, vector_blf_so, linux_dbc_gen_obj],
    target=SIM_DIR.File('libBmsSim.so')
)

Alias('sim-interface', bms_sim_so)

f29bms_bin = freertos_comp_env.Program(
    source=[freertos_objs, linux_driver_objects.values(), linux_app_objects.values(), linux_common_objects.values(), linux_dbc_gen_obj],
    target='f29bms'
)

Alias('sim', f29bms_bin)

"""
Open-loop tests.

These tests spin up the linux version of the f29bms program and imperatively feed it input
and validate output. 'Imperatively' indicates a one-after-the-other flow of applying input
and checking output. They're literally python scripts (pytest).
"""

pytest_env = Environment(tools=[TOOL_PYTEST])

# list of python files with pytests in them
pytest_source = []
for source in Glob(os.path.join(OPEN_LOOP_TESTS_DIR.abspath, '*test.py')):
    pytest_source.append(source)

pytest_results = []
for pytest in pytest_source:
    result = pytest_env.RunTestFile(
        source=pytest
    )
    pytest_results.append(result)

Depends(pytest_results, bms_sim_so)
Depends(pytest_results, f29bms_bin)
    
Alias('open-loop', pytest_results)

"""
The whole reason we're here: Instructions for building the f29bms binary.
That is, the binary that gets loaded onto the BMS hardware itself.
"""

# stm32_freertos_source = []
# stm32_freertos_source += Glob(FREERTOS_DIR.Dir('Source').abspath + '/*.c')
# stm32_freertos_source.append(FREERTOS_DIR.File('Source/portable/MemMang/heap_3.c'))
# stm32_freertos_source.append(FREERTOS_DIR.File('Source/portable/ThirdParty/GCC/ARM_CM0/port.c'))


# stm32_freertos_objs = []
# for source_file in stm32_freertos_source:
#     stm32_freertos_objs += stm32_comp_env.Object(source=source_file, target=source_file.abspath + '.stm32.o')

# #Depends(freertos_objs, generated_dbc_source)

stm32_freertos_objs += stm32_comp_env.Object(source=SRC_DIR.File('main.c'), target=SRC_DIR.File('main.stm32.o'))
f29bms_startup_obj = stm32_comp_env.Object(LIBS_DIR.File('stm32libs/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc_ride7/startup_stm32f0xx.s'))
# stm32 elf generation
f29bmself = stm32_comp_env.BuildElf(
    source=[stm32_freertos_objs, stm32_app_objects.values(), f29bms_startup_obj, stm32_driver_objects.values(), stm32_common_objects.values(), stm32_lib_objs, stm32_dbc_gen_obj],
    target=SRC_DIR.File('f29bms.elf')
)

Clean(stm32_elf, REPO_ROOT_DIR.File('f29bms.map'))

# stm32 hex generation
f29bmsbin = stm32_comp_env.BuildHex(
    source=f29bmself,
    target=BIN_DIR.File('f29bms.bin')
)

Alias('f29bms-bin', f29bmsbin)
