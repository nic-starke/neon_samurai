folders = [
	'Display',
    'Input',
    'MIDI',
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

    'Display/Colour.c',
    'Display/Display.c',
    'Display/EncoderDisplay.c',
    'Display/fast_hsv2rgb_8bit.c',

    'Input/Encoder.c',
    'Input/Input.c',
    'Input/SideSwitch.c',

    'MIDI/MIDI.c',
    
    'Peripheral/DMA.c',
    'Peripheral/GPIO.c',
    'Peripheral/Timer.c',
    'Peripheral/USART.c',

    'System/Data.c',
    'System/SoftTimer.c',
    'System/System.c',

    'USB/Descriptors.c',
    'USB/VirtualSerial.c',
    'USB/USB.c',
)