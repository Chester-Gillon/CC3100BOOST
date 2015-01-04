/*
 * @file main.c
 * @date 2 Jan 2015
 * @author Chester Gillon
 * @brief Allow a CC3100BOOST fitted to a EK-TM4C123GXL to be accessed by CC31xx & CC32xx UniFlash
 * @details
 *
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/fpu.h>
#include <driverlib/gpio.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

#include "usb_serial_structs.h"

/**
 * @brief If a program assertion fails, light the red LED and halt
 * @param[in] assertion Value which must be true to allow program execution to continue
 */
static void check_assert (const bool assertion)
{
    if (!assertion)
    {
        SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
        GPIOPinTypeGPIOOutput (GPIO_PORTF_BASE, GPIO_PIN_1);
        GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);

        for (;;)
        {

        }
    }
}

/**
 * @brief Deassert the nHIB signal to the CC3100BOOST
 */
static void deassert_nHIB (void)
{
    GPIOPinWrite (GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
}

uint32_t cdc_rx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData)
{
    check_assert (false); /* @todo stub */
    return 0;
}

uint32_t cdc_tx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData)
{
    check_assert (false); /* @todo stub */
    return 0;
}

uint32_t cdc_control_handler(void *pvCBData, uint32_t ui32Event,
                             uint32_t ui32MsgValue, void *pvMsgData)
{
    switch (ui32Event)
    {
    /* Ignore BUS suppend and resume */
    case USB_EVENT_SUSPEND:
    case USB_EVENT_RESUME:
        break;

    default:
        check_assert (false);
        break;
    }

    return 0;
}

int main (void)
{
    FPULazyStackingEnable();

    /* Set to maximum 80MHz clock */
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                   SYSCTL_OSC_MAIN);

    /* Configure the required pins for USB operation. */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeUSBAnalog (GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    /* Configure the required pins for the UART1 used to communicate with the CC3100BOOST */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_UART1);
    GPIOPinConfigure (GPIO_PB0_U1RX);
    GPIOPinConfigure (GPIO_PB1_U1TX);
    GPIOPinConfigure (GPIO_PC4_U1RTS);
    GPIOPinConfigure (GPIO_PC5_U1CTS);
    GPIOPinTypeUART (GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    GPIOPinTypeUART (GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    /* Configure the GPIO pin for controlling the CC3100BOOST nHIB, initially not asserted */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput (GPIO_PORTE_BASE, GPIO_PIN_4);
    deassert_nHIB ();

    /* Initialize the transmit and receive buffers. */
    check_assert (USBBufferInit (&cdc_tx_buffer) != NULL);
    check_assert (USBBufferInit (&cdc_rx_buffer) != NULL);

    /* Set the USB stack mode to Device mode with VBUS monitoring. */
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    /* Pass our device information to the USB library and place the device on the bus. */
    check_assert (USBDCDCInit(0, &CDC_device) != NULL);

    for (;;)
    {
        /* @todo stub */
    }

    return 0;
}
