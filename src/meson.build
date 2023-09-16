folders = [
	'Display',
    'Input',
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
    'Display/EncoderDisplay.c',

    'Input/Encoder.c',
    'Input/Input.c',
    'Input/SideSwitch.c',    
    
    'Peripheral/DMA.c',
    'Peripheral/GPIO.c',
    'Peripheral/Timer.c',
    'Peripheral/USART.c',

    'System/SoftTimer.c',
    'System/System.c',

    'USB/Descriptors.c',
    'USB/USB.c',
)