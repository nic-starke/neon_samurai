/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>

#include "system/types.h"
#include "usb/usb.h"
#include "usb/usb_lufa.h"
#include "system/print.h"

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Drivers/USB/Class/Common/MIDIClassCommon.h"
#include "LUFA/Drivers/USB/Class/Device/MIDIClassDevice.h"
#include "LUFA/Drivers/USB/Class/Device/CDCClassDevice.h" // Add CDC include
#include "LUFAConfig.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define USB_VENDOR_ID								0x2580
#define USB_PRODUCT_ID							0x0007
#define USB_HID_EPSIZE							32
#define USB_KEYBOARD_EPSIZE					8
#define USB_MOUSE_EPSIZE						8
#define USB_USB_MIDI_STREAM_EPSIZE	64
#define USB_CDC_NOTIFICATION_EPSIZE 8
#define USB_CDC_EPSIZE							16
#define USB_MIDI_POLLING_INTERVAL		0x05

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern USB_ClassInfo_MIDI_Device_t lufa_usb_midi_device;
#ifdef ENABLE_CONSOLE
extern USB_ClassInfo_CDC_Device_t lufa_usb_cdc_device;
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum usb_desc_str {
	DESC_STR_LANG = 0,
	DESC_STR_MF		= 1,
	DESC_STR_PROD = 2,
	DESC_STR_SER	= 3,

	DESC_STR_NB,
};

struct usb_descriptor {
	USB_Descriptor_Configuration_Header_t Config;

	USB_Descriptor_Interface_Association_t Audio_Interface_Association;
	// MIDI Audio Control Interface
	USB_Descriptor_Interface_t						 Audio_ControlInterface;
	USB_Audio_Descriptor_Interface_AC_t		 Audio_ControlInterface_SPC;

	// MIDI Audio Streaming Interface
	USB_Descriptor_Interface_t								Audio_StreamInterface;
	USB_MIDI_Descriptor_AudioInterface_AS_t		Audio_StreamInterface_SPC;
	USB_MIDI_Descriptor_InputJack_t						MIDI_In_Jack_Emb;
	USB_MIDI_Descriptor_InputJack_t						MIDI_In_Jack_Ext;
	USB_MIDI_Descriptor_OutputJack_t					MIDI_Out_Jack_Emb;
	USB_MIDI_Descriptor_OutputJack_t					MIDI_Out_Jack_Ext;
	USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
	USB_MIDI_Descriptor_Jack_Endpoint_t				MIDI_In_Jack_Endpoint_SPC;
	USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
	USB_MIDI_Descriptor_Jack_Endpoint_t				MIDI_Out_Jack_Endpoint_SPC;

#ifdef HID_ENABLE
	// Keyboard/Mouse
	USB_Descriptor_Interface_t HID_Interface;
	USB_HID_Descriptor_HID_t	 HID_Descriptor;
	USB_Descriptor_Endpoint_t	 HID_In_Endpoint;
#endif

#ifdef ENABLE_CONSOLE
	// Virtual Serial
	// CDC Control Interface
	USB_Descriptor_Interface_Association_t CDC_Interface_Association;
	USB_Descriptor_Interface_t						 CDC_CCI_Interface;
	USB_CDC_Descriptor_FunctionalHeader_t	 CDC_Functional_Header;
	USB_CDC_Descriptor_FunctionalACM_t		 CDC_Functional_ACM;
	USB_CDC_Descriptor_FunctionalUnion_t	 CDC_Functional_Union;
	USB_Descriptor_Endpoint_t							 CDC_NotificationEndpoint;
	// CDC Data Interface
	USB_Descriptor_Interface_t						 CDC_DCI_Interface;
	USB_Descriptor_Endpoint_t							 CDC_DataOutEndpoint;
	USB_Descriptor_Endpoint_t							 CDC_DataInEndpoint;
#endif
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma GCC diagnostic ignored "-Wpedantic"

PROGMEM static const USB_Descriptor_String_t desc_str_lang =
		USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

PROGMEM static const USB_Descriptor_String_t desc_str_mf =
		USB_STRING_DESCRIPTOR(L"NEON");

PROGMEM static const USB_Descriptor_String_t desc_str_prod =
		USB_STRING_DESCRIPTOR(L"SAMURAI");

PROGMEM static const USB_Descriptor_String_t desc_str_ser =
		USB_STRING_DESCRIPTOR(L"2024");

#pragma GCC diagnostic pop

#ifdef ENABLE_CONSOLE
USB_ClassInfo_CDC_Device_t lufa_usb_cdc_device = {
		.Config =
				{
						.ControlInterfaceNumber = 0,
						.DataINEndpoint =
								{
										.Address = (CDC_IN_EPNUM | ENDPOINT_DIR_IN),
										.Size		 = USB_CDC_EPSIZE,
										.Banks	 = 1,
								},
						.DataOUTEndpoint =
								{
										.Address = (CDC_OUT_EPNUM | ENDPOINT_DIR_OUT),
										.Size		 = USB_CDC_EPSIZE,
										.Banks	 = 1,
								},
						.NotificationEndpoint =
								{
										.Address = (CDC_NOTIFICATION_EPNUM | ENDPOINT_DIR_IN),
										.Size		 = USB_CDC_NOTIFICATION_EPSIZE,
										.Banks	 = 1,
								},
				},
};
#endif

PROGMEM static const USB_Descriptor_Device_t desc_device = {
		.Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
		.USBSpecification = VERSION_BCD(1, 1, 0),

#ifdef ENABLE_CONSOLE
		.Class		= USB_CSCP_IADDeviceClass,
		.SubClass = USB_CSCP_IADDeviceSubclass,
		.Protocol = USB_CSCP_IADDeviceProtocol,
#else
		.Class		= USB_CSCP_NoDeviceClass,
		.SubClass = USB_CSCP_NoDeviceSubclass,
		.Protocol = USB_CSCP_NoDeviceProtocol,
#endif

		.Endpoint0Size				= FIXED_CONTROL_ENDPOINT_SIZE,
		.VendorID							= USB_VENDOR_ID,
		.ProductID						= USB_PRODUCT_ID,
		.ReleaseNumber				= VERSION_BCD(0, 0, 1),
		.ManufacturerStrIndex = DESC_STR_MF,
		.ProductStrIndex			= DESC_STR_PROD,
		// .SerialNumStrIndex		= DESC_STR_SER,
		.SerialNumStrIndex		= USE_INTERNAL_SERIAL,

		.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS,
};

PROGMEM static const struct usb_descriptor desc_cfg = {
		.Config =
				{
						.Header =
								{
										.Size = sizeof(USB_Descriptor_Configuration_Header_t),
										.Type = DTYPE_Configuration,
								},
						.TotalConfigurationSize = sizeof(struct usb_descriptor),
						.TotalInterfaces				= NUM_USB_INTERFACES,
						.ConfigurationNumber		= 1,
						.ConfigurationStrIndex	= NO_DESCRIPTOR,
						.ConfigAttributes				= (USB_CONFIG_ATTR_RESERVED),
						.MaxPowerConsumption		= USB_CONFIG_POWER_MA(480),
				},

		.Audio_Interface_Association =
				{
						.Header =
								{
										.Size = sizeof(USB_Descriptor_Interface_Association_t),
										.Type = DTYPE_InterfaceAssociation,
								},
						.FirstInterfaceIndex = MIDI_AC_INTERFACE,
						.TotalInterfaces		 = 2,
						.Class							 = AUDIO_CSCP_AudioClass,
						.SubClass						 = AUDIO_CSCP_ControlSubclass,
						.Protocol						 = AUDIO_CSCP_ControlProtocol,
						.IADStrIndex				 = NO_DESCRIPTOR,
				},
		.Audio_ControlInterface =
				{
						.Header =
								{
										.Size = sizeof(USB_Descriptor_Interface_t),
										.Type = DTYPE_Interface,
								},
						.InterfaceNumber	 = MIDI_AC_INTERFACE,
						.AlternateSetting	 = 0,
						.TotalEndpoints		 = 0,
						.Class						 = AUDIO_CSCP_AudioClass,
						.SubClass					 = AUDIO_CSCP_ControlSubclass,
						.Protocol					 = AUDIO_CSCP_ControlProtocol,
						.InterfaceStrIndex = NO_DESCRIPTOR,
				},
		.Audio_ControlInterface_SPC =
				{
						.Header =
								{
										.Size = sizeof(USB_Audio_Descriptor_Interface_AC_t),
										.Type = AUDIO_DTYPE_CSInterface,
								},
						.Subtype				 = AUDIO_DSUBTYPE_CSInterface_Header,
						.ACSpecification = VERSION_BCD(1, 0, 0),
						.TotalLength		 = sizeof(USB_Audio_Descriptor_Interface_AC_t),
						.InCollection		 = 1,
						.InterfaceNumber = MIDI_AS_INTERFACE,
				},
		.Audio_StreamInterface =
				{
						.Header =
								{
										.Size = sizeof(USB_Descriptor_Interface_t),
										.Type = DTYPE_Interface,
								},
						.InterfaceNumber	 = MIDI_AS_INTERFACE,
						.AlternateSetting	 = 0,
						.TotalEndpoints		 = 2,
						.Class						 = AUDIO_CSCP_AudioClass,
						.SubClass					 = AUDIO_CSCP_MIDIStreamingSubclass,
						.Protocol					 = AUDIO_CSCP_StreamingProtocol,
						.InterfaceStrIndex = NO_DESCRIPTOR,
				},
		.Audio_StreamInterface_SPC =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_AudioInterface_AS_t),
										.Type = AUDIO_DTYPE_CSInterface,
								},
						.Subtype						= AUDIO_DSUBTYPE_CSInterface_General,
						.AudioSpecification = VERSION_BCD(1, 0, 0),
						.TotalLength =
								offsetof(struct usb_descriptor, MIDI_Out_Jack_Endpoint_SPC) +
								sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t) -
								offsetof(struct usb_descriptor, Audio_StreamInterface_SPC),
				},
		.MIDI_In_Jack_Emb =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_InputJack_t),
										.Type = AUDIO_DTYPE_CSInterface,
								},
						.Subtype			= AUDIO_DSUBTYPE_CSInterface_InputTerminal,
						.JackType			= MIDI_JACKTYPE_Embedded,
						.JackID				= 0x01,
						.JackStrIndex = NO_DESCRIPTOR,
				},
		.MIDI_In_Jack_Ext =
				{
						.Header				= {.Size = sizeof(USB_MIDI_Descriptor_InputJack_t),
														 .Type = AUDIO_DTYPE_CSInterface},
						.Subtype			= AUDIO_DSUBTYPE_CSInterface_InputTerminal,
						.JackType			= MIDI_JACKTYPE_External,
						.JackID				= 0x02,
						.JackStrIndex = NO_DESCRIPTOR,
				},
		.MIDI_Out_Jack_Emb =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_OutputJack_t),
										.Type = AUDIO_DTYPE_CSInterface,
								},
						.Subtype			= AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
						.JackType			= MIDI_JACKTYPE_Embedded,
						.JackID				= 0x03,
						.NumberOfPins = 1,
						.SourceJackID = {0x02},
						.SourcePinID	= {0x01},
						.JackStrIndex = NO_DESCRIPTOR,
				},
		.MIDI_Out_Jack_Ext =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_OutputJack_t),
										.Type = AUDIO_DTYPE_CSInterface,
								},
						.Subtype			= AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
						.JackType			= MIDI_JACKTYPE_External,
						.JackID				= 0x04,
						.NumberOfPins = 1,
						.SourceJackID = {0x01},
						.SourcePinID	= {0x01},
						.JackStrIndex = NO_DESCRIPTOR,
				},
		.MIDI_In_Jack_Endpoint =
				{
						.Endpoint =
								{
										.Header =
												{
														.Size = sizeof(
																USB_Audio_Descriptor_StreamEndpoint_Std_t),
														.Type = DTYPE_Endpoint,
												},
										.EndpointAddress =
												(ENDPOINT_DIR_OUT | USB_EP_MIDI_STREAM_OUT),
										.Attributes				 = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
																		ENDPOINT_USAGE_DATA),
										.EndpointSize			 = USB_MIDI_STREAM_EPSIZE,
										.PollingIntervalMS = USB_MIDI_POLLING_INTERVAL,
								},
						.Refresh						= 0,
						.SyncEndpointNumber = 0,
				},
		.MIDI_In_Jack_Endpoint_SPC =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
										.Type = AUDIO_DTYPE_CSEndpoint,
								},
						.Subtype						= AUDIO_DSUBTYPE_CSEndpoint_General,
						.TotalEmbeddedJacks = 0x01,
						.AssociatedJackID		= {0x01},
				},
		.MIDI_Out_Jack_Endpoint =
				{
						.Endpoint =
								{
										.Header =
												{
														.Size = sizeof(
																USB_Audio_Descriptor_StreamEndpoint_Std_t),
														.Type = DTYPE_Endpoint,
												},
										.EndpointAddress =
												(ENDPOINT_DIR_IN | USB_EP_MIDI_STREAM_IN),
										.Attributes				 = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
																		ENDPOINT_USAGE_DATA),
										.EndpointSize			 = USB_MIDI_STREAM_EPSIZE,
										.PollingIntervalMS = USB_MIDI_POLLING_INTERVAL,
								},
						.Refresh						= 0,
						.SyncEndpointNumber = 0,
				},
		.MIDI_Out_Jack_Endpoint_SPC =
				{
						.Header =
								{
										.Size = sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
										.Type = AUDIO_DTYPE_CSEndpoint,
								},
						.Subtype						= AUDIO_DSUBTYPE_CSEndpoint_General,
						.TotalEmbeddedJacks = 0x01,
						.AssociatedJackID		= {0x03},
				},

#ifdef HID_ENABLE
#error "No descriptor configuration yet - woops!"
#endif

#ifdef ENABLE_CONSOLE
		.CDC_Interface_Association =
				{
						.Header = {.Size = sizeof(USB_Descriptor_Interface_Association_t),
											 .Type = DTYPE_InterfaceAssociation},
						.FirstInterfaceIndex = CCI_INTERFACE,
						.TotalInterfaces		 = 2,
						.Class							 = CDC_CSCP_CDCClass,
						.SubClass						 = CDC_CSCP_ACMSubclass,
						.Protocol						 = CDC_CSCP_ATCommandProtocol,
						.IADStrIndex				 = NO_DESCRIPTOR,
				},
		.CDC_CCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
																		 .Type = DTYPE_Interface},
													.InterfaceNumber	 = CCI_INTERFACE,
													.AlternateSetting	 = 0,
													.TotalEndpoints		 = 1,
													.Class						 = CDC_CSCP_CDCClass,
													.SubClass					 = CDC_CSCP_ACMSubclass,
													.Protocol					 = CDC_CSCP_ATCommandProtocol,
													.InterfaceStrIndex = NO_DESCRIPTOR},
		.CDC_Functional_Header =
				{
						.Header	 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
												.Type = CDC_DTYPE_CSInterface},
						.Subtype = 0x00,
						.CDCSpecification = VERSION_BCD(1, 1, 0),
				},
		.CDC_Functional_ACM =
				{
						.Header				= {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
														 .Type = CDC_DTYPE_CSInterface},
						// .Subtype                = 0x02,
						// .Capabilities           = 0x02,
						.Capabilities = 0x06,
				},
		.CDC_Functional_Union =
				{
						.Header	 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
												.Type = CDC_DTYPE_CSInterface},
						.Subtype = 0x06,
						.MasterInterfaceNumber = CCI_INTERFACE,
						.SlaveInterfaceNumber	 = CDI_INTERFACE,
				},
		.CDC_NotificationEndpoint =
				{.Header					= {.Size = sizeof(USB_Descriptor_Endpoint_t),
														 .Type = DTYPE_Endpoint},
				 .EndpointAddress = (ENDPOINT_DIR_IN | CDC_NOTIFICATION_EPNUM),
				 .Attributes =
						 (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
				 .EndpointSize			= USB_CDC_NOTIFICATION_EPSIZE,
				 .PollingIntervalMS = 0xFF},
		.CDC_DCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
																		 .Type = DTYPE_Interface},
													.InterfaceNumber	 = CDI_INTERFACE,
													.AlternateSetting	 = 0,
													.TotalEndpoints		 = 2,
													.Class						 = CDC_CSCP_CDCDataClass,
													.SubClass					 = CDC_CSCP_NoDataSubclass,
													.Protocol					 = CDC_CSCP_NoDataProtocol,
													.InterfaceStrIndex = NO_DESCRIPTOR},
		.CDC_DataOutEndpoint =
				{.Header					= {.Size = sizeof(USB_Descriptor_Endpoint_t),
														 .Type = DTYPE_Endpoint},
				 .EndpointAddress = (ENDPOINT_DIR_OUT | CDC_OUT_EPNUM),
				 .Attributes =
						 (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
				 .EndpointSize			= USB_CDC_EPSIZE,
				 .PollingIntervalMS = 0x05},
		.CDC_DataInEndpoint = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
																			.Type = DTYPE_Endpoint},
													 .EndpointAddress = (ENDPOINT_DIR_IN | CDC_IN_EPNUM),
													 .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
																					ENDPOINT_USAGE_DATA),
													 .EndpointSize			= USB_CDC_EPSIZE,
													 .PollingIntervalMS = 0x05},
#endif
};

#pragma GCC diagnostic ignored "-Wunused-variable"
static bool vser_active = false;
#pragma GCC diagnostic pop

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int usb_init(void) {
	USB_Init(); // Init LUFA usb stack
	return 0;
}

int usb_update(void) {
	// The USB packets are periodically transmitted by LUFA, calling the
	// MIDI_Device_USBTask function flushes the packets immediately to the
	// host.)
	MIDI_Device_USBTask(&lufa_usb_midi_device);

#ifdef ENABLE_CONSOLE
	// Throw away unused received bytes from host
	if (vser_active) {
		// while (CDC_Device_ReceiveByte(&lufa_usb_cdc_device) == true) {}
		CDC_Device_USBTask(&lufa_usb_cdc_device);
	}

#endif

	USB_USBTask(); // LUFA usb stack update
	return 0;
}

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t		 wValue,
																		const uint16_t		 wIndex,
																		const void** const DescriptorAddress) {
	const uint8_t DescriptorType	= (u8)(wValue >> 8);
	const uint8_t DescriptorIndex = (wValue & 0xFF);
	const void*		Address					= NULL;
	uint16_t			Size						= NO_DESCRIPTOR;

	switch (DescriptorType) {
		case DTYPE_Device:
			Address = &desc_device;
			Size		= sizeof(USB_Descriptor_Device_t);
			break;

		case DTYPE_Configuration:
			Address = &desc_cfg;
			Size		= sizeof(struct usb_descriptor);
			break;

		case DTYPE_String:
			switch (DescriptorIndex) {
				case DESC_STR_LANG:
					Address = &desc_str_lang;
					Size		= pgm_read_byte(&desc_str_lang.Header.Size);
					break;

				case DESC_STR_MF:
					Address = &desc_str_mf;
					Size		= pgm_read_byte(&desc_str_mf.Header.Size);

					break;

				case DESC_STR_PROD:
					Address = &desc_str_prod;
					Size		= pgm_read_byte(&desc_str_prod.Header.Size);
					break;

				case DESC_STR_SER:
					Address = &desc_str_ser;
					Size		= pgm_read_byte(&desc_str_ser.Header.Size);
					break;
			}

			break;
		case HID_DTYPE_HID:
			switch (wIndex) {

#ifdef HID_ENABLE
#error "This is not completed"
				case HID_INTERFACE:
					Address = &ConfigurationDescriptor.Shared_HID;
					Size		= sizeof(USB_HID_Descriptor_HID_t);

					break;
#endif
			}

			break;
		case HID_DTYPE_Report:
			switch (wIndex) {
#ifdef HID_ENABLE
#error "This is not completed"
				case HID_INTERFACE:
					Address = &SharedReport;
					Size		= sizeof(SharedReport);

					break;
#endif
			}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

// Callback for USB device connection
void EVENT_USB_Device_Connect(void) {
}

// Callback for USB device disconnection
void EVENT_USB_Device_Disconnect(void) {
}

// Callback for USB device configuration changed
void EVENT_USB_Device_ConfigurationChanged(void) {
	MIDI_Device_ConfigureEndpoints(&lufa_usb_midi_device);

#ifdef HID_ENABLE
	ConfigSuccess &= Endpoint_ConfigureEndpoint(
			(SHARED_IN_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_INTERRUPT, SHARED_EPSIZE, 1);
#endif

#ifdef ENABLE_CONSOLE
	CDC_Device_ConfigureEndpoints(&lufa_usb_cdc_device);
#endif
}

void EVENT_USB_Device_ControlRequest(void) {
	MIDI_Device_ProcessControlRequest(&lufa_usb_midi_device);

#ifdef HID_ENABLE
#error "Need to handle HID class requests here"
#endif

#ifdef ENABLE_CONSOLE
	CDC_Device_ProcessControlRequest(&lufa_usb_cdc_device);
#endif
}

#ifdef ENABLE_CONSOLE
void EVENT_CDC_Device_ControLineStateChanged(
		USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) {
	/* You can get changes to the virtual CDC lines in this callback; a common
		 use-case is to use the Data Terminal Ready (DTR) flag to enable and
		 disable CDC communications in your application when set to avoid the
		 application blocking while waiting for a host to become ready and read
		 in the pending data from the USB endpoints.
	*/
	vser_active = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
								 CDC_CONTROL_LINE_OUT_DTR) != 0;
}

// Function to check if the virtual serial port is active (DTR asserted)
bool usb_cdc_is_active(void) {
	return vser_active;
}

void println_progmem(const char* const str) {
	assert(str);

	if (vser_active) {
		CDC_Device_SendString_P(&lufa_usb_cdc_device, str);
		CDC_Device_SendString_P(&lufa_usb_cdc_device, PSTR("\r\n"));
		CDC_Device_Flush(&lufa_usb_cdc_device);
	}
}

void println(const char* const str) {
	assert(str);

	if (vser_active) {
		CDC_Device_SendString(&lufa_usb_cdc_device, str);
		CDC_Device_SendString_P(&lufa_usb_cdc_device, PSTR("\r\n"));
		CDC_Device_Flush(&lufa_usb_cdc_device);
	}
}

void printbuf(u8* buf, uint len) {
	assert(buf);

	if (vser_active) {
		CDC_Device_SendData(&lufa_usb_cdc_device, buf, len);
		CDC_Device_SendString_P(&lufa_usb_cdc_device, PSTR("\r\n"));
		CDC_Device_Flush(&lufa_usb_cdc_device);
	}
}

#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
