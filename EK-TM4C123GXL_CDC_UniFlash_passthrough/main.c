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
#include <driverlib/uart.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/cpu.h>
#include <driverlib/systick.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

#include "usb_serial_structs.h"

/** When true the nHIB has been asserted following the break being asserted.
 *  When the timer expires the nHIB is de-asserted.
 */
static volatile bool nHIB_timer_running;

/** Millisecond count-down for timing now long to assert nHIB */
static volatile uint32_t nHIB_timer_ms;

/**
 * @brief If a program assertion fails, light only the red LED and halt
 * @param[in] assertion Value which must be true to allow program execution to continue
 */
static void check_assert (const bool assertion)
{
    if (!assertion)
    {
        SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
        GPIOPinTypeGPIOOutput (GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
        GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1);

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

/**
 * @brief Assert the nHIB signal to the CC3100BOOST
 */
static void assert_nHIB (void)
{
    GPIOPinWrite (GPIO_PORTE_BASE, GPIO_PIN_4, 0);
}

/**
 * @brief Interrupt handler for Sys Tick which de-asserts nHIB after the timer expires
 */
void sys_tick_handler (void)
{
    if (nHIB_timer_running)
    {
        if (nHIB_timer_ms == 0)
        {
            nHIB_timer_running = false;
            deassert_nHIB ();
        }
        else
        {
            nHIB_timer_ms--;
        }
    }
}

/**
 * @brief Handles CDC driver notifications related to the receive channel (data from the USB host).
 */
uint32_t cdc_rx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData)
{
    uint32_t return_value;

    switch (ui32Event)
    {
    case USB_EVENT_DATA_REMAINING:
        /* We are being asked how much unprocessed data we have still to
           process. We return 0 if the UART is currently idle or 1 if it is
           in the process of transmitting something. The actual number of
           bytes in the UART FIFO is not important here, merely whether or
           not everything previously sent to us has been transmitted. */
        return_value = UARTBusy (UART1_BASE) ? 1 : 0;
        break;

    default:
        check_assert (false);
    }

    return return_value;
}

uint32_t cdc_tx_handler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgValue, void *pvMsgData)
{
    check_assert (false); /* @todo stub */
    return 0;
}

/**
 * @brief Get the current line coding for the UART
 * @param[out] line_coding Current values read from the UART, in the CDC format
 */
static void get_line_coding (tLineCoding *const line_coding)
{
    uint32_t baud;
    uint32_t config;

    UARTConfigGetExpClk (UART1_BASE, MAP_SysCtlClockGet(), &baud, &config);
    line_coding->ui32Rate = baud;

    switch (config & UART_CONFIG_STOP_MASK)
    {
    case UART_CONFIG_STOP_ONE:
        line_coding->ui8Stop = USB_CDC_STOP_BITS_1;
        break;

    case UART_CONFIG_STOP_TWO:
        line_coding->ui8Stop = USB_CDC_STOP_BITS_2;
        break;

    default:
        check_assert (false);
        break;
    }

    switch (config & UART_CONFIG_PAR_MASK)
    {
    case UART_CONFIG_PAR_NONE:
        line_coding->ui8Parity = USB_CDC_PARITY_NONE;
        break;

    case UART_CONFIG_PAR_EVEN:
        line_coding->ui8Parity = USB_CDC_PARITY_EVEN;
        break;

    case UART_CONFIG_PAR_ODD:
        line_coding->ui8Parity = USB_CDC_PARITY_ODD;
        break;

    case UART_CONFIG_PAR_ONE:
        line_coding->ui8Parity = USB_CDC_PARITY_MARK;
        break;

    case UART_CONFIG_PAR_ZERO:
        line_coding->ui8Parity = USB_CDC_PARITY_SPACE;
        break;

    default:
        check_assert (false);
        break;
    }

    switch (config & UART_CONFIG_WLEN_MASK)
    {
    case UART_CONFIG_WLEN_5:
        line_coding->ui8Databits = 5;
        break;

    case UART_CONFIG_WLEN_6:
        line_coding->ui8Databits = 6;
        break;

    case UART_CONFIG_WLEN_7:
        line_coding->ui8Databits = 7;
        break;

    case UART_CONFIG_WLEN_8:
        line_coding->ui8Databits = 8;
        break;

    default:
        check_assert (false);
        break;
    }
}

/**
 * @brief Set the line coding for the UART
 * @todo If the line coding is invalid, the UART configuration is not changed.
 *       Unsure if can report the error to the USB bus.
 *       The Control Callback has a return value, which is always ignored by the USB stack.
 * @param[in] line_coding The values to set, in CDC format
 */
static void set_line_coding (const tLineCoding *const line_coding)
{
    bool config_valid = true;
    uint32_t config = 0;

    switch (line_coding->ui8Databits)
    {
    case 5:
        config |= UART_CONFIG_WLEN_5;
        break;

    case 6:
        config |= UART_CONFIG_WLEN_6;
        break;

    case 7:
        config |= UART_CONFIG_WLEN_7;
        break;

    case 8:
        config |= UART_CONFIG_WLEN_8;
        break;

    default:
        config_valid = false;
        break;
    }

    switch (line_coding->ui8Parity)
    {
    case USB_CDC_PARITY_NONE:
        config |= UART_CONFIG_PAR_NONE;
        break;

    case USB_CDC_PARITY_EVEN:
        config |= UART_CONFIG_PAR_EVEN;
        break;

    case USB_CDC_PARITY_ODD:
        config |= UART_CONFIG_PAR_ODD;
        break;

    case USB_CDC_PARITY_MARK:
        config |= UART_CONFIG_PAR_ONE;
        break;

    case USB_CDC_PARITY_SPACE:
        config |= UART_CONFIG_PAR_ZERO;
        break;

    default:
        config_valid = false;
        break;
    }

    switch (line_coding->ui8Stop)
    {
    case USB_CDC_STOP_BITS_1:
        config|= UART_CONFIG_STOP_ONE;
        break;

    case USB_CDC_STOP_BITS_2:
        config |= UART_CONFIG_STOP_TWO;
        break;

    default:
        config_valid = false;
        break;
    }

    if (config_valid)
    {
        UARTConfigSetExpClk (UART1_BASE, MAP_SysCtlClockGet(), line_coding->ui8Databits, config);
    }
}

/**
 * @brief Set the UART RTS state to the requested value
 * @param[in] line_state The requested control line state, in CDC format
 */
static void set_control_line_state (const uint32_t line_state)
{
    if (line_state & USB_CDC_ACTIVATE_CARRIER)
    {
        UARTModemControlSet (UART1_BASE, UART_OUTPUT_RTS);
    }
    else
    {
        UARTModemControlClear (UART1_BASE, UART_OUTPUT_RTS);
    }
}

/**
 * @brief Set the break state on the UART connected to the CC3100BOOST
 * @param[in] break_state If true assert break.
 */
static void send_break (bool break_state)
{
    UARTBreakCtl (UART1_BASE, break_state);
}

/**
 * @brief Handles CDC driver notifications related to control and setup of the device.
 */
uint32_t cdc_control_handler(void *pvCBData, uint32_t ui32Event,
                             uint32_t ui32MsgValue, void *pvMsgData)
{
    switch (ui32Event)
    {
    /* Ignore BUS suppend and resume */
    case USB_EVENT_SUSPEND:
    case USB_EVENT_RESUME:
        break;

    case USB_EVENT_CONNECTED:
        /* Light Green LED to indicate connected */
        GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
        break;

    case USB_EVENT_DISCONNECTED:
        /* Turn off Green LED to indicate disconnected.
         * Note that since the USB stack is set to eUSBModeForceDevice, this event isn't received. */
        GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_3, 0);
        break;

    case USBD_CDC_EVENT_GET_LINE_CODING:
        /* Obtain the current UART configuration */
        get_line_coding (pvMsgData);
        break;

    case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
        /* Set the requested modem control line state */
        set_control_line_state (ui32MsgValue);
        break;

    case USBD_CDC_EVENT_SET_LINE_CODING:
        /* Set the UART configuration */
        set_line_coding (pvMsgData);
        break;

    case USBD_CDC_EVENT_SEND_BREAK:
        /* Send a break condition on the serial line.
         * The CC3100BOOST nHIB is asserted for 100ms.
         * The de-assertion of nHIB triggers the CC3100BOOST to communicate with UniFlash. */
        send_break (true);
        assert_nHIB ();
        nHIB_timer_ms = 100;
        nHIB_timer_running = true;
        break;

    case USBD_CDC_EVENT_CLEAR_BREAK:
        /* Clear the break condition on the serial line.
         * Ensure nHIB is de-asserted (this should not be necessary as UniFlash
         * only seems to clear the break condition after communication has been established) */
        send_break (false);
        deassert_nHIB ();
        nHIB_timer_running = false;
        break;

    default:
        check_assert (false);
        break;
    }

    return 0;
}

int main (void)
{
    uint32_t ui32SysClock;

    FPULazyStackingEnable();

    /* Set to maximum 80MHz clock */
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                   SYSCTL_OSC_MAIN);
    ui32SysClock = MAP_SysCtlClockGet();
    check_assert (ui32SysClock == 80000000);

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

    /* Set default UART configuration */
    UARTConfigSetExpClk (UART1_BASE, MAP_SysCtlClockGet(), 115200,
                         UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    /* Enable hardware flow control (not sure if the CC3100BOOST uses it) */
    UARTFlowControlSet (UART1_BASE, UART_FLOWCONTROL_TX);

    /* Configure the GPIO pin for controlling the CC3100BOOST nHIB, initially not asserted */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput (GPIO_PORTE_BASE, GPIO_PIN_4);
    deassert_nHIB ();

    /* Configure the pins for status LEDs, initially all off */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput (GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    /* Set Sys Tick to generate a 1 millisecond tick */
    SysTickPeriodSet (MAP_SysCtlClockGet() / 1000);
    SysTickEnable ();
    SysTickIntEnable ();

    /* Initialize the transmit and receive buffers. */
    check_assert (USBBufferInit (&cdc_tx_buffer) != NULL);
    check_assert (USBBufferInit (&cdc_rx_buffer) != NULL);

    /* Set the USB stack mode to Device mode with no VBUS monitoring.
     * On the EK-TM4C123GXL the USB ID and USB VBUS signals are not connected to PB0 and PB1
     * and so must force Device mode. (PB0 and PB1 are used for the UART connection) */
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    /* Pass our device information to the USB library and place the device on the bus. */
    check_assert (USBDCDCInit(0, &CDC_device) != NULL);

    /* Sleep, as all work is triggered from interrupt handlers */
    for (;;)
    {
        CPUwfi ();
    }

    return 0;
}
