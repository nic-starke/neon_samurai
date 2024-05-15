folders = [
    'include/',
]

foreach f : folders
project_inc += include_directories(f)
endforeach

project_src += files(
    'core/core_switch.c',


    'event/event.c',
    'event/event_core.c',
    'event/event_io.c',

    'io/io_device.c',
    'io/encoder/encoder_quadrature.c',

    'midi/midi.c',
    'midi/backend/midi_lufa.c',

    'usb/lufa/usb_lufa.c',

    'virtmap/virtmap.c',

    'hal/avr/xmega/128a4u/boot.c',
    'hal/avr/xmega/128a4u/dma.c',
    'hal/avr/xmega/128a4u/gpio.c',
    'hal/avr/xmega/128a4u/init.c',
    'hal/avr/xmega/128a4u/usart.c',
    'hal/avr/xmega/128a4u/timer.c',
)
