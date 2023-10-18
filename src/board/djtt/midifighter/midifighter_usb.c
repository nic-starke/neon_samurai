/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"

#include "board/djtt/midifighter.h"
#include "board/djtt/midifighter_usb.h"

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const USB_Descriptor_String_t PROGMEM LanguageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
static const USB_Descriptor_String_t PROGMEM ManufacturerString =
    USB_STRING_DESCRIPTOR(L"DJTT");
static const USB_Descriptor_String_t PROGMEM ProductString =
    USB_STRING_DESCRIPTOR(L"Muffin Twister");
static const USB_Descriptor_String_t PROGMEM SerialString =
    USB_STRING_DESCRIPTOR(L"666");

/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */
static const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
    .USBSpecification = VERSION_BCD(1, 1, 0),

#ifdef VSER_ENABLE
    .Class    = USB_CSCP_IADDeviceClass,
    .SubClass = USB_CSCP_IADDeviceSubclass,
    .Protocol = USB_CSCP_IADDeviceProtocol,
#else
    .Class    = USB_CSCP_NoDeviceClass,
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
#endif

    .Endpoint0Size        = FIXED_CONTROL_ENDPOINT_SIZE,
    // Specified in config.h
    .VendorID             = DEFAULT_USB_VENDOR_ID,
    .ProductID            = DEFAULT_USB_PRODUCT_ID,
    .ReleaseNumber        = VERSION_BCD(0, 0, 1),
    .ManufacturerStrIndex = STRING_ID_Manufacturer,
    .ProductStrIndex      = STRING_ID_Product,
    .SerialNumStrIndex    = STRING_ID_Serial,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS};

static const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},
               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces        = NUM_USB_INTERFACES,
               .ConfigurationNumber    = 1,
               .ConfigurationStrIndex  = NO_DESCRIPTOR,
               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),
               .MaxPowerConsumption = USB_CONFIG_POWER_MA(480)},

#ifdef MIDI_ENABLE
    .Audio_Interface_Association =
        {
            .Header = {.Size = sizeof(USB_Descriptor_Interface_Association_t),
                       .Type = DTYPE_InterfaceAssociation},
            .FirstInterfaceIndex = MIDI_AC_INTERFACE,
            .TotalInterfaces     = 2,
            .Class               = AUDIO_CSCP_AudioClass,
            .SubClass            = AUDIO_CSCP_ControlSubclass,
            .Protocol            = AUDIO_CSCP_ControlProtocol,
            .IADStrIndex         = NO_DESCRIPTOR,
        },
    .Audio_ControlInterface = {.Header            = {.Size = sizeof(
                                              USB_Descriptor_Interface_t),
                                                     .Type = DTYPE_Interface},
                               .InterfaceNumber   = MIDI_AC_INTERFACE,
                               .AlternateSetting  = 0,
                               .TotalEndpoints    = 0,
                               .Class             = AUDIO_CSCP_AudioClass,
                               .SubClass          = AUDIO_CSCP_ControlSubclass,
                               .Protocol          = AUDIO_CSCP_ControlProtocol,
                               .InterfaceStrIndex = NO_DESCRIPTOR},
    .Audio_ControlInterface_SPC =
        {
            .Header  = {.Size = sizeof(USB_Audio_Descriptor_Interface_AC_t),
                        .Type = AUDIO_DTYPE_CSInterface},
            .Subtype = AUDIO_DSUBTYPE_CSInterface_Header,
            .ACSpecification = VERSION_BCD(1, 0, 0),
            .TotalLength     = sizeof(USB_Audio_Descriptor_Interface_AC_t),
            .InCollection    = 1,
            .InterfaceNumber = MIDI_AS_INTERFACE,
        },
    .Audio_StreamInterface = {.Header           = {.Size =
                                                       sizeof(USB_Descriptor_Interface_t),
                                                   .Type = DTYPE_Interface},
                              .InterfaceNumber  = MIDI_AS_INTERFACE,
                              .AlternateSetting = 0,
                              .TotalEndpoints   = 2,
                              .Class            = AUDIO_CSCP_AudioClass,
                              .SubClass = AUDIO_CSCP_MIDIStreamingSubclass,
                              .Protocol = AUDIO_CSCP_StreamingProtocol,
                              .InterfaceStrIndex = NO_DESCRIPTOR},
    .Audio_StreamInterface_SPC =
        {.Header  = {.Size = sizeof(USB_MIDI_Descriptor_AudioInterface_AS_t),
                     .Type = AUDIO_DTYPE_CSInterface},
         .Subtype = AUDIO_DSUBTYPE_CSInterface_General,
         .AudioSpecification = VERSION_BCD(1, 0, 0),
         .TotalLength        = offsetof(USB_Descriptor_Configuration_t,
                                        MIDI_Out_Jack_Endpoint_SPC) +
                        sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t) -
                        offsetof(USB_Descriptor_Configuration_t,
                                 Audio_StreamInterface_SPC)},
    .MIDI_In_Jack_Emb  = {.Header   = {.Size =
                                           sizeof(USB_MIDI_Descriptor_InputJack_t),
                                       .Type = AUDIO_DTYPE_CSInterface},
                          .Subtype  = AUDIO_DSUBTYPE_CSInterface_InputTerminal,
                          .JackType = MIDI_JACKTYPE_Embedded,
                          .JackID   = 0x01,
                          .JackStrIndex = NO_DESCRIPTOR},
    .MIDI_In_Jack_Ext  = {.Header   = {.Size =
                                           sizeof(USB_MIDI_Descriptor_InputJack_t),
                                       .Type = AUDIO_DTYPE_CSInterface},
                          .Subtype  = AUDIO_DSUBTYPE_CSInterface_InputTerminal,
                          .JackType = MIDI_JACKTYPE_External,
                          .JackID   = 0x02,
                          .JackStrIndex = NO_DESCRIPTOR},
    .MIDI_Out_Jack_Emb = {.Header   = {.Size = sizeof(
                                         USB_MIDI_Descriptor_OutputJack_t),
                                       .Type = AUDIO_DTYPE_CSInterface},
                          .Subtype  = AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
                          .JackType = MIDI_JACKTYPE_Embedded,
                          .JackID   = 0x03,
                          .NumberOfPins = 1,
                          .SourceJackID = {0x02},
                          .SourcePinID  = {0x01},
                          .JackStrIndex = NO_DESCRIPTOR},
    .MIDI_Out_Jack_Ext = {.Header   = {.Size = sizeof(
                                         USB_MIDI_Descriptor_OutputJack_t),
                                       .Type = AUDIO_DTYPE_CSInterface},
                          .Subtype  = AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
                          .JackType = MIDI_JACKTYPE_External,
                          .JackID   = 0x04,
                          .NumberOfPins = 1,
                          .SourceJackID = {0x01},
                          .SourcePinID  = {0x01},
                          .JackStrIndex = NO_DESCRIPTOR},
    .MIDI_In_Jack_Endpoint =
        {.Endpoint           = {.Header = {.Size = sizeof(
                                     USB_Audio_Descriptor_StreamEndpoint_Std_t),
                                           .Type = DTYPE_Endpoint},
                                .EndpointAddress =
                                    (ENDPOINT_DIR_OUT | MIDI_STREAM_OUT_EPNUM),
                                .Attributes   = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                     ENDPOINT_USAGE_DATA),
                                .EndpointSize = MIDI_STREAM_EPSIZE,
                                .PollingIntervalMS = MIDI_POLLING_INTERVAL},
         .Refresh            = 0,
         .SyncEndpointNumber = 0},
    .MIDI_In_Jack_Endpoint_SPC =
        {.Header  = {.Size = sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
                     .Type = AUDIO_DTYPE_CSEndpoint},
         .Subtype = AUDIO_DSUBTYPE_CSEndpoint_General,
         .TotalEmbeddedJacks = 0x01,
         .AssociatedJackID   = {0x01}},
    .MIDI_Out_Jack_Endpoint =
        {.Endpoint           = {.Header = {.Size = sizeof(
                                     USB_Audio_Descriptor_StreamEndpoint_Std_t),
                                           .Type = DTYPE_Endpoint},
                                .EndpointAddress =
                                    (ENDPOINT_DIR_IN | MIDI_STREAM_IN_EPNUM),
                                .Attributes   = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                     ENDPOINT_USAGE_DATA),
                                .EndpointSize = MIDI_STREAM_EPSIZE,
                                .PollingIntervalMS = MIDI_POLLING_INTERVAL},
         .Refresh            = 0,
         .SyncEndpointNumber = 0},
    .MIDI_Out_Jack_Endpoint_SPC =
        {.Header  = {.Size = sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
                     .Type = AUDIO_DTYPE_CSEndpoint},
         .Subtype = AUDIO_DSUBTYPE_CSEndpoint_General,
         .TotalEmbeddedJacks = 0x01,
         .AssociatedJackID   = {0x03}},
#endif

#ifdef HID_ENABLE
#error "No descriptor configuration yet - woops!"
#endif

#ifdef VSER_ENABLE
    .CDC_Interface_Association =
        {
            .Header = {.Size = sizeof(USB_Descriptor_Interface_Association_t),
                       .Type = DTYPE_InterfaceAssociation},
            .FirstInterfaceIndex = CCI_INTERFACE,
            .TotalInterfaces     = 2,
            .Class               = CDC_CSCP_CDCClass,
            .SubClass            = CDC_CSCP_ACMSubclass,
            .Protocol            = CDC_CSCP_ATCommandProtocol,
            .IADStrIndex         = NO_DESCRIPTOR,
        },
    .CDC_CCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                     .Type = DTYPE_Interface},
                          .InterfaceNumber   = CCI_INTERFACE,
                          .AlternateSetting  = 0,
                          .TotalEndpoints    = 1,
                          .Class             = CDC_CSCP_CDCClass,
                          .SubClass          = CDC_CSCP_ACMSubclass,
                          .Protocol          = CDC_CSCP_ATCommandProtocol,
                          .InterfaceStrIndex = NO_DESCRIPTOR},
    .CDC_Functional_Header =
        {
            .Header  = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
                        .Type = CDC_DTYPE_CSInterface},
            .Subtype = 0x00,
            .CDCSpecification = VERSION_BCD(1, 1, 0),
        },
    .CDC_Functional_ACM =
        {
            .Header       = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
                             .Type = CDC_DTYPE_CSInterface},
            // .Subtype                = 0x02,
            // .Capabilities           = 0x02,
            .Capabilities = 0x06,
        },
    .CDC_Functional_Union =
        {
            .Header  = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
                        .Type = CDC_DTYPE_CSInterface},
            .Subtype = 0x06,
            .MasterInterfaceNumber = CCI_INTERFACE,
            .SlaveInterfaceNumber  = CDI_INTERFACE,
        },
    .CDC_NotificationEndpoint =
        {.Header          = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                             .Type = DTYPE_Endpoint},
         .EndpointAddress = (ENDPOINT_DIR_IN | CDC_NOTIFICATION_EPNUM),
         .Attributes =
             (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
         .EndpointSize      = CDC_NOTIFICATION_EPSIZE,
         .PollingIntervalMS = 0xFF},
    .CDC_DCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                     .Type = DTYPE_Interface},
                          .InterfaceNumber   = CDI_INTERFACE,
                          .AlternateSetting  = 0,
                          .TotalEndpoints    = 2,
                          .Class             = CDC_CSCP_CDCDataClass,
                          .SubClass          = CDC_CSCP_NoDataSubclass,
                          .Protocol          = CDC_CSCP_NoDataProtocol,
                          .InterfaceStrIndex = NO_DESCRIPTOR},
    .CDC_DataOutEndpoint =
        {.Header          = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                             .Type = DTYPE_Endpoint},
         .EndpointAddress = (ENDPOINT_DIR_OUT | CDC_OUT_EPNUM),
         .Attributes =
             (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
         .EndpointSize      = CDC_EPSIZE,
         .PollingIntervalMS = 0x05},
    .CDC_DataInEndpoint = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                      .Type = DTYPE_Endpoint},
                           .EndpointAddress = (ENDPOINT_DIR_IN | CDC_IN_EPNUM),
                           .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                          ENDPOINT_USAGE_DATA),
                           .EndpointSize      = CDC_EPSIZE,
                           .PollingIntervalMS = 0x05},
#endif
};

#ifdef MIDI_ENABLE
USB_ClassInfo_MIDI_Device_t gMIDI_Interface = {
    .Config = {.DataINEndpoint =
                   {
                       .Address = (MIDI_STREAM_IN_EPNUM | ENDPOINT_DIR_IN),
                       .Size    = MIDI_STREAM_EPSIZE,
                       .Banks   = 1,
                   },
               .DataOUTEndpoint = {
                   .Address = (MIDI_STREAM_OUT_EPNUM | ENDPOINT_DIR_OUT),
                   .Size    = MIDI_STREAM_EPSIZE,
                   .Banks   = 1,
               }}};
#endif

#ifdef VSER_ENABLE
USB_ClassInfo_CDC_Device_t gCDC_Interface = {
    .Config = {
        .ControlInterfaceNumber = CCI_INTERFACE,
        .DataINEndpoint         = {.Address = (CDC_IN_EPNUM | ENDPOINT_DIR_IN),
                                   .Size    = CDC_EPSIZE,
                                   .Banks   = 1},
        .DataOUTEndpoint      = {.Address = (CDC_OUT_EPNUM | ENDPOINT_DIR_OUT),
                                 .Size    = CDC_EPSIZE,
                                 .Banks   = 1},
        .NotificationEndpoint = {.Address =
                                     (CDC_NOTIFICATION_EPNUM | ENDPOINT_DIR_IN),
                                 .Size  = CDC_NOTIFICATION_EPSIZE,
                                 .Banks = 1}}};
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

inline void mf_usb_init(void) {
  USB_Init(); // Init LUFA usb stack
}

inline void mf_usb_update(void) {
  USB_USBTask(); // LUFA usb stack update
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t     wValue,
                                    const uint16_t     wIndex,
                                    const void** const DescriptorAddress) {
  const uint8_t DescriptorType  = (wValue >> 8);
  const uint8_t DescriptorIndex = (wValue & 0xFF);
  const void*   Address         = NULL;
  uint16_t      Size            = NO_DESCRIPTOR;

  switch (DescriptorType) {
    case DTYPE_Device:
      Address = &DeviceDescriptor;
      Size    = sizeof(USB_Descriptor_Device_t);
      break;

    case DTYPE_Configuration:
      Address = &ConfigurationDescriptor;
      Size    = sizeof(USB_Descriptor_Configuration_t);
      break;

    case DTYPE_String:
      switch (DescriptorIndex) {
        case STRING_ID_Language:
          Address = &LanguageString;
          Size    = pgm_read_byte(&LanguageString.Header.Size);
          break;

        case STRING_ID_Manufacturer:
          Address = &ManufacturerString;
          Size    = pgm_read_byte(&ManufacturerString.Header.Size);
          break;

        case STRING_ID_Product:
          Address = &ProductString;
          Size    = pgm_read_byte(&ProductString.Header.Size);
          break;

        case STRING_ID_Serial:
          Address = &SerialString;
          Size    = pgm_read_byte(&SerialString.Header.Size);
          break;
      }

      break;
    case HID_DTYPE_HID:
      switch (wIndex) {

#ifdef HID_ENABLE
#error "This is not completed"
        case HID_INTERFACE:
          Address = &ConfigurationDescriptor.Shared_HID;
          Size    = sizeof(USB_HID_Descriptor_HID_t);

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
          Size    = sizeof(SharedReport);

          break;
#endif
      }

      break;
  }

  *DescriptorAddress = Address;
  return Size;
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;
#ifdef MIDI_ENABLE
  ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&gMIDI_Interface);
  // ConfigSuccess &= Endpoint_ConfigureEndpoint((MIDI_STREAM_OUT_EPNUM |
  // ENDPOINT_DIR_IN), EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1); ConfigSuccess &=
  // Endpoint_ConfigureEndpoint((MIDI_STREAM_IN_EPNUM | ENDPOINT_DIR_OUT),
  // EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1);
#endif

#ifdef HID_ENABLE
  ConfigSuccess &= Endpoint_ConfigureEndpoint(
      (SHARED_IN_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_INTERRUPT, SHARED_EPSIZE, 1);
#endif

#ifdef VSER_ENABLE
  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&gCDC_Interface);
  // ConfigSuccess &= Endpoint_ConfigureEndpoint((CDC_NOTIFICATION_EPNUM |
  // ENDPOINT_DIR_IN), EP_TYPE_INTERRUPT, CDC_NOTIFICATION_EPSIZE, 1);
  // ConfigSuccess &= Endpoint_ConfigureEndpoint((CDC_OUT_EPNUM |
  // ENDPOINT_DIR_OUT), EP_TYPE_BULK, CDC_EPSIZE, 1); ConfigSuccess &=
  // Endpoint_ConfigureEndpoint((CDC_IN_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_BULK,
  // CDC_EPSIZE, 1);
#endif
  //   Serial_Print(ConfigSuccess ? "USB configured successfully\r\n"
  //                              : "USB failed to configure\r\n");
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
#ifdef MIDI_ENABLE
  MIDI_Device_ProcessControlRequest(&gMIDI_Interface);
#endif

#ifdef HID_ENABLE
#error "Need to handle HID class requests here"
#endif

#ifdef VSER_ENABLE
  CDC_Device_ProcessControlRequest(&gCDC_Interface);
#endif
}
