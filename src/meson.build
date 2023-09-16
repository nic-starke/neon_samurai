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
    
    'Peripheral/DMA.c',

    'System/System.c',

    'USB/Descriptors.c',
    'USB/USB.c',
)