import minimalmodbus
import time

instrument: minimalmodbus.Instrument = minimalmodbus.Instrument('COM4', 1,debug = True)

instrument.serial.baudrate = 115200
instrument.serial.bytesize = 8
instrument.serial.parity   = 'N'
instrument.serial.stopbits = 1
instrument.serial.timeout  = 2
instrument.mode = minimalmodbus.MODE_RTU   # tryb RTU (lub MODE_ASCII)

# instrument.write_register(34, 16, functioncode=6) # zmiana buadrate: reg 16 - 9600; reg 17 - 19200; reg 18 - 38400; reg 19 - 57600; reg 20 - 115200
# time.sleep(1)                                     # wymagany reset zasilania


# instrument.write_register(33, 1, functioncode=6)  # zmiana ID Slave: reg 33 ; ID: 1-247
# time.sleep(0.2)
# instrument.address = 1

# instrument.write_register(26, 390, functioncode=6, signed=True)  # kalibracja ofsetu ADC reg 26-31 ; wartosc 10 zmiana o 0.01 *C
# time.sleep(0.5) 
# instrument.write_register(27, 500, functioncode=6, signed=True)
# time.sleep(0.5) 
# instrument.write_register(28, 490, functioncode=6, signed=True)
# time.sleep(0.5) 
# instrument.write_register(29, 490, functioncode=6, signed=True)
# time.sleep(0.5) 
# instrument.write_register(30, 470, functioncode=6, signed=True)
# time.sleep(0.5) 

try:
    while True:
        def to_signed_16bit(val):
            if val > 0x7FFF:
                return val - 0x10000
            else:
                return val

        raw = instrument.read_registers(0, 6)                       # odczyt rejestrów 0-6 (5 kanalow adc, 1 czujnik na plytce)
        temperatures = [to_signed_16bit(x) / 100.0 for x in raw]
        print("Temperatures [°C]:", temperatures)
        time.sleep(0.5) 

except KeyboardInterrupt:
    print("Close")
    time.sleep(1)

