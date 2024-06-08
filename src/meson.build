folders = [
    'include/',
]

foreach f : folders
    project_inc += include_directories(f)
endforeach

project_src += files(
    'event/event.c',
    'event/io.c',
    'event/sys.c',

    'input/encoder.c',
    'input/quadrature.c',
    'input/switch.c',

    'lfo/lfo.c',

    'menu/menu.c',

    'midi/midi.c',

    'virtmap/virtmap.c',

)
