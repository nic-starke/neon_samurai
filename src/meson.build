folders = [
    'include/',
]

foreach f : folders
project_inc += include_directories(f)
endforeach

project_src += files(
    'core/core_encoder.c',
    'core/core_event.c',
    'core/core_switch.c',
    
    'hal/avr/xmega/128a4u/boot.c',
    'hal/avr/xmega/128a4u/dma.c',
    'hal/avr/xmega/128a4u/gpio.c',
    'hal/avr/xmega/128a4u/init.c',
    'hal/avr/xmega/128a4u/usart.c',
    'hal/avr/xmega/128a4u/timer.c',
)