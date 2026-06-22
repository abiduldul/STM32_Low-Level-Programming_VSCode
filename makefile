# Define phony targets so Make doesn't confuse them with actual files
.PHONY: all build-mertani-2.5 clean

# Default rule when you just type 'make'
all: build-mertani-2.5

flash:
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program build/mertani-2.5/firmware.elf verify reset exit"

# Rule to configure and build for the Mertani 2.5 hardware
build-mertani-2.5:
	@echo "--- Configuring and Building for Mertani 2.5 Hardware ---"
	cmake --preset mertani-2.5
	cmake --build --preset build-mertani-2.5
	arm-none-eabi-size build/mertani-2.5/firmware.elf

# Rule to completely wipe the generated build files
clean:
	@echo "--- Cleaning build directory ---"
	rm -rf build/