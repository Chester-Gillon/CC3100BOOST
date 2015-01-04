/*
 * @file usb_serial_structs.h
 * @date 4 Jan 2015
 * @author Chester Gillon
 * @brief Data structures declaring this CDC USB device
 */

#ifndef USB_SERIAL_STRUCTS_H_
#define USB_SERIAL_STRUCTS_H_

/** CDC device callback function prototypes */
uint32_t cdc_rx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData);
uint32_t cdc_tx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData);
uint32_t cdc_control_handler(void *pvCBData, uint32_t ui32Event,
                             uint32_t ui32MsgValue, void *pvMsgData);

/* The size of the transmit and receive buffers used for the redirected UART.
   This number should be a power of 2 for best performance.  256 is chosen
   pretty much at random though the buffer should be at least twice the size of
   a maxmum-sized USB packet.
*/
#define UART_BUFFER_SIZE 256

extern const tUSBBuffer cdc_tx_buffer;
extern const tUSBBuffer cdc_rx_buffer;
extern tUSBDCDCDevice CDC_device;

#endif /* USB_SERIAL_STRUCTS_H_ */
