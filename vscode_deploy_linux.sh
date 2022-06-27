mbed deploy
# Creation of the JLink script for the custom board
touch flashefr32custom.jlink
echo "halt" > flashefr32custom.jlink
echo "loadbin BUILD/$1/GCC_ARM/$2.bin,0x0" >> flashefr32custom.jlink
echo "setpc 0x0" >> flashefr32custom.jlink
echo "reset" >> flashefr32custom.jlink
echo "go" >> flashefr32custom.jlink
echo "exit" >> flashefr32custom.jlink
# Creation of the JLink script for the custom board (debug config)
touch flashefr32custom_debug.jlink
echo "halt" > flashefr32custom_debug.jlink
echo "loadbin BUILD/$1/GCC_ARM-DEBUG/$2.bin,0x0" >> flashefr32custom_debug.jlink
echo "setpc 0x0" >> flashefr32custom_debug.jlink
echo "reset" >> flashefr32custom_debug.jlink
echo "go" >> flashefr32custom_debug.jlink
echo "exit" >> flashefr32custom_debug.jlink