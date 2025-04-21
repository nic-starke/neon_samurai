/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @file adc.h
 * @brief ADC driver header for XMEGA128A4U
 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief ADC channels available
 */
typedef enum {
    ADC_CH0 = 0,
    ADC_CH1,
    ADC_CH2,
    ADC_CH3
} adc_channel_t;

/**
 * @brief ADC reference voltage selection
 */
typedef enum {
    ADC_REF_INT1V     = ADC_REFSEL_INT1V_gc,    /**< Internal 1V reference */
    ADC_REF_INTVCC    = ADC_REFSEL_INTVCC_gc,   /**< Internal VCC/1.6 reference */
    ADC_REF_AREFA     = ADC_REFSEL_AREFA_gc,    /**< External reference on PORTA */
    ADC_REF_AREFB     = ADC_REFSEL_AREFB_gc,    /**< External reference on PORTB */
    ADC_REF_INTVCC2   = ADC_REFSEL_INTVCC2_gc   /**< Internal VCC/2 reference */
} adc_reference_t;

/**
 * @brief ADC result resolution
 */
typedef enum {
    ADC_RES_12BIT     = ADC_RESOLUTION_12BIT_gc,      /**< 12-bit right-adjusted result */
    ADC_RES_8BIT      = ADC_RESOLUTION_8BIT_gc,       /**< 8-bit right-adjusted result */
    ADC_RES_LEFT12BIT = ADC_RESOLUTION_LEFT12BIT_gc   /**< 12-bit left-adjusted result */
} adc_resolution_t;

/**
 * @brief ADC clock prescaler
 */
typedef enum {
    ADC_PRESCALER_DIV4   = ADC_PRESCALER_DIV4_gc,    /**< Divide clock by 4 */
    ADC_PRESCALER_DIV8   = ADC_PRESCALER_DIV8_gc,    /**< Divide clock by 8 */
    ADC_PRESCALER_DIV16  = ADC_PRESCALER_DIV16_gc,   /**< Divide clock by 16 */
    ADC_PRESCALER_DIV32  = ADC_PRESCALER_DIV32_gc,   /**< Divide clock by 32 */
    ADC_PRESCALER_DIV64  = ADC_PRESCALER_DIV64_gc,   /**< Divide clock by 64 */
    ADC_PRESCALER_DIV128 = ADC_PRESCALER_DIV128_gc,  /**< Divide clock by 128 */
    ADC_PRESCALER_DIV256 = ADC_PRESCALER_DIV256_gc,  /**< Divide clock by 256 */
    ADC_PRESCALER_DIV512 = ADC_PRESCALER_DIV512_gc   /**< Divide clock by 512 */
} adc_prescaler_t;

/**
 * @brief ADC channel input mode
 */
typedef enum {
    ADC_CH_INPUTMODE_INTERNAL    = ADC_CH_INPUTMODE_INTERNAL_gc,     /**< Internal inputs, no gain */
    ADC_CH_INPUTMODE_SINGLEENDED = ADC_CH_INPUTMODE_SINGLEENDED_gc,  /**< Single-ended input, no gain */
    ADC_CH_INPUTMODE_DIFF        = ADC_CH_INPUTMODE_DIFF_gc,         /**< Differential input, no gain */
    ADC_CH_INPUTMODE_DIFFWGAIN   = ADC_CH_INPUTMODE_DIFFWGAIN_gc     /**< Differential input, with gain */
} adc_ch_input_mode_t;

/**
 * @brief ADC channel gain values
 */
typedef enum {
    ADC_CH_GAIN_1X  = ADC_CH_GAIN_1X_gc,   /**< 1x gain */
    ADC_CH_GAIN_2X  = ADC_CH_GAIN_2X_gc,   /**< 2x gain */
    ADC_CH_GAIN_4X  = ADC_CH_GAIN_4X_gc,   /**< 4x gain */
    ADC_CH_GAIN_8X  = ADC_CH_GAIN_8X_gc,   /**< 8x gain */
    ADC_CH_GAIN_16X = ADC_CH_GAIN_16X_gc,  /**< 16x gain */
    ADC_CH_GAIN_32X = ADC_CH_GAIN_32X_gc,  /**< 32x gain */
    ADC_CH_GAIN_64X = ADC_CH_GAIN_64X_gc,  /**< 64x gain */
    ADC_CH_GAIN_DIV2 = ADC_CH_GAIN_DIV2_gc /**< x/2 gain */
} adc_ch_gain_t;

/**
 * @brief ADC internal input selection
 */
typedef enum {
    ADC_CH_MUXINT_TEMP    = ADC_CH_MUXINT_TEMP_gc,       /**< Temperature reference */
    ADC_CH_MUXINT_BANDGAP = ADC_CH_MUXINT_BANDGAP_gc,    /**< Bandgap reference */
    ADC_CH_MUXINT_SCALEDVCC = ADC_CH_MUXINT_SCALEDVCC_gc, /**< VCC scaled down by 10 */
    ADC_CH_MUXINT_DAC     = ADC_CH_MUXINT_DAC_gc         /**< DAC output */
} adc_ch_internal_input_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @brief Initialize the ADC module
 *
 * @param reference The reference to use for ADC measurements
 * @param resolution The resolution mode for ADC results
 * @param prescaler The clock prescaler value
 */
void adc_init(adc_reference_t reference, adc_resolution_t resolution, adc_prescaler_t prescaler);

/**
 * @brief Configure an ADC channel for external input
 *
 * @param ch The ADC channel to configure
 * @param input_mode The input mode to use
 * @param gain The gain to apply (only used when input_mode is DIFFWGAIN)
 * @param positive_input The positive input pin selection
 */
void adc_channel_config(adc_channel_t ch, adc_ch_input_mode_t input_mode,
                        adc_ch_gain_t gain, uint8_t positive_input);

/**
 * @brief Configure an ADC channel for internal inputs
 *
 * @param ch The ADC channel to configure
 * @param input The internal input to select
 */
void adc_channel_config_internal(adc_channel_t ch, adc_ch_internal_input_t input);

/**
 * @brief Start a conversion on an ADC channel
 *
 * @param ch The ADC channel on which to start a conversion
 */
void adc_start_conversion(adc_channel_t ch);

/**
 * @brief Check if a conversion is complete on an ADC channel
 *
 * @param ch The ADC channel to check
 * @return true if the conversion is complete, false otherwise
 */
bool adc_is_conversion_complete(adc_channel_t ch);

/**
 * @brief Read the result of a conversion on an ADC channel
 *
 * @param ch The ADC channel to read from
 * @return The conversion result
 */
uint16_t adc_read_result(adc_channel_t ch);

/**
 * @brief Perform a single conversion and return the result
 *
 * @param ch The ADC channel on which to perform the conversion
 * @return The conversion result
 */
uint16_t adc_get_sample(adc_channel_t ch);

/**
 * @brief Read the internal temperature sensor and convert to degrees Celsius
 *
 * @return Temperature in degrees Celsius
 */
int16_t adc_read_temperature(void);

/**
 * @brief Read the internal temperature sensor and convert to degrees Celsius with decimal precision
 *
 * @return Temperature in degrees Celsius as a floating-point value
 */
float adc_read_temperature_float(void);

/**
 * @brief Enable the ADC
 */
void adc_enable(void);

/**
 * @brief Disable the ADC
 */
void adc_disable(void);
