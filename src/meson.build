folders = [
    'include/',
]

foreach f : folders
project_inc += include_directories(f)
endforeach

project_src += files(
    'event/event.c',
    'event/sys.c',
    'event/io.c',

    'hal/avr/xmega/128a4u/boot.c',
    'hal/avr/xmega/128a4u/dma.c',
    'hal/avr/xmega/128a4u/gpio.c',
    'hal/avr/xmega/128a4u/init.c',
    'hal/avr/xmega/128a4u/usart.c',
    'hal/avr/xmega/128a4u/timer.c',

    'input/encoder.c',
    'input/quadrature.c',
    'input/switch.c',

    'lfo/lfo.c',

    'menu/menu.c',

    'midi/backend/midi_lufa.c',
    'midi/midi.c',

    'virtmap/virtmap.c',

    'usb/lufa/usb_lufa.c',


)
