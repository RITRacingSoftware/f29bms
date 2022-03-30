from BmsSim import sim
import pytest

"""
Verify that the current limiting is functioning properly. Tests that the current limit isn't set above the 
max current limit, that the current limit decreases when a cell drops below the threshold voltage, and that the
current limit is decreased when the cell voltages are low but above the threshold voltage
"""
def test_current_limiting(sim):
    sim.start_logging('sim_traces/current_limiting.blf')

    # Set cell voltage, current limit should be max current
    for i in range(0, 90):
        sim.stage_cell_info(i, 4.25, True)

    sim.set_current(0)
    
    for i in range(0,1000):
        sim.tick()

    # Set the max and min current limit, will need to be changed when the max current limit is changed in f29bmsconfig.h
    max_current_limit = 230
    min_current_limit = 50

    # Run for 1000 ticks and make sure the current limit is set to the max
    for i in range(0,1000):
        sim.tick()
    assert 'BmsMaxDischargeCurrent' in sim.signals, "MaxDischargeCurrent never sent"
    for i in range(0,5000):
        sim.tick()
        current_limit = sim.signals['BmsMaxDischargeCurrent']
        if current_limit != max_current_limit:
            break
    assert current_limit == max_current_limit, "Current limit not set to max when cells are charged."

    # Set voltage and current lower to a state that should limit current
    sim.stage_cell_info(1, 3.3, True)
    sim.set_current(50)

    # Run for 1000 ticks and make sure the current limit is set to a value between the instantaneous
    # current and the max current limit
    for i in range(0,5000):
        sim.tick()
        current_limit = sim.signals['BmsMaxDischargeCurrent']

    assert current_limit < max_current_limit, "Current not limited at low cell voltage."
    assert current_limit > 50, "Current limit set too low at low cell voltage."

    # Set voltage and current to a very drained state to make sure the current limit doesn't go below the minimum limit
    sim.stage_cell_info(1, 3.12, True)
    sim.set_current(20)

    for i in range(0,5000):
        sim.tick()

    # Run for 1000 ticks and make sure the current limit is set to the minimum current limit value
    for i in range(0,5000):
        sim.tick
        current_limit = sim.signals['BmsMaxDischargeCurrent']

    assert 'CountActiveCorrectionCount' in sim.signals, "CountActiveCorrectionCount never sent"
    assert current_limit == min_current_limit, "Current limit isn't set to minimum current limit when cell at very low voltage."