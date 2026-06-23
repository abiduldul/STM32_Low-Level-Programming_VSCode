cmake_minimum_required(VERSION 3.13)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(mertani_firmware C ASM)

set(THREADX_ARCH "cortex_m4" CACHE STRING "Architecture" FORCE)
set(THREADX_TOOLCHAIN "gnu" CACHE STRING "Toolchain" FORCE)

add_subdirectory(middlewares/threadx)
add_compile_definitions(USE_HAL_DRIVER STM32L4P5xx)

include_directories(after PUBLIC
    middlewares/threadx/common/inc                 
    middlewares/threadx/ports/cortex_m4/gnu/inc   
    drivers/CMSIS_6/CMSIS/Core/Include
    drivers/cmsis-device-l4/Include
    drivers/stm32l4xx-hal-driver/Inc
    core/include/
    bsp/include/
)

set(APP_SOURCES
    middlewares/threadx/ports/cortex_m4/gnu/example_build/tx_initialize_low_level.S
    startup.s

    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal.c 
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_cortex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_gpio.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_rcc.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_rcc_ex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_uart.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_uart_ex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_dma.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_dma_ex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_pwr.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_pwr_ex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_sd.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_sd_ex.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_ll_sdmmc.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_spi.c
    drivers/stm32l4xx-hal-driver/Src/stm32l4xx_hal_spi_ex.c
    
    drivers/cmsis-device-l4/Source/Templates/system_stm32l4xx.c
)

file(GLOB MERTANI_SOURCES1 "core/source/*.c")
file(GLOB MERTANI_SOURCES2 "bsp/source/*.c")
list(APPEND APP_SOURCES ${MERTANI_SOURCES1} ${MERTANI_SOURCES2})

add_executable(firmware.elf ${APP_SOURCES}) 

target_link_libraries(firmware.elf PRIVATE threadx)

target_link_options(firmware.elf PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -Wl,--gc-sections
    --specs=nosys.specs  # THE FIX: Provides empty stubs for _sbrk and other OS calls
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map)