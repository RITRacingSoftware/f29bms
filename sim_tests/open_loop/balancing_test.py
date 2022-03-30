from BmsSim import sim
import pytest

def test_balancing(sim):
    """
    Verify cells balance when they are above the charging threshold and when they are
    too far ahead of other cells.
    """
    sim.start_logging('sim_traces/balancing.blf')

    # shouldn't start with AIRs closed (assuming on charger, so this closes the AIRs)
    assert sim.get_charge_enable() == False

    # start charging
    for i in range(0, 90):
        sim.stage_cell_info(i, 3.9, False)
    sim.set_charger_available(1)

    charging = None
    for i in range(0, 8000):
        sim.tick()
        if 'BmsChargeRequest_Control' in sim.signals:
            charging = not sim.signals['BmsChargeRequest_Control'] # yes this is active low for some reason
    
    assert charging, "Charging did not begin"

    # now set every other cell to a voltage that should trigger balancing
    for i in range(1, 91, 2):
        sim.stage_cell_info(i, 4.31, False)

    # cells that are above the charging threshold should be balanced even if they arent that far behind the largest voltage
    sim.stage_cell_info(0, 4.29, False)

    # enter balancing
    for i in range(0, 10000):
        sim.tick()
    
    for i in range(2, 90, 2):
        assert sim.read_drain_state(i) == False, "Wrong cell drained"
    for i in range(1, 91, 2):
        assert sim.read_drain_state(i) == True, "Cell not drained when appropriate"
    
    # should have AIRs closed when balancing
    assert sim.get_charge_enable() == True

    assert sim.read_drain_state(0) == True, "Cell not drained when appropriate"
