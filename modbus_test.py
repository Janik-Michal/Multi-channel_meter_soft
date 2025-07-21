import minimalmodbus
import time

instrument: minimalmodbus.Instrument = minimalmodbus.Instrument('COM4', 1,debug = True)

instrument.serial.baudrate = 115200
instrument.serial.bytesize = 8
instrument.serial.parity   = 'N'
instrument.serial.stopbits = 1
instrument.serial.timeout  = 2
instrument.mode = minimalmodbus.MODE_RTU   # tryb RTU (lub MODE_ASCII)

instrument.write_register(33, 2, functioncode=6)
time.sleep(0.2)
instrument.address = 2

instrument.write_register(26, 400, functioncode=6, signed=True)
time.sleep(0.5) 
instrument.write_register(27, 510, functioncode=6, signed=True)
time.sleep(0.5) 
instrument.write_register(28, 470, functioncode=6, signed=True)
time.sleep(0.5) 
instrument.write_register(29, 460, functioncode=6, signed=True)
time.sleep(0.5) 
instrument.write_register(30, 410, functioncode=6, signed=True)
time.sleep(0.5) 

while(1):
    def to_signed_16bit(val):
        if val > 0x7FFF:
            return val - 0x10000
        else:
            return val

    raw = instrument.read_registers(0, 6)
    temperatures = [to_signed_16bit(x) / 100.0 for x in raw]
    print("Temperatures [°C]:", temperatures)
    time.sleep(0.5) 


