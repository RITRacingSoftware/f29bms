import ctypes
import time
import signal
import os
import pytest
import cantools

"""
Python wrapping of BmsSim.c functions.
"""
class BmsSim:
    def __init__(self):
        signal.signal(signal.SIGINT, self._interrupt_signal_handler)
        self.can_db = cantools.database.load_file('src/app/CAN/f29bms_dbc.dbc')

        self.signals = {}

        self.clib = ctypes.CDLL('src/sim/libBmsSim.so') # must be relative from root of repo, thats where the test scripts are ran from
        self.clib.BmsSim_init()

    def start(self):
        self.clib.BmsSim_start()
        time.sleep(1) # give time for the sim to initialize
    
    def start_logging(self, filename):
        self.clib.BmsSim_begin_logging(ctypes.c_char_p(filename.encode("utf-8")))

    def stop(self):
        time.sleep(1) # give time for the sim to stop processing input
        self.clib.BmsSim_stop()
    
    # makes sure the socket gets closed on interrupt
    def _interrupt_signal_handler(self, signo, arg1):
        self.stop()
        exit(0)

    def tick(self):
        self.clib.BmsSim_tick()

        # get all the new CAN messages and update the signal database
        can_data = ctypes.c_int64()
        can_id = 0
        while(can_id != -1):
            can_id = self.clib.BmsSim_next_can_msg(ctypes.byref(can_data))
            if (can_id != -1):
                self.signals.update(self.can_db.decode_message(can_id & 0x1FFFFFFF, can_data.value.to_bytes(8, "little", signed=True)))

    def stage_temp_info(self, therm_index, voltage):
        self.clib.BmsSim_set_temp_info(therm_index, ctypes.c_float(voltage))
    
    def stage_cell_info(self, cell_index, voltage, is_draining):
        self.clib.BmsSim_set_cell_info(cell_index, ctypes.c_float(voltage), is_draining)
    
    def get_status_led(self):
        return self.clib.BmsSim_get_status_led()
    
    def get_shutdown_line(self):
        return self.clib.BmsSim_get_shutdown_line()
        
    def get_watchdog_pet(self):
        return self.clib.BmsSim_get_watchdog_pet()
    
    def get_charge_enable(self):
        return self.clib.BmsSim_get_charge_enable()
    
    def set_charger_available(self, c_a):
        self.clib.BmsSim_set_charger_available(c_a)

    def set_current(self, current):
        self.clib.BmsSim_set_current(ctypes.c_float(current))
    
    def read_drain_state(self, index):
        return self.clib.BmsSim_read_drain_state(index)

can_db_loaded = False
@pytest.fixture
def sim():
    """
    Think of this as a "Before Each" function from other testing frameworks.
    If a pytest function has an argument "sim", this function is run before the test and its 
    return value is "sim".

    Starts the f29bms process in a nominal steady state, defined as no faults being active
    and no draining occuring. Also starts fully charged.
    """
    
    s = BmsSim()
    s.start()

    for i in range(0, 90):
        s.stage_cell_info(i, 4.2, False)
        
    for i in range(0, 20):
        s.stage_temp_info(i, 1.65)

    s.set_current(0)
    s.set_charger_available(False) 
    s.tick() # essential- applies these changes so tests dont try to initially overwrite them

    yield s

    s.stop()
