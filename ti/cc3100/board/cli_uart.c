/*
 * cli_uart.c
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*----------------------------------------------------------------------------*/

#include <string.h>

#include <msp432.h>
#include "driverlib.h"

#include "cli_uart.h"

/*----------------------------------------------------------------------------*/

// #define _USE_CLI_

#define UCA0_OS   1    /* 1 = oversampling mode, 0 = low-freq mode */
#define UCA0_BR0  17   /* Value of UCA1BR0 register */
#define UCA0_BR1  0    /* Value of UCA1BR1 register */
#define UCA0_BRS  0    /* Value of UCBRS field in UCA1MCTL register */
#define UCA0_BRF  6    /* Value of UCBRF field in UCA1MCTL register */

#define ASCII_ENTER     0x0D

#ifdef _USE_CLI_
unsigned char *g_ucUARTBuffer;

int cli_have_cmd = 0;
#endif

#define UCBRF_0                (0x00)           /* USCI First Stage Modulation: 0 */
#define UCBRS_3                (0x0600)         /* USCI First Stage Modulation: 3 */

/*----------------------------------------------------------------------------*/

void CLI_Configure(void)
{
#ifdef _USE_CLI_
    /* MSP432P401R = P1.2,3 = USCI_A0 TXD/RXD */
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    /* Use SMCLK, keep RESET */
    UCA0CTLW0 = UCSSEL__SMCLK + UCSWRST;

    /* 48MHz/9600 = 5000 */
    UCA0BR0 = 139;
    UCA0BR1 = 19;

    /* Modulation UCBRSx=3, UCBRFx=0 */
    UCA0MCTLW = UCBRS_3 + UCBRF_0;

    /* Initialize USCI state machine */
    UCA0CTLW0 &= ~UCSWRST;

    /* Disable RX Interrupt on UART */
    UCA0IFG &= ~ (UCRXIFG | UCRXIFG);
    UCA0IE &= ~UCRXIE;
#endif
}

/*----------------------------------------------------------------------------*/

int CLI_Read(unsigned char *pBuff)
{
    if(pBuff == NULL)
        return -1;
#ifdef _USE_CLI_
    cli_have_cmd = 0;
    g_ucUARTBuffer = pBuff;
    UCA0IE |= UCRXIE;

    Interrupt_enableMaster();
    Interrupt_enableInterrupt(INT_EUSCIA0);
    PCM_gotoLPM0();

    // __bis_SR_register(LPM0_bits + GIE);

    while(cli_have_cmd == 0);
    UCA0IE &= ~UCRXIE;

    return strlen((const char *)pBuff);
#else
    return 0;
#endif
}

/*----------------------------------------------------------------------------*/

int CLI_Write(unsigned char *inBuff)
{
    if (inBuff == NULL)
        return -1;
#ifdef _USE_CLI_
    unsigned short ret,usLength = strlen((const char *)inBuff);
    ret = usLength;
    while (usLength)
    {
        while (!(UCA0IFG & UCTXIFG)) ;
        UCA0TXBUF = *inBuff;
        usLength--;
        inBuff++;
    }
    return (int)ret;
#else
    return 0;
#endif
}

/*----------------------------------------------------------------------------*/

void EUSCIA0_IRQHandler(void)
{
    if (UCA0IFG & UCRXIFG)
    {
#ifdef _USE_CLI_
        UCA0IFG &= ~UCRXIFG;
        *g_ucUARTBuffer = UCA1RXBUF;
        if (*g_ucUARTBuffer == ASCII_ENTER)
        {
            cli_have_cmd = 1;
            *g_ucUARTBuffer = 0x00;
            Interrupt_disableSleepOnIsrExit();

            // __bic_SR_register_on_exit(LPM0_bits);
        }
        g_ucUARTBuffer++;
#endif
        __no_operation();
    }
}

/*----------------------------------------------------------------------------*/
