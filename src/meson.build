folders = [
    'include/',
    # 'Comms',
    # 'Comms/Modules',
	# 'Display',
    # 'Input',
    # 'MIDI',
	# 'Peripheral',
	# 'System',
    # 'USB',
    # 'Utility',
    # 'LUFA',
]

foreach f : folders
project_inc += include_directories(f)
endforeach

project_src += files(
    'main.c',

    'application/io/encoder.c',
    
    'board/board.c',

    'hal/avr/xmega/128a4u/dma.c',
    'hal/avr/xmega/128a4u/gpio.c',
    'hal/avr/xmega/128a4u/usart.c',
    'hal/avr/xmega/128a4u/timer.c',

    'drivers/quad_encoder.c',
    'drivers/gpio_switch.c',
    
    'system/system.c',
    'system/os.c',
    'system/event.c',
)