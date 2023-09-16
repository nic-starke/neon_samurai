folders = [
	'Display',
	'Peripheral',
	'System',
    'USB',
    'Utility',
    'LUFA',
]

foreach f : folders
project_inc += include_directories(f)
endforeach

project_src += files(
    'main.c',

    'Display/Display.c',

    'Peripheral/DMA.c',
    'Peripheral/GPIO.c',
    'Peripheral/Peripheral.c',
    'Peripheral/USART.c',

    'System/System.c',

    'USB/Descriptors.c',
    'USB/USB.c',
)