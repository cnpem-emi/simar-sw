# SPIxxCONPy3

## Changes

- `spicon.transfer` now takes only read-only bytes-like objects as an argument, as Python2 strings were closer to bytes objs. than actual Unicode objects. 
    - Example: `spicon.transfer(spi, '\x83\x10')` becomes `spicon.transfer(spi, b'\x83\x10')`
- All values returned by `spicon.transfer` will also only take the form of bytes objects. 
    - Example: `_data = map(ord, spicon.transfer(spi, chr(0x83 + channels*4) + chr(0x10)))` becomes `_data = spicon.transfer(spi, bytes([0x83 + _channels[index+1]*4, 0x10]))`