add_executable(getData getData.cpp)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(getData PRIVATE pico_stdlib pico_mpu_9250_object)

pico_enable_stdio_usb(getData 1)
pico_enable_stdio_uart(getData 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(getData)

# add url via pico_set_program_url
example_auto_set_url(getData)