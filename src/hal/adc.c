/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal/signature.h"
#include "hal/adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* Constants for temperature sensor conversion */
#define ADC_TEMP_FACTOR (1.0f / 1.13f) /* Temperature coefficient */
#define ADC_TEMP_OFFSET (-272.8f)			 /* Temperature offset value */

/* Base address for production signature row access (adjust if needed) */
/* NOTE: Direct pointer access to signature row might not be portable or
	 reliable. Using NVM controller commands or specific toolchain functions
	 (e.g., boot_signature_byte_get) is generally safer. This implementation
	 follows the pattern in the original code. */
#define PROD_SIGNATURES_BASE                                                   \
	0x00 // Assuming base address, check datasheet/toolchain

/* Offsets for calibration bytes within the signature row */
#define ADCACAL0_OFFSET 0x20
#define ADCACAL1_OFFSET 0x21
// #define TEMPSENSE0_OFFSET 0x2E // Not used in current simple temp formula
// #define TEMPSENSE1_OFFSET 0x2F // Not used in current simple temp formula

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void adc_wait_for_channel(adc_channel_t channel);
static void adc_load_calibration(void);
static void adc_apply_calibration(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
static ADC_CH_t* adc_get_channel_ptr(adc_channel_t ch);

// Store calibration values read from signature row
static uint8_t adca_cal0;
static uint8_t adca_cal1;
static uint8_t tempsense0;
static uint8_t tempsense1;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @brief Initialize the ADC module
 *
 * This function initializes the ADC module with the specified
 * reference, resolution, and prescaler. It also applies calibration
 * values read from production signatures row.
 */
void adc_init(adc_reference_t reference, adc_resolution_t resolution,
							adc_prescaler_t prescaler) {
	static bool initialized = false;

	if (initialized) {
		/* ADC already initialized, no need to reconfigure */
		return;
	}
	/* Disable the ADC before configuring */
	ADCA.CTRLA &= (uint8_t)(~(uint8_t)ADC_ENABLE_bm);

	/* Load calibration data from signature row */
	adc_load_calibration();

	/* Set up the ADC control registers */
	ADCA.CTRLB =
			(ADCA.CTRLB & (uint8_t)(~(uint8_t)ADC_RESOLUTION_gm)) | resolution;
	ADCA.REFCTRL =
			(ADCA.REFCTRL & (uint8_t)(~(uint8_t)ADC_REFSEL_gm)) | reference;
	ADCA.PRESCALER =
			(ADCA.PRESCALER & (uint8_t)(~(uint8_t)ADC_PRESCALER_gm)) | prescaler;

	/* Set current limit to ensure accuracy at higher sample rates */
	ADCA.CTRLB |= ADC_CURRLIMIT_HIGH_gc;

	/* Apply calibration values */
	adc_apply_calibration();

	/* Enable the ADC module */
	ADCA.CTRLA |= ADC_ENABLE_bm;

	/* Wait for ADC to stabilize */
	_delay_us(10);

	/* Wait for ADC to be ready (check if enable bit is set in CTRLA) */
	while (!(ADCA.CTRLA & ADC_ENABLE_bm))
		;
}

/**
 * @brief Configure an ADC channel
 *
 * This function configures the specified ADC channel with the given
 * input mode, gain, and positive input.
 */
void adc_channel_config(adc_channel_t ch, adc_ch_input_mode_t input_mode,
												adc_ch_gain_t gain, uint8_t positive_input) {
	ADC_CH_t* adc_ch = adc_get_channel_ptr(ch);

	/* Configure the channel's control register */
	adc_ch->CTRL = (adc_ch->CTRL &
									(uint8_t)(~(uint8_t)(ADC_CH_INPUTMODE_gm | ADC_CH_GAIN_gm))) |
								 input_mode;

	/* Set gain if using differential with gain */
	if (input_mode == ADC_CH_INPUTMODE_DIFFWGAIN) {
		adc_ch->CTRL |= gain;
	}

	/* Set the positive input pin */
	/* Clear MUX settings first */
	adc_ch->MUXCTRL = 0;
	adc_ch->MUXCTRL = (adc_ch->MUXCTRL & (uint8_t)(~(uint8_t)ADC_CH_MUXPOS_gm)) |
										positive_input;

	/* For single-ended measurements, INPUTMODE setting handles negative input.
		 No need to explicitly set MUXNEG. */
}

/**
 * @brief Configure an ADC channel for internal input
 *
 * This function sets up a channel to read from internal sources like
 * the temperature sensor, bandgap reference, etc.
 */
void adc_channel_config_internal(adc_channel_t					 ch,
																 adc_ch_internal_input_t input) {
	ADC_CH_t* adc_ch = adc_get_channel_ptr(ch);

	/* Set input mode to internal */
	adc_ch->CTRL = (adc_ch->CTRL & (uint8_t)(~(uint8_t)ADC_CH_INPUTMODE_gm)) |
								 ADC_CH_INPUTMODE_INTERNAL_gc;

	/* Configure the internal input mux */
	adc_ch->MUXCTRL =
			(adc_ch->MUXCTRL & (uint8_t)(~(uint8_t)ADC_CH_MUXINT_gm)) | input;
}

/**
 * @brief Start a conversion on an ADC channel
 */
void adc_start_conversion(adc_channel_t ch) {
	ADC_CH_t* adc_ch = adc_get_channel_ptr(ch);

	/* Set channel start conversion bit */
	adc_ch->CTRL |= ADC_CH_START_bm;
}

/**
 * @brief Check if a conversion is complete on an ADC channel
 *
 * @return true if the conversion is complete, false otherwise
 */
bool adc_is_conversion_complete(adc_channel_t ch) {
	/* Check the corresponding interrupt flag */
	switch (ch) {
		case ADC_CH0: return (ADCA.INTFLAGS & ADC_CH0IF_bm) != 0;
		case ADC_CH1: return (ADCA.INTFLAGS & ADC_CH1IF_bm) != 0;
		case ADC_CH2: return (ADCA.INTFLAGS & ADC_CH2IF_bm) != 0;
		case ADC_CH3: return (ADCA.INTFLAGS & ADC_CH3IF_bm) != 0;
		default: return false;
	}
}

/**
 * @brief Read the result of a conversion on an ADC channel
 *
 * This function reads and returns the ADC result after a conversion.
 * The result format depends on the resolution setting.
 */
uint16_t adc_read_result(adc_channel_t ch) {
	uint16_t result;

	/* Get the result based on channel number */
	switch (ch) {
		case ADC_CH0: result = ADCA.CH0RES; break;
		case ADC_CH1: result = ADCA.CH1RES; break;
		case ADC_CH2: result = ADCA.CH2RES; break;
		case ADC_CH3: result = ADCA.CH3RES; break;
		default: result = 0; break;
	}

	/* Clear the corresponding interrupt flag */
	switch (ch) {
		case ADC_CH0: ADCA.INTFLAGS = ADC_CH0IF_bm; break;
		case ADC_CH1: ADCA.INTFLAGS = ADC_CH1IF_bm; break;
		case ADC_CH2: ADCA.INTFLAGS = ADC_CH2IF_bm; break;
		case ADC_CH3: ADCA.INTFLAGS = ADC_CH3IF_bm; break;
		default: break;
	}

	return result;
}

/**
 * @brief Perform a single conversion and return the result
 *
 * This function starts a conversion, waits for it to complete,
 * and returns the result.
 */
uint16_t adc_get_sample(adc_channel_t ch) {
	/* Start the conversion */
	adc_start_conversion(ch);

	/* Wait for conversion to complete */
	adc_wait_for_channel(ch);

	/* Return the conversion result */
	return adc_read_result(ch);
}

/**
 * @brief Read the temperature sensor and convert to degrees Celsius
 *
 * This function reads the internal temperature sensor and converts the
 * ADC reading to a temperature in degrees Celsius using the factory
 * calibration values stored in the production signature row.
 *
 * The conversion formula from the XMEGA datasheet is:
 * Temperature (°C) = ((ADC reading - TEMPSENSE0) * TEMPSENSE1) / 256 + 25°C
 *
 * @return float The temperature in degrees Celsius with decimal precision
 */
float adc_read_temperature_float(void) {
	uint16_t							adc_value;
	float									temperature;
	NVM_PROD_SIGNATURES_t sig_data;

	/* Read calibration values from signature row */
	signature_read(&sig_data);

	/* Configure channel 0 for temperature measurement */
	adc_channel_config_internal(ADC_CH0, ADC_CH_MUXINT_TEMP);

	/* Get ADC reading */
	adc_value = adc_get_sample(ADC_CH0);

	/*
	 * Calculate temperature using calibration values from signature row.
	 * Formula from XMEGA datasheet:
	 * Temperature = ((ADC reading - TEMPSENSE0) * TEMPSENSE1) / 256 + 25°C
	 */

	/* Using floating point for more precision */
	temperature = ((float)adc_value - (float)sig_data.TEMPSENSE0);
	temperature = (temperature * (float)sig_data.TEMPSENSE1) / 256.0f + 25.0f;

	return temperature;
}

/**
 * @brief Read the temperature sensor and convert to degrees Celsius
 *
 * This function reads the internal temperature sensor and converts the
 * ADC reading to a temperature in degrees Celsius using the factory
 * calibration values stored in the production signature row.
 *
 * The conversion formula from the XMEGA datasheet is:
 * Temperature (°C) = ((ADC reading - TEMPSENSE0) * TEMPSENSE1) / 256 + 25°C
 *
 * @return int16_t The temperature in degrees Celsius (can be negative)
 */
int16_t adc_read_temperature(void) {
	/* Call the floating point version and round to integer */
	float temp = adc_read_temperature_float();
	return (int16_t)(temp + 0.5f); /* Round to nearest integer */
}

/**
 * @brief Enable the ADC
 */
void adc_enable(void) {
	ADCA.CTRLA |= ADC_ENABLE_bm;
}

/**
 * @brief Disable the ADC
 */
void adc_disable(void) {
	ADCA.CTRLA &= (uint8_t)(~(uint8_t)ADC_ENABLE_bm);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @brief Get a pointer to the channel structure
 *
 * @param ch The channel number
 * @return Pointer to the channel structure
 */
static ADC_CH_t* adc_get_channel_ptr(adc_channel_t ch) {
	switch (ch) {
		case ADC_CH0: return &ADCA.CH0;
		case ADC_CH1: return &ADCA.CH1;
		case ADC_CH2: return &ADCA.CH2;
		case ADC_CH3: return &ADCA.CH3;
		default: return &ADCA.CH0;
	}
}

/**
 * @brief Wait until conversion is complete on a channel
 */
static void adc_wait_for_channel(adc_channel_t channel) {
	uint8_t int_flags;
	uint8_t int_mask;

	/* Determine which flag to check based on channel */
	switch (channel) {
		case ADC_CH0: int_mask = ADC_CH0IF_bm; break;
		case ADC_CH1: int_mask = ADC_CH1IF_bm; break;
		case ADC_CH2: int_mask = ADC_CH2IF_bm; break;
		case ADC_CH3: int_mask = ADC_CH3IF_bm; break;
		default: int_mask = 0; break;
	}

	/* Wait for conversion to complete */
	do {
		int_flags = ADCA.INTFLAGS;
	} while (!(int_flags & int_mask));
}

/**
 * @brief Load calibration values from production signature row
 */
static void adc_load_calibration(void) {
	NVM_PROD_SIGNATURES_t prod_sig;
	signature_read(&prod_sig);
	adca_cal0 = prod_sig.ADCACAL0;
	adca_cal1 = prod_sig.ADCACAL1;

	tempsense0 = prod_sig.TEMPSENSE0;
	tempsense1 = prod_sig.TEMPSENSE1;
}

/**
 * @brief Apply loaded calibration values to ADC registers
 */
static void adc_apply_calibration(void) {
	/* Apply calibration values to the ADC calibration registers */
	ADCA.CALL = adca_cal0;
	ADCA.CALH = adca_cal1;
}
