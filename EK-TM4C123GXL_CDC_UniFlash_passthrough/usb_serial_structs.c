/*
 * @file usb_serial_structs.c
 * @data 4 Jan 2015
 * @author Chester Gillon
 * @brief Data structures defining this CDC USB device
 */

#include <stdint.h>
#include <stdbool.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/usb-ids.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

#include "usb_serial_structs.h"

/** The languages supported by this device. */
static const uint8_t language_descriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_UK)
};

/** The product string. */
static const uint8_t manufacturer_string[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

/** The product string */
static const uint8_t product_string[] =
{
    2 + (28 * 2),
    USB_DTYPE_STRING,
    'C', 0, 'C', 0, '3', 0, '1', 0, '0', 0, '0', 0, 'B', 0, 'O', 0,
    'O', 0, 'S', 0, 'T', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0,
    'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0, ' ', 0,
    'P', 0, 'o', 0, 'r', 0, 't', 0
};

/** The serial number string */
static const uint8_t serial_number_string[] =
{
    2 + (32 * 2),
    USB_DTYPE_STRING,
    '2', 0, 'b', 0, '0', 0, 'c', 0, '3', 0, '2', 0, '3', 0, '8', 0,
    '0', 0, 'd', 0, 'd', 0, '6', 0, '4', 0, '6', 0, 'c', 0, 'd', 0,
    '2', 0, '2', 0, 'c', 0, '0', 0, '8', 0, '7', 0, '6', 0, '5', 0,
    '6', 0, '7', 0, '2', 0, '1', 0, '1', 0, '1', 0, 'a', 0, 'f', 0
};

/** The control interface description string */
static const uint8_t control_interface_string[] =
{
    2 + (21 * 2),
    USB_DTYPE_STRING,
    'A', 0, 'C', 0, 'M', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
    'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

/** The configuration description string */
static const uint8_t config_string[] =
{
    2 + (26 * 2),
    USB_DTYPE_STRING,
    'S', 0, 'e', 0, 'l', 0, 'f', 0, ' ', 0, 'P', 0, 'o', 0, 'w', 0,
    'e', 0, 'r', 0, 'e', 0, 'd', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0,
    'o', 0, 'n', 0
};

/** The descriptor string table */
static const uint8_t * const string_descriptors[] =
{
    language_descriptor,
    manufacturer_string,
    product_string,
    serial_number_string,
    control_interface_string,
    config_string
};

#define NUM_STRING_DESCRIPTORS (sizeof(string_descriptors) / sizeof(uint8_t *))


/**
  The CDC device initialization and customization structures. In this case,
  we are using USBBuffers between the CDC device class driver and the
  application code. The function pointers and callback data values are set
  to insert a buffer in each of the data channels, transmit and receive.

   With the buffer in place, the CDC channel callback is set to the relevant
   channel function and the callback data is set to point to the channel
   instance data. The buffer, in turn, has its callback set to the application
   function and the callback data set to our CDC instance structure.
*/
tUSBDCDCDevice CDC_device =
{
    USB_VID_TI_1CBE,
    USB_PID_SERIAL,
    0,
    USB_CONF_ATTR_SELF_PWR,
    cdc_control_handler,
    (void *)&CDC_device,
    USBBufferEventCallback,
    (void *)&cdc_rx_buffer,
    USBBufferEventCallback,
    (void *)&cdc_tx_buffer,
    string_descriptors,
    NUM_STRING_DESCRIPTORS
};

/** Receive buffer (from the USB perspective). */
static uint8_t usb_rx_buffer[UART_BUFFER_SIZE];
static uint8_t rx_buffer_workspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer cdc_rx_buffer =
{
    false,                          /* This is a receive buffer. */
    cdc_rx_handler,                 /* pfnCallback */
    (void *)&CDC_device,            /* Callback data is our device pointer. */
    USBDCDCPacketRead,              /* pfnTransfer */
    USBDCDCRxPacketAvailable,       /* pfnAvailable */
    (void *)&CDC_device,            /* pvHandle */
    usb_rx_buffer,                  /* pui8Buffer */
    UART_BUFFER_SIZE,               /* ui32BufferSize */
    rx_buffer_workspace             /* pvWorkspace */
};

/* Transmit buffer (from the USB perspective). */
static uint8_t usb_tx_buffer[UART_BUFFER_SIZE];
static uint8_t tx_buffer_workspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer cdc_tx_buffer =
{
    true,                           // This is a transmit buffer.
    cdc_tx_handler,                 // pfnCallback
    (void *)&CDC_device,            // Callback data is our device pointer.
    USBDCDCPacketWrite,             // pfnTransfer
    USBDCDCTxPacketAvailable,       // pfnAvailable
    (void *)&CDC_device,            // pvHandle
    usb_tx_buffer,                  // pui8Buffer
    UART_BUFFER_SIZE,               // ui32BufferSize
    tx_buffer_workspace             // pvWorkspace
};
