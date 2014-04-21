/**
 * @file pal_uart.c
 *
 * @brief PAL UART related functions
 *
 * This file implements ATxmega128A1's UART related transmission
 * and reception functions.
 *
 * $Id: pal_uart.c 19047 2009-11-03 10:22:32Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* === Includes ============================================================= */

#if ((defined UART0) || (defined UART1))
#include <stdint.h>
#include "pal.h"
#include "pal_config.h"
#include "pal_uart.h"

/* === Macros =============================================================== */


/* === Globals ============================================================== */

#if (defined UART0)
uart_communication_buffer_t uart_0_buffer;
#endif

#if (defined UART1)
uart_communication_buffer_t uart_1_buffer;
#endif

/* === Prototypes =========================================================== */


/* === Implementation ======================================================= */

#ifdef UART0
/**
 * @brief Initializes UART 0
 *
 * This function initializes the UART channel 0.
 *
 * @param baud_rate Actual UART baud rate
 */
void sio_uart_0_init(uint32_t baud_rate)
{
    /* Calculate corresponding value for baud rateregister. */
    uint16_t baud_rate_reg = UART_BAUD(baud_rate);

    /* Init Port D pins */
    /* PD3 (TXD0) as output. */
    PORTD.DIRSET = PIN3_bm;
    /* PD2 (RXD0) as input. */
    PORTD.DIRCLR = PIN2_bm;

    /*
     * Microcontroller's USART register is updated to
     * run at the given baud rate.
     */
    /* 4 most siginificant bits of the Baud rate, BSCALE = 0 */
    USARTD0.BAUDCTRLB = (baud_rate_reg >> 8) & 0xFF;
    USARTD0.BAUDCTRLA = (uint8_t)baud_rate_reg;

    /* Faster async mode (UART clock divider = 8, instead of 16) */
    /* Enable Rx and Tx */
    USARTD0.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;

    /* Set 8N1  */
    USARTD0.CTRLC = USART_CHSIZE1_bm | USART_CHSIZE0_bm;

    /*
     * Receive and transmit interrrupt are enabled.
     */
    USARTD0.CTRLA = USART_RXCINTLVL_gm | USART_TXCINTLVL_gm;
}
#endif  /* UART0 */



#ifdef UART1
/**
 * @brief Initializes UART 1
 *
 * This function initializes the UART channel 1.
 *
 * @param baud_rate Actual UART baud rate
 */
void sio_uart_1_init(uint32_t baud_rate)
{
    /* Calculate corresponding value for baud rateregister. */
    uint16_t baud_rate_reg = UART_BAUD(baud_rate);

    /* Init Port D pins */
    /* PD3 (TXD1) as output. */
    PORTD.DIRSET = PIN7_bm;
    /* PD2 (RXD1) as input. */
    PORTD.DIRCLR = PIN6_bm;

    /*
     * Microcontroller's USART register is updated to
     * run at the given baud rate.
     */
    /* 4 most siginificant bits of the Baud rate, BSCALE = 0 */
    USARTD1.BAUDCTRLB = (baud_rate_reg >> 8) & 0xFF;
    USARTD1.BAUDCTRLA = (uint8_t)baud_rate_reg;

    /* Faster async mode (UART clock divider = 8, instead of 16) */
    /* Enable Rx and Tx */
    USARTD1.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;

    /* Set 8N1  */
    USARTD1.CTRLC = USART_CHSIZE1_bm | USART_CHSIZE0_bm;

    /*
     * Receive and transmit interrrupt are enabled.
     */
    USARTD1.CTRLA = USART_RXCINTLVL_gm | USART_TXCINTLVL_gm;
}
#endif  /* UART1 */



#ifdef UART0
/**
 * @brief Transmit data via UART 0
 *
 * This function transmits data via UART channel 0.
 *
 * @param data Pointer to the buffer where the data to be transmitted is present
 * @param length Number of bytes to be transmitted
 *
 * @return Number of bytes actually transmitted
 */
uint8_t sio_uart_0_tx(uint8_t *data, uint8_t length)
{
    uint8_t bytes_to_be_written;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
    uint8_t back;

    /* The transmit interrupt is disabled. */
    DISABLE_UART_0_TX_INT();

    /*
     * Calculate available buffer space
     */
    head = uart_0_buffer.tx_buf_head;
    tail = uart_0_buffer.tx_buf_tail;

    if (tail >= head)
    {
        size = (UART_MAX_TX_BUF_LENGTH - 1) - (tail - head);
    }
    else
    {
        size = (head - 1) - tail;
    }

    if (size < length)
    {
        /* Not enough buffer space available. Use the remaining size. */
        bytes_to_be_written = size;
    }
    else
    {
        bytes_to_be_written = length;
    }

    /* Remember the number of bytes transmitted. */
    back = bytes_to_be_written;

    /* The data is copied to the transmit buffer. */
    while (bytes_to_be_written > 0)
    {
        uart_0_buffer.tx_buf[uart_0_buffer.tx_buf_tail] = *data;

        if ((UART_MAX_TX_BUF_LENGTH - 1) == uart_0_buffer.tx_buf_tail)
        {
            /* Reached the end of buffer, revert back to beginning of buffer. */
            uart_0_buffer.tx_buf_tail = 0;
        }
        else
        {
            /*
             * Increment the index to point the next character to be
             * inserted.
             */
            uart_0_buffer.tx_buf_tail++;
        }

        bytes_to_be_written--;
        data++;
    }

    /*
     * Check whether there is a transmission ongoing. Otherwise write
     * data into the UART data register. Transmission of subsequent
     * bytes / data will be taken care in the ISR.
     */
    if (uart_0_buffer.tx_count == 0)
    {
        USARTD0.DATA = uart_0_buffer.tx_buf[head];
        uart_0_buffer.tx_count = 1;
    }

    /* The transmit interrupt is enabled. */
    ENABLE_UART_0_TX_INT();

    return back;
}
#endif  /* UART0 */



#ifdef UART1
/**
 * @brief Transmit data via UART 1
 *
 * This function transmits data via UART channel 1.
 *
 * @param data Pointer to the buffer where the data to be transmitted is present
 * @param length Number of bytes to be transmitted
 *
 * @return Number of bytes actually transmitted
 */
uint8_t sio_uart_1_tx(uint8_t *data, uint8_t length)
{
    uint8_t bytes_to_be_written;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
    uint8_t back;

    /* The transmit interrupt is disabled. */
    DISABLE_UART_1_TX_INT();

    /*
     * Calculate available buffer space
     */
    head = uart_1_buffer.tx_buf_head;
    tail = uart_1_buffer.tx_buf_tail;

    if (tail >= head)
    {
        size = (UART_MAX_TX_BUF_LENGTH - 1) - (tail - head);
    }
    else
    {
        size = (head - 1) - tail;
    }

    if (size < length)
    {
        /* Not enough buffer space available. Use the remaining size. */
        bytes_to_be_written = size;
    }
    else
    {
        bytes_to_be_written = length;
    }

    /* Remember the number of bytes transmitted. */
    back = bytes_to_be_written;

    /* The data is copied to the transmit buffer. */
    while (bytes_to_be_written > 0)
    {
        uart_1_buffer.tx_buf[uart_1_buffer.tx_buf_tail] = *data;

        if ((UART_MAX_TX_BUF_LENGTH - 1) == uart_1_buffer.tx_buf_tail)
        {
            /* Reached the end of buffer, revert back to beginning of buffer. */
            uart_1_buffer.tx_buf_tail = 0;
        }
        else
        {
            /*
             * Increment the index to point the next character to be
             * inserted.
             */
            uart_1_buffer.tx_buf_tail++;
        }

        bytes_to_be_written--;
        data++;
    }

    /*
     * Check whether there is a transmission ongoing. Otherwise write
     * data into the UART data register. Transmission of subsequent
     * bytes / data will be taken care in the ISR.
     */
    if (uart_1_buffer.tx_count == 0)
    {
        USARTD1.DATA = uart_1_buffer.tx_buf[head];
        uart_1_buffer.tx_count = 1;
    }

    /* The transmit interrupt is enabled. */
    ENABLE_UART_1_TX_INT();

    return back;
}
#endif  /* UART1 */



#ifdef UART0
/**
 * @brief Receives data from UART 0
 *
 * This function receives data from UART channel 0.
 *
 * @param data pointer to the buffer where the received data is to be stored
 * @param max_length maximum length of data to be received
 *
 * @return actual number of bytes received
 */
uint8_t sio_uart_0_rx(uint8_t *data, uint8_t max_length)
{
    uint8_t data_received = 0;

    if (uart_0_buffer.rx_count == 0)
    {
        /* UART receive buffer is empty. */
        return 0;
    }

    /* The receive interrupt is disabled. */
    DISABLE_UART_0_RX_INT();

    if (UART_MAX_RX_BUF_LENGTH <= uart_0_buffer.rx_count)
    {
        /*
         * Bytes between head and tail are overwritten by new data.
         * The oldest data in buffer is the one to which the tail is
         * pointing. So reading operation should start from the tail.
         */
        uart_0_buffer.rx_buf_head = uart_0_buffer.rx_buf_tail;

        /*
         * This is a buffer overflow case. But still only bytes equivalent to
         * full buffer size are useful.
         */
        uart_0_buffer.rx_count = UART_MAX_RX_BUF_LENGTH;

        /* Bytes received is more than or equal to buffer. */
        if (UART_MAX_RX_BUF_LENGTH <= max_length)
        {
            /*
             * Requested receive length (max_length) is more than the
             * max size of receive buffer, but at max the full
             * buffer can be read.
             */
            max_length = UART_MAX_RX_BUF_LENGTH;
        }
    }
    else
    {
        /* Bytes received is less than receive buffer maximum length. */
        if (max_length > uart_0_buffer.rx_count)
        {
            /*
             * Requested receive length (max_length) is more than the data
             * present in receive buffer. Hence only the number of bytes
             * present in receive buffer are read.
             */
            max_length = uart_0_buffer.rx_count;
        }
    }

    data_received = max_length;

    while (max_length > 0)
    {
        /* Start to copy from head. */
        *data = uart_0_buffer.rx_buf[uart_0_buffer.rx_buf_head];
        uart_0_buffer.rx_buf_head++;
        uart_0_buffer.rx_count--;
        data++;
        max_length--;
        if ((UART_MAX_RX_BUF_LENGTH) == uart_0_buffer.rx_buf_head)
        {
            uart_0_buffer.rx_buf_head = 0;
        }
    }

    /* The receive interrupt is enabled back. */
    ENABLE_UART_0_RX_INT();

    return data_received;
}
#endif  /* UART0 */



#ifdef UART1
/**
 * @brief Receives data from UART 1
 *
 * This function receives data from UART channel 1.
 *
 * @param data pointer to the buffer where the received data is to be stored
 * @param max_length maximum length of data to be received
 *
 * @return actual number of bytes received
 */
uint8_t sio_uart_1_rx(uint8_t *data, uint8_t max_length)
{
    uint8_t data_received = 0;

    if (uart_1_buffer.rx_count == 0)
    {
        /* UART receive buffer is empty. */
        return 0;
    }

    /* The receive interrupt is disabled. */
    DISABLE_UART_1_RX_INT();

    if (UART_MAX_RX_BUF_LENGTH <= uart_1_buffer.rx_count)
    {
        /*
         * Bytes between head and tail are overwritten by new data.
         * The oldest data in buffer is the one to which the tail is
         * pointing. So reading operation should start from the tail.
         */
        uart_1_buffer.rx_buf_head = uart_1_buffer.rx_buf_tail;

        /*
         * This is a buffer overflow case. But still only bytes equivalent to
         * full buffer size are useful.
         */
        uart_1_buffer.rx_count = UART_MAX_RX_BUF_LENGTH;

        /* Bytes received is more than or equal to buffer. */
        if (UART_MAX_RX_BUF_LENGTH <= max_length)
        {
            /*
             * Requested receive length (max_length) is more than the
             * max size of receive buffer, but at max the full
             * buffer can be read.
             */
            max_length = UART_MAX_RX_BUF_LENGTH;
        }
    }
    else
    {
        /* Bytes received is less than receive buffer maximum length. */
        if (max_length > uart_1_buffer.rx_count)
        {
            /*
             * Requested receive length (max_length) is more than the data
             * present in receive buffer. Hence only the number of bytes
             * present in receive buffer are read.
             */
            max_length = uart_1_buffer.rx_count;
        }
    }

    data_received = max_length;

    while (max_length > 0)
    {
        /* Start to copy from head. */
        *data = uart_1_buffer.rx_buf[uart_1_buffer.rx_buf_head];
        uart_1_buffer.rx_buf_head++;
        uart_1_buffer.rx_count--;
        data++;
        max_length--;
        if ((UART_MAX_RX_BUF_LENGTH) == uart_1_buffer.rx_buf_head)
        {
            uart_1_buffer.rx_buf_head = 0;
        }
    }

    /* The receive interrupt is enabled back. */
    ENABLE_UART_1_RX_INT();

    return data_received;
}
#endif  /* UART1 */



#ifdef UART0
/**
 * @brief ISR for UART 0 receive interrupt
 *
 * This service routine is executed when a byte is received successfully on
 * UART channel 0.
 */
ISR(USARTD0_RXC_vect)
{
    uint8_t tail = uart_0_buffer.rx_buf_tail;

    /* Count of bytes received through UART 0 channel is incremented. */
    uart_0_buffer.rx_count++;

    uart_0_buffer.rx_buf[tail] = USARTD0.DATA;

    if ((UART_MAX_RX_BUF_LENGTH - 1) == uart_0_buffer.rx_buf_tail)
    {
        /* Revert back to beginning of buffer after reaching end of buffer. */
        uart_0_buffer.rx_buf_tail = 0;
    }
    else
    {
        uart_0_buffer.rx_buf_tail++;
    }
}



/**
 * @brief ISR for UART 0 transmit interrupt
 *
 * This service routine is executed when a byte is transmitted successfully on
 * UART channel 0.
 */
ISR(USARTD0_TXC_vect)
{
    if ((UART_MAX_TX_BUF_LENGTH - 1) == uart_0_buffer.tx_buf_head)
    {
        /* Reached the end of buffer, revert back to beginning of buffer. */
        uart_0_buffer.tx_buf_head = 0;
    }
    else
    {
        /*
         * Increment the index to point the next character to be
         * transmitted.
         */
        uart_0_buffer.tx_buf_head++;
    }

    if (uart_0_buffer.tx_buf_head != uart_0_buffer.tx_buf_tail)
    {
        USARTD0.DATA = uart_0_buffer.tx_buf[uart_0_buffer.tx_buf_head];
    }
    else
    {
        /* No more data for transmission */
        uart_0_buffer.tx_count = 0;
    }
}
#endif  /* UART0 */



#ifdef UART1
/**
 * @brief ISR for UART 1 receive interrupt
 *
 * This service routine is executed when a byte is received successfully on
 * UART channel 1.
 */
ISR(USARTD1_RXC_vect)
{
    uint8_t tail = uart_1_buffer.rx_buf_tail;

    /* Count of bytes received through UART 1 channel is incremented. */
    uart_1_buffer.rx_count++;

    uart_1_buffer.rx_buf[tail] = USARTD1.DATA;

    if ((UART_MAX_RX_BUF_LENGTH - 1) == uart_1_buffer.rx_buf_tail)
    {
         /* Revert back to beginning of buffer after reaching end of buffer. */
        uart_1_buffer.rx_buf_tail = 0;
    }
    else
    {
        uart_1_buffer.rx_buf_tail++;
    }
}



/**
 * @brief ISR for UART 1 transmit interrupt
 *
 * This service routine is executed when a byte is transmitted successfully on
 * UART channel 1.
 */
ISR(USARTD1_TXC_vect)
{
    if ((UART_MAX_TX_BUF_LENGTH - 1) == uart_1_buffer.tx_buf_head)
    {
        /* Reached the end of buffer, revert back to beginning of buffer. */
        uart_1_buffer.tx_buf_head = 0;
    }
    else
    {
        /*
         * Increment the index to point the next character to be
         * transmitted.
         */
        uart_1_buffer.tx_buf_head++;
    }

    if (uart_1_buffer.tx_buf_head != uart_1_buffer.tx_buf_tail)
    {
        USARTD1.DATA = uart_1_buffer.tx_buf[uart_1_buffer.tx_buf_head];
    }
    else
    {
        /* No more data for transmission */
        uart_1_buffer.tx_count = 0;
    }
}
#endif  /* UART1 */

#endif  /* ((defined UART0) || (defined UART1)) */

/* EOF */
