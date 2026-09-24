/**
 * @file ltc_gpio_config.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#ifndef LTC_GPIO_CONFIG_H_
#define LTC_GPIO_CONFIG_H_

#define LTC_OUTPUT_RCU_GPIOx 			RCU_GPIOB
#define LTC_OUTPUT_GPIOx 				GPIOB
#define LTC_OUTPUT_GPIO_PINx 			GPIO_PIN_15
#define LTC_OUTPUT_GPIO_PIN_OFFSET 		15U

#define PPS_INPUT_RCU_GPIOx 			RCU_GPIOA
#define PPS_INPUT_GPIOx 				GPIOA
#define PPS_INPUT_GPIO_PINx 			GPIO_PIN_5
#define PPS_INPUT_EXTI_SOURCE_GPIOx 	EXTI_SOURCE_GPIOA
#define PPS_INPUT_EXTI_SOURCE_PINx 		EXTI_SOURCE_PIN5
#define PPS_INPUT_EXTI_x 				EXTI_5
#define PPS_INPUT_EXTIx_IRQn 			EXTI5_9_IRQn
#define PPS_INPUT_EXTIx_IRQHandler 		EXTI5_9_IRQHandler

#define MIDI_UARTx 						USART2
#define MIDI_UARTx_IRQHandler			USART2_IRQHandler
#define MIDI_UARTx_IRQn					USART2_IRQn

#define PIXEL_OUTPUT_RCU_GPIOx			RCU_GPIOC
#define PIXEL_OUTPUT_GPIOx				GPIOC
#define PIXEL_OUTPUT_GPIO_PINx 			GPIO_PIN_0
#define PIXEL_OUTPUT_GPIO_PIN_OFFSET 	0U

#endif // LTC_GPIO_CONFIG_H_
