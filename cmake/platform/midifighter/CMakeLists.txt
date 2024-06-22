# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include(FetchContent)

# LUFA USB Library

FetchContent_Declare(
  LUFA
  GIT_REPOSITORY https://github.com/nic-starke/lufa.git
  GIT_TAG master
  GIT_SHALLOW true
)

FetchContent_MakeAvailable(LUFA)
FetchContent_GetProperties(LUFA)
if(NOT lufa_POPULATED)
  FetchContent_Populate(LUFA)
endif()

# message(STATUS "LUFA source directory: ${lufa_SOURCE_DIR}")

# Add LUFA sources to a variable
set(LUFA_SOURCES
    ${lufa_SOURCE_DIR}/LUFA/Platform/UC3/InterruptManagement.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_RW.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_W.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_R.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/USBInterrupt_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/USBController_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/PipeStream_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Pipe_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Host_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/EndpointStream_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Endpoint_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Device_XMEGA.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Pipe_RW.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_RW.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_W.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_R.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/USBInterrupt_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/USBController_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/PipeStream_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Pipe_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Host_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/EndpointStream_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Endpoint_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Device_UC3.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Pipe_RW.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_RW.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_W.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_R.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/PipeStream_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Pipe_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Host_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/USBTask.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/HostStandardReq.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/Events.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/DeviceStandardReq.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/ConfigDescriptors.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/StillImageClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/RNDISClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/PrinterClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/MIDIClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/MassStorageClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/HIDClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/CDCClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/AudioClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/AndroidAccessoryClassHost.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/RNDISClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/PrinterClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/MIDIClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/MassStorageClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/HIDClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/CDCClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/CCIDClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/AudioClassDevice.c
    ${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Common/HIDParser.c
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Executable
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_executable(mfexe)
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/midifighter/*.c")

target_compile_definitions(mfexe PRIVATE USE_LUFA_CONFIG_HEADER=1)
target_sources(mfexe PRIVATE ${PLATFORM_SOURCES})
target_sources(mfexe PRIVATE ${LUFA_SOURCES})

target_include_directories(mfexe PRIVATE
  ${CMAKE_SOURCE_DIR}/src/include/platform/midifighter
  ${lufa_SOURCE_DIR}
)

target_link_libraries(mfexe PRIVATE common hal_xmega128a4u)
