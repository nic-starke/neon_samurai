project('lufa', 'c')

lufa_inc = [
    '.',
]

lufa_src = files(
    'LUFA/Platform/UC3/InterruptManagement.c',
    'LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_RW.c',
    'LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_W.c',
    'LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_R.c',
    'LUFA/Drivers/USB/Core/XMEGA/USBInterrupt_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/USBController_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/PipeStream_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/Pipe_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/Host_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/EndpointStream_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/Endpoint_XMEGA.c',
    'LUFA/Drivers/USB/Core/XMEGA/Device_XMEGA.c',
    'LUFA/Drivers/USB/Core/UC3/Template/Template_Pipe_RW.c',
    'LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_RW.c',
    'LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_W.c',
    'LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_R.c',
    'LUFA/Drivers/USB/Core/UC3/USBInterrupt_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/USBController_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/PipeStream_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/Pipe_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/Host_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/EndpointStream_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/Endpoint_UC3.c',
    'LUFA/Drivers/USB/Core/UC3/Device_UC3.c',
    'LUFA/Drivers/USB/Core/AVR8/Template/Template_Pipe_RW.c',
    'LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_RW.c',
    'LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_W.c',
    'LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_R.c',
    'LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/PipeStream_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/Pipe_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/Host_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c',
    'LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c',
    'LUFA/Drivers/USB/Core/USBTask.c',
    'LUFA/Drivers/USB/Core/HostStandardReq.c',
    'LUFA/Drivers/USB/Core/Events.c',
    'LUFA/Drivers/USB/Core/DeviceStandardReq.c',
    'LUFA/Drivers/USB/Core/ConfigDescriptors.c',
    'LUFA/Drivers/USB/Class/Host/StillImageClassHost.c',
    'LUFA/Drivers/USB/Class/Host/RNDISClassHost.c',
    'LUFA/Drivers/USB/Class/Host/PrinterClassHost.c',
    'LUFA/Drivers/USB/Class/Host/MIDIClassHost.c',
    'LUFA/Drivers/USB/Class/Host/MassStorageClassHost.c',
    'LUFA/Drivers/USB/Class/Host/HIDClassHost.c',
    'LUFA/Drivers/USB/Class/Host/CDCClassHost.c',
    'LUFA/Drivers/USB/Class/Host/AudioClassHost.c',
    'LUFA/Drivers/USB/Class/Host/AndroidAccessoryClassHost.c',
    'LUFA/Drivers/USB/Class/Device/RNDISClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/PrinterClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/MIDIClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/MassStorageClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/HIDClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/CDCClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/CCIDClassDevice.c',
    'LUFA/Drivers/USB/Class/Device/AudioClassDevice.c',
    'LUFA/Drivers/USB/Class/Common/HIDParser.c'
)

compiler_args=[
'-DUSE_LUFA_CONFIG_HEADER=1',
]

add_project_arguments(compiler_args , language : 'c')

lufa_lib = library('lufa', lufa_src, include_directories: lufa_inc)
lufa_dep = declare_dependency(include_directories : lufa_inc, sources : lufa_src)
