Custom DMG boot ROM used by this emulator

Its derived from the original gameboy boot rom behaviour,
but modified the logo  to scroll horizontally 

This is not the original Nintendo boot ROM

Assembled using RGBDS

rgbasm bootROM.asm -o bootROM.o      
rgblink bootROM.o -o bootROM.cb