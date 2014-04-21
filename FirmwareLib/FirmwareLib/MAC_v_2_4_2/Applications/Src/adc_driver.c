/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief
 *      XMEGA ADC driver source file.
 *
 *      This file contains the function implementations the XMEGA ADC driver.
 *
 *      The driver is not intended for size and/or speed critical code, since
 *      most functions are just a few lines of code, and the function call
 *      overhead would decrease code performance. The driver is intended for
 *      rapid prototyping and documentation purposes for getting started with
 *      the XMEGA ADC module.
 *
 *      For size and/or speed critical code, it is recommended to copy the
 *      function contents directly into your application instead of making
 *      a function call.
 *
 *      Several functions use the following construct:
 *          "some_register = ... | (some_parameter ? SOME_BIT_bm : 0) | ..."
 *      Although the use of the ternary operator ( if ? then : else ) is discouraged,
 *      in some occasions the operator makes it possible to write pretty clean and
 *      neat code. In this driver, the construct is used to set or not set a
 *      configuration bit based on a boolean input parameter, such as
 *      the "some_parameter" in the example above.
 *
 * \par Application note:
 *      AVR1300: Using the XMEGA ADC
 *
 * \par Documentation
 *      For comprehensive code documentation, supported compilers, compiler
 *      settings and supported devices see readme.html
 *
 * \author
 *      Atmel Corporation: http://www.atmel.com \n
 *      Support email: avr@atmel.com
 *
 * $Revision: 1714 $
 * $Date: 2008-08-13 11:09:37 +0200 (on, 13 aug 2008) $  \n
 *
 * Copyright (c) 2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITD TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <stdbool.h>
// LSR
//#include "platform_types.h"
// ^
#include "pal.h"
#include "adc_driver.h"


/*! \brief This function get the calibration data from the production calibration.
 *
 *  The calibration data is loaded from flash and stored in the calibration
 *  register. The calibration data reduces the gain error in the adc.
 *  \param  adc          Pointer to ADC module register section.
 */
void ADC_CalibrationValues_Set(ADC_t * adc)
{
	if(&ADCA == adc)
	{
		 // Get ADCCAL from bytes 0x20 (CALL) and 0x21 (CALH) (Word address 0x10).
// LSR
//		adc->CALIB = SP_ReadCalibrationByte(0x20);
		adc->CALL = SP_ReadCalibrationByte(0x20);
        adc->CALH = SP_ReadCalibrationByte(0x21);
// ^
	}
	else
	{
		// Get ADCCAL from bytes 0x24 (CALL) and 0x25 (CALH) (Word address 0x12).
// LSR
//              adc->CALIB = SP_ReadCalibrationByte(ADCBCAL0);
		adc->CALL = SP_ReadCalibrationByte(0x24);
		adc->CALH = SP_ReadCalibrationByte(0x25);
// ^
	}
}


/*! \brief This function clears the interrupt flag and returns the conversion result.
 *
 *	This function should be used together with the ADC_Ch_Conversion_Complete.
 *      When the conversion result is ready this function reads out the result.
 *
 *  \param  adc_ch  Pointer to ADC channel register section.
 *  \param  offset  Offset value that is compensated for.
 *  \return  Conversion result.
 */
uint16_t ADC_ResultCh_GetWord(ADC_CH_t * adc_ch, uint8_t offset)
{
  	uint16_t answer;

/*
  	uint16_t signedOffset = (uint16_t) offset;

	// Append 16-bit signed value if offset (signed) is negative.
	if (offset >= 128)
	{
		signedOffset |= 0xFF00;
	}
*/

	// Clear interrupt flag.
	adc_ch->INTFLAGS = ADC_CH_CHIF_bm;

	// Return result register contents
//	answer = adc_ch->RES - signedOffset;
	answer = adc_ch->RES - offset;

	return answer;
}


/*! \brief This function clears the interrupt flag and returns the low byte of the conversion result.
 *
 *	This function should be used together with the ADC_Ch_Conversion_Complete.
 *      When the conversion result is ready this function reads out the result.
 *
 *  \note  If this function is used with 12-bit right adjusted results, it
 *         returns the 8 LSB only.
 *
 *  \param  adc_ch  Pointer to ADC channel register section.
 *  \param  offset  Compensation offset value.
 *
 *  \return  Low byte of conversion result.
 */
uint8_t ADC_ResultCh_GetLowByte(ADC_CH_t * adc_ch, uint8_t offset)
{

  	uint8_t answer;

	/* Clear interrupt flag.*/
	adc_ch->INTFLAGS = ADC_CH_CHIF_bm;
	/* Return result register contents*/
	answer = adc_ch->RESL - offset;

	return answer;
}

/*! \brief This function clears the interrupt flag and returns the high byte of the conversion result.
 *
 *	This function should be used together with the ADC_ResultCh_ConversionComplete.
 *      When the conversion result is ready this function reads out the result.
 *
 *  \note  If this function is used with 12-bit right adjusted results, it
 *         returns the 8 LSB only. Offset is not compensated.
 *
 *  \note  If ADC interrupts are enabled, the interrupt handler will clear the
 *         interrupt flag before the polling loop can detect it, and this function
 *         will hang forever. Therefore, do not use interrupts with this function.
 *         Instead, read the result registers directly.
 *
 *  \param  adc_ch  Pointer to ADC channel register section.
 *
 *  \return  High byte of conversion result.
 */
uint8_t ADC_ResultCh_GetHighByte(ADC_CH_t * adc_ch)
{
	/* Clear interrupt flag.*/
	adc_ch->INTFLAGS = ADC_CH_CHIF_bm;

	/* Return low byte result register contents.*/
	return adc_ch->RESH;
}

/*! \brief This function waits until the adc common mode is settled.
 *
 *  After the ADC clock has been turned on, the common mode voltage in the ADC
 *  need some time to settle. The time it takes equals one dummy conversion.
 *  Instead of doing a dummy conversion this function waits until the common
 *  mode is settled.
 *
 *  \note The function sets the prescaler to the minimum value to minimize the
 *        time it takes the common mode to settle. If the clock speed is higher
 *        than 8 MHz use the ADC_wait_32MHz function.
 *
 *  \param  adc Pointer to ADC module register section.
 */
void ADC_Wait_8MHz(ADC_t * adc)
{
  	/* Store old prescaler value. */
  	uint8_t prescaler_val = adc->PRESCALER;

	/* Set prescaler value to minimum value. */
	adc->PRESCALER = ADC_PRESCALER_DIV4_gc;

	/* Wait 4 x COMMON_MODE_CYCLES for common mode to settle. */
	pal_timer_delay(4 * COMMON_MODE_CYCLES);

	/* Set prescaler to old value*/
	adc->PRESCALER = prescaler_val;
}


/*! \brief This function waits until the adc common mode is settled.
 *
 *  After the ADC clock has been turned on, the common mode voltage in the ADC
 *  need some time to settle. The time it takes equals one dummy conversion.
 *  Instead of doing a dummy conversion this function waits until the common
 *  mode is settled.
 *
 *  \note The function sets the prescaler to the minimum value possible when the
 *        clock speed is larger than 8 MHz to minimize the time it takes the
 *        common mode to settle.
 *
 *  \note The ADC clock is turned off every time the ADC i disabled or the
 *        device goes into sleep (not Idle sleep mode).
 *
 *  \param  adc Pointer to ADC module register section.
 */
void ADC_Wait_32MHz(ADC_t * adc)
{
  	/* Store old prescaler value. */
  	uint8_t prescaler_val = adc->PRESCALER;

	/* Set prescaler value to minimum value. */
	adc->PRESCALER = ADC_PRESCALER_DIV8_gc;

	/* wait 8 x COMMON_MODE_CYCLES for common mode to settle*/
	pal_timer_delay(8 * COMMON_MODE_CYCLES);

	/* Set prescaler to old value*/
	adc->PRESCALER = prescaler_val;
}

/*! \brief This function get the offset of the ADC
 *
 *   This function make an internal coupling to the same pin and calculate
 *   the internal offset in the ADC.
 *
 *  \note This function only return the low byte of the 12-bit conversion,
 *        because the offset should never be more than +-8 LSB off.
 *
 *  \param adc Pointer to the ADC to calculate offset from.
 *
 *  \return Offset on the selected ADC
 */
uint8_t ADC_Offset_Get(ADC_t * adc)
{
	uint8_t offset;

  	/* Set up ADC to get offset. */
  	ADC_ConvMode_and_Resolution_Config(adc, true, ADC_RESOLUTION_12BIT_gc);

	ADC_Prescaler_Config(adc, ADC_PRESCALER_DIV8_gc);

	ADC_Reference_Config(adc, ADC_REFSEL_VCC_gc);

	ADC_Ch_InputMode_and_Gain_Config(&(adc->CH0),
	                                 ADC_CH_INPUTMODE_DIFF_gc,
	                                 ADC_CH_GAIN_1X_gc);

	ADC_Ch_InputMux_Config(&(adc->CH0), ADC_CH_MUXPOS_PIN0_gc, ADC_CH_MUXNEG_PIN0_gc);

	/* Enable ADC. */
	ADC_Enable(adc);

	/* Wait until ADC is ready. */
	ADC_Wait_32MHz(adc);

	/* Do one conversion to find offset. */
	ADC_Ch_Conversion_Start(&(adc->CH0));

	do{
	}while(!ADC_Ch_Conversion_Complete(&(adc->CH0)));
	offset = ADC_ResultCh_GetLowByte(&(adc->CH0), 0x00);

	/* Disable ADC. */
	ADC_Disable(adc);

	return offset;
}

#ifdef __GNUC__

/*! \brief Function for GCC to read out calibration byte.
 *
 *  \note For IAR support, include the adc_driver_asm.S90 file in your project.
 *
 *  \param index The index to the calibration byte.
 *
 *  \return Calibration byte.
 */
uint8_t SP_ReadCalibrationByte( uint8_t index )
{
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
 	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
 	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return result;
}

#endif
