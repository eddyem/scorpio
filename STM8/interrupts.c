/*
 * interrupts.c
 *
 * Copyright 2014 Edward V. Emelianoff <eddy@sao.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "ports_definition.h"
#include "stepper.h"
#include "uart.h"

// Top Level Interrupt
INTERRUPT_HANDLER(TLI_IRQHandler, 0){}

// Auto Wake Up Interrupt
INTERRUPT_HANDLER(AWU_IRQHandler, 1){}

// Clock Controller Interrupt
INTERRUPT_HANDLER(CLK_IRQHandler, 2){}

// External Interrupt PORTA
INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler, 3){}

// External Interrupt PORTB
INTERRUPT_HANDLER(EXTI_PORTB_IRQHandler, 4){}

// External Interrupt PORTC
INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5){}

// External Interrupt PORTD
INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6){
}

// External Interrupt PORTE
INTERRUPT_HANDLER(EXTI_PORTE_IRQHandler, 7){}

#ifdef STM8S903
// External Interrupt PORTF
INTERRUPT_HANDLER(EXTI_PORTF_IRQHandler, 8){}
#endif // STM8S903

#if defined (STM8S208) || defined (STM8AF52Ax)
// CAN RX Interrupt routine.
INTERRUPT_HANDLER(CAN_RX_IRQHandler, 8){}

// CAN TX Interrupt routine.
INTERRUPT_HANDLER(CAN_TX_IRQHandler, 9){}
#endif // STM8S208 || STM8AF52Ax

// SPI Interrupt routine.
INTERRUPT_HANDLER(SPI_IRQHandler, 10){}

// Timer1 Update/Overflow/Trigger/Break Interrupt
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_BRK_IRQHandler, 11){
    if(TIM1_SR1 & TIM_SR1_UIF){ // update interrupt
//        Global_time++; // increase timer
    }
    TIM1_SR1 = 0; // clear all interrupt flags
}

// Timer1 Capture/Compare Interrupt routine.
INTERRUPT_HANDLER(TIM1_CAP_COM_IRQHandler, 12){}

#ifdef STM8S903
// Timer5 Update/Overflow/Break/Trigger Interrupt
INTERRUPT_HANDLER(TIM5_UPD_OVF_BRK_TRG_IRQHandler, 13){}

// Timer5 Capture/Compare Interrupt
INTERRUPT_HANDLER(TIM5_CAP_COM_IRQHandler, 14){}

#else // STM8S208, STM8S207, STM8S105 or STM8S103 or STM8AF62Ax or STM8AF52Ax or STM8AF626x

volatile char Ustep = 0;
// Timer2 Update/Overflow/Break Interrupt
INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13){ // generate pulses for stepper CLK
    char tmp;
    U8 sw;
    if(TIM2_SR1 & TIM_SR1_UIF){
        TIM2_SR1 &= ~TIM_SR1_UIF; // take off flag
        tmp = PORT(STP_PORT, ODR) & ~STP_PINS;
        PORT(STP_PORT, ODR) = tmp | usteps[Ustep];

        if(Steps_left == 0){
            stop_motor();
            return;
        }

        sw = check_endsw();
        if(Dir){
            if(--Ustep < 0){
                Ustep = 7;
                --Steps_left;
                if(sw == 1){
                    stop_motor();
                    return;
                }
            }
        }else{
            if(Ustep == 0 && sw == 2){ // check end-switches only @ full steps
                stop_motor();
                return;
            }
            if(++Ustep > 7){
                Ustep = 0;
                --Steps_left;

            }
        }
    }
}


// Timer2 Capture/Compare Interrupt
INTERRUPT_HANDLER(TIM2_CAP_COM_IRQHandler, 14){
}
#endif // STM8S903

#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S105) || \
    defined(STM8S005) ||  defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8AF626x)
// Timer3 Update/Overflow/Break Interrupt
INTERRUPT_HANDLER(TIM3_UPD_OVF_BRK_IRQHandler, 15){}

// Timer3 Capture/Compare Interrupt
INTERRUPT_HANDLER(TIM3_CAP_COM_IRQHandler, 16){}
#endif // STM8S208, STM8S207 or STM8S105 or STM8AF62Ax or STM8AF52Ax or STM8AF626x

#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S103) || \
    defined(STM8S003) ||  defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8S903)
// UART1 TX Interrupt
INTERRUPT_HANDLER(UART1_TX_IRQHandler, 17){}

// UART1 RX Interrupt
INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18){}
#endif // STM8S208 or STM8S207 or STM8S103 or STM8S903 or STM8AF62Ax or STM8AF52Ax

// I2C Interrupt
INTERRUPT_HANDLER(I2C_IRQHandler, 19){}

#if defined(STM8S105) || defined(STM8S005) ||  defined (STM8AF626x)
// UART2 TX interrupt
INTERRUPT_HANDLER(UART2_TX_IRQHandler, 20){
    if(UART2_SR & UART_SR_TXE){
        if(tx_len == 0){
            UART2_CR2 &= ~UART_CR2_TIEN; // disable TXE interrupt
            tx_idx = 0;
            return;
        }
        if(tx_idx < tx_len){
            UART2_DR = UART_tx[tx_idx++];
        }else{
            UART2_CR2 &= ~UART_CR2_TIEN;
            tx_idx = 0;
            tx_len = 0;
            return;
        }
    }
}

// UART2 RX interrupt
INTERRUPT_HANDLER(UART2_RX_IRQHandler, 21){
    U8 rb;
    if(UART2_SR & UART_SR_RXNE){ // data received
        rb = UART2_DR; // read received byte & clear RXNE flag
        //while(!(UART2_SR & UART_SR_TXE));
        UART_rx[rx_idx++] = rb; // put received byte into cycled buffer
        //UART2_DR = rb;
        if(rx_idx == UART_BUF_LEN && rb != '\n'){ // Oops: buffer overflow! Just forget old data
            rx_idx = 0;
            return;
        }
        if(rb == '\n'){
            uart_rdy = 1;
            UART_rx[rx_idx] = 0;
        }
    }
}
#endif // STM8S105 or STM8AF626x

#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
// UART3 TX interrupt
INTERRUPT_HANDLER(UART3_TX_IRQHandler, 20){}

// UART3 RX interrupt
INTERRUPT_HANDLER(UART3_RX_IRQHandler, 21){}
#endif // STM8S208 or STM8S207 or STM8AF52Ax or STM8AF62Ax

#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
// ADC2 interrupt
INTERRUPT_HANDLER(ADC2_IRQHandler, 22){}
#else
// ADC1 interrupt
INTERRUPT_HANDLER(ADC1_IRQHandler, 22){
}
#endif // STM8S208 or STM8S207 or STM8AF52Ax or STM8AF62Ax

#ifdef STM8S903
// Timer6 Update/Overflow/Trigger Interrupt
INTERRUPT_HANDLER(TIM6_UPD_OVF_TRG_IRQHandler, 23){}
#else // STM8S208, STM8S207, STM8S105 or STM8S103 or STM8AF52Ax or STM8AF62Ax or STM8AF626x
// Timer4 Update/Overflow Interrupt
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23){}
#endif // STM8S903

// Eeprom EEC Interrupt
INTERRUPT_HANDLER(EEPROM_EEC_IRQHandler, 24){}
