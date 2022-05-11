del flashefr32custom.jlink
@echo off
(
:: Creation of the JLink script for the custom board
echo halt
echo loadbin BUILD/%1/GCC_ARM-DEBUG/%2.bin,0x0
echo setpc 0x0
echo reset
echo go
echo exit
) > flashefr32custom.jlink
@echo on
jlink -device efr32mg12p433f1024gm68 -autoconnect 1 -NoGui 0 -if SWD -Speed 4000 -CommandFile flashefr32custom.jlink