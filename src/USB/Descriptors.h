#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_

/* Includes: */
#include <avr/pgmspace.h>
#include <Drivers/USB/USB.h>

/* Macros: */
/** Endpoint address of the MIDI streaming data IN endpoint, for device-to-host
 * data transfers. */
#define MIDI_STREAM_IN_EPADDR (ENDPOINT_DIR_IN | 2)

/** Endpoint address of the MIDI streaming data OUT endpoint, for host-to-device
 * data transfers. */
#define MIDI_STREAM_OUT_EPADDR (ENDPOINT_DIR_OUT | 1)

/** Endpoint size in bytes of the Audio isochronous streaming data IN and OUT
 * endpoints. */
#define MIDI_STREAM_EPSIZE 64

/* Type Defines: */
/** Type define for the device configuration descriptor structure. This must be
 * defined in the application code, as the configuration descriptor contains
 * several sub-descriptors which vary between devices, and which describe the
 * device's usage to the host.
 */
typedef struct
{
	USB_Descriptor_Configuration_Header_t Config;

	// MIDI Audio Control Interface
	USB_Descriptor_Interface_t			Audio_ControlInterface;
	USB_Audio_Descriptor_Interface_AC_t Audio_ControlInterface_SPC;

	// MIDI Audio Streaming Interface
	USB_Descriptor_Interface_t				  Audio_StreamInterface;
	USB_MIDI_Descriptor_AudioInterface_AS_t	  Audio_StreamInterface_SPC;
	USB_MIDI_Descriptor_InputJack_t			  MIDI_In_Jack_Emb;
	USB_MIDI_Descriptor_InputJack_t			  MIDI_In_Jack_Ext;
	USB_MIDI_Descriptor_OutputJack_t		  MIDI_Out_Jack_Emb;
	USB_MIDI_Descriptor_OutputJack_t		  MIDI_Out_Jack_Ext;
	USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
	USB_MIDI_Descriptor_Jack_Endpoint_t		  MIDI_In_Jack_Endpoint_SPC;
	USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
	USB_MIDI_Descriptor_Jack_Endpoint_t		  MIDI_Out_Jack_Endpoint_SPC;
} USB_Descriptor_Configuration_t;

/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t
{
	INTERFACE_ID_AudioControl = 0, /**< Audio control interface descriptor ID */
	INTERFACE_ID_AudioStream  = 1, /**< Audio streaming interface descriptor ID */
};

/** Enum for the device string descriptor IDs within the device. Each string
 * descriptor should have a unique ID index associated with it, which can be
 * used to refer to the string from other descriptors.
 */
enum StringDescriptors_t
{
	STRING_ID_Language	   = 0, /**< Supported Languages string descriptor ID (must be zero) */
	STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
	STRING_ID_Product	   = 2, /**< Product string ID */
	STRING_ID_Serial	   = 3, /**< Serial string ID */
};

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint16_t wIndex,
									const void** const DescriptorAddress) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif /* DESCRIPTORS_H_ */