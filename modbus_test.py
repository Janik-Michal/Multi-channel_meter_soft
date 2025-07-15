import minimalmodbus
import time

instrument: minimalmodbus.Instrument = minimalmodbus.Instrument('COM4', 1,debug = True)

instrument.serial.baudrate = 115200
instrument.serial.bytesize = 8
instrument.serial.parity   = 'N'
instrument.serial.stopbits = 1
instrument.serial.timeout  = 2
instrument.mode = minimalmodbus.MODE_RTU   # tryb RTU (lub MODE_ASCII)

while(1):
    def to_signed_16bit(val):
        if val > 0x7FFF:
            return val - 0x10000
        else:
            return val

    raw = instrument.read_registers(0, 5)
    temperatures = [to_signed_16bit(x) / 100.0 for x in raw]
    print("Temperatures [°C]:", temperatures)
    time.sleep(0.5) 


