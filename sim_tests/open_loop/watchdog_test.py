from BmsSim import sim
import pytest

def test_watchdog(sim):
    """
    Basic test to make sure watchdog task works. Test to make sure 
    Check window (such as 43-56 seconds) make sure false and then gets pet
    """
    sim.start_logging('sim_traces/watchdog.blf')
    pet = None
    # for i in range(0, 43000):
    #     sim.tick(sim)
    # for i in range(0, 11000):
    #     sim.tick(self)
    for i in range(0, 1000):
        sim.tick()
    for i in range(0, 10000):
        pet = None
        sim.tick()
        if sim.get_watchdog_pet():
            pet = True
        assert pet, "Watchdog not pet"
        
""" If I comment out the TaskWatchdog_pet in the 1Hz task, assert fails. If I comment it out in the 1KHz or 10 Hz
tasks, then there is a wierd error in one of the other tests, maybe something to do with adding the pet to the
sim data getting sent, but then not being set after watchdog times out. When the TaskWatchdog_pet is added back,
the others test run with no errors."""
    # for i in range(0, 43000):
    #     sim.tick(sim)
    # for i in range(0, 11000):
    #     sim.tick(self)
    #     if pet == False:
    #         if sim.get_watchdog_pet():
    #             pet = True
    #     elif pet == True:
    #         break
    #     elif not sim.get_watchdog_pet():
    #         pet = False
    # assert pet, "Watchdog not pet"
