/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "console/console.h"
#include "event/event.h"
#include "event/sys.h"
#include "hal/adc.h" // Add ADC header for temperature reading
#include "hal/signature.h"
#include "hal/sys.h"
#include "led/color.h"	// Add color header for HSV functions
#include "system/rng.h" // Add RNG header for accessing seed value
#include "system/hardware.h"
#include "usb/usb.h"

#include <LUFA/Drivers/USB/Class/Device/CDCClassDevice.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define CONSOLE_PROMPT PSTR("> ")

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef ENABLE_CONSOLE
// Need the LUFA CDC device info structure
extern USB_ClassInfo_CDC_Device_t lufa_usb_cdc_device;
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Enum defining command identifiers (conceptual, not directly used for indexing
// here)
enum console_command_id {
	CMD_RESET,
	CMD_HELP,
	// Add new command IDs here
	CMD_COUNT // Keep this last for array sizing if needed elsewhere
};

// Function pointer type for command handlers
typedef void (*command_handler_t)(const char* args);

// Structure to define a console command
typedef struct {
	const char* name; // Command name (stored in PROGMEM)
	command_handler_t
							handler;	 // Function pointer to the handler (stored in PROGMEM)
	const char* help_text; // Help text for the command (stored in PROGMEM)
} console_command_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void process_line(const char* line);

static void handle_reset(const char* args);
static void handle_help(const char* args);
static void handle_config_reset(const char* args);
static void handle_signature(const char* args);
static void handle_temperature(const char* args);
static void handle_rng_seed(const char* args); // New RNG seed command handler
static void handle_set_vmap_hsv(const char* args);

static int console_sys_event_handler(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static char		 line_buffer[CONSOLE_LINE_BUFFER_SIZE];
static uint8_t line_buffer_index = 0;
static bool		 needs_prompt			 = true;

// Command table stored in PROGMEM, initialized using the macro
// Define command strings in PROGMEM
static const char reset_command_name[] PROGMEM = "reset";
static const char reset_command_help[] PROGMEM = "Resets the device";

static const char help_command_name[] PROGMEM = "help";
static const char help_command_help[] PROGMEM = "Shows this help message";

static const char config_reset_command_name[] PROGMEM = "reset_cfg";
static const char config_reset_command_help[] PROGMEM =
		"Performs reset to factory defaults";

static const char signature_command_name[] PROGMEM = "signature";
static const char signature_command_help[] PROGMEM =
		"Displays the production signature row values";

static const char temperature_command_name[] PROGMEM = "temp";
static const char temperature_command_help[] PROGMEM =
		"Reads and displays the internal temperature in Celsius";

static const char rng_seed_command_name[] PROGMEM = "rngseed";
static const char rng_seed_command_help[] PROGMEM =
		"Displays the current random number generator seed value";

// New command definitions for HSV color system
static const char set_vmap_hsv_name[] PROGMEM = "set_vmap_hsv";
static const char set_vmap_hsv_help[] PROGMEM =
		"Sets HSV values for vmap: <bank> <enc> <vmap_idx> <H (0-1535)> <S "
		"(0-255)> <V (0-255)>";

static const console_command_t commands[] PROGMEM = {
		{.name			= help_command_name,
		 .handler		= handle_help,
		 .help_text = help_command_help},
		{.name			= reset_command_name,
		 .handler		= handle_reset,
		 .help_text = reset_command_help},
		{.name			= config_reset_command_name,
		 .handler		= handle_config_reset,
		 .help_text = config_reset_command_help},
		{.name			= signature_command_name,
		 .handler		= handle_signature,
		 .help_text = signature_command_help},
		{.name			= temperature_command_name,
		 .handler		= handle_temperature,
		 .help_text = temperature_command_help},
		{.name			= rng_seed_command_name,
		 .handler		= handle_rng_seed,
		 .help_text = rng_seed_command_help},
		{.name			= set_vmap_hsv_name,
		 .handler		= handle_set_vmap_hsv,
		 .help_text = set_vmap_hsv_help},
};

static const uint8_t num_commands = sizeof(commands) / sizeof(commands[0]);

// Event handler structure for system events
static struct event_ch_handler console_sys_evt_handler_def = {
		.handler	= &console_sys_event_handler,
		.next			= NULL,
		.priority = 1,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void console_init(void) {
	line_buffer_index = 0;
	line_buffer[0]		= '\0';
	needs_prompt			= true;
	// Subscribe to system events
	event_channel_subscribe(EVENT_CHANNEL_SYS, &console_sys_evt_handler_def);
}

void console_update(void) {
#ifdef ENABLE_CONSOLE
	if (!usb_cdc_is_active()) {
		needs_prompt = true; // Reset prompt state if disconnected
		return;
	}

	// Print prompt if needed
	if (needs_prompt) {
		console_puts_p(CONSOLE_PROMPT);
		needs_prompt = false;
	}

	int16_t received_byte = CDC_Device_ReceiveByte(&lufa_usb_cdc_device);

	if (received_byte >= 0) {
		char c = (char)received_byte;

		// Handle backspace/delete
		if ((c == '\b' || c == 127) && line_buffer_index > 0) {
			line_buffer_index--;
			line_buffer[line_buffer_index] = '\0';
			// Echo backspace, space, backspace to erase character on terminal
			console_putc('\b');
			console_putc(' ');
			console_putc('\b');
		} else if (c == '\t') { // Handle Tab key for help
			console_putc('\r');
			console_putc('\n');
			handle_help(NULL);				// Call help handler directly
			line_buffer_index = 0;		// Reset buffer
			needs_prompt			= true; // Need a new prompt
		} else if (c == '\r' || c == '\n') {
			// End of line
			console_putc('\r');										 // Echo CR
			console_putc('\n');										 // Echo LF
			line_buffer[line_buffer_index] = '\0'; // Null-terminate
			if (line_buffer_index > 0) {					 // Only process if not empty
				process_line(line_buffer);					 // Use renamed function
			}
			line_buffer_index = 0;		// Reset buffer
			needs_prompt			= true; // Need a new prompt
		} else if (isprint(c) &&
							 line_buffer_index < (CONSOLE_LINE_BUFFER_SIZE - 1)) {
			// Store printable characters
			line_buffer[line_buffer_index++] = c;
			console_putc(c); // Echo character
		}
		// Flush output buffer periodically or after specific actions
		CDC_Device_Flush(&lufa_usb_cdc_device);
	}
#endif // ENABLE_CONSOLE
}

void console_putc(char c) {
#ifdef ENABLE_CONSOLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendByte(&lufa_usb_cdc_device, c);
		// Consider flushing here or let console_update handle it
	}
#endif
}

void console_puts(const char* str) {
#ifdef ENABLE_CONSOLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendString(&lufa_usb_cdc_device, str);
		CDC_Device_Flush(&lufa_usb_cdc_device); // Flush after sending a string
	}
#endif
}

void console_puts_p(const char* str_p) {
#ifdef ENABLE_CONSOLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendString_P(&lufa_usb_cdc_device, str_p);
		CDC_Device_Flush(&lufa_usb_cdc_device); // Flush after sending a string
	}
#endif
}

void console_put_float_val(float value) {
#ifdef ENABLE_CONSOLE
    char buffer[32]; // Buffer for float string
    int int_part = (int)value;
    int frac_part = (int)((value - (float)int_part) * 100.0f); // Get 2 decimal places

    if (frac_part < 0) { // Ensure fractional part is positive
        frac_part = -frac_part;
    }
    // Handle negative numbers like -0.xx correctly
    if (value < 0.0f && int_part == 0) {
         snprintf_P(buffer, sizeof(buffer), PSTR("-%d.%02d"), int_part, frac_part);
    } else {
         snprintf_P(buffer, sizeof(buffer), PSTR("%d.%02d"), int_part, frac_part);
    }
    console_puts(buffer);
#endif
}

#include <stdio.h>
#include <stdint.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void process_line(const char* line) {
	char				command_name[CONSOLE_LINE_BUFFER_SIZE];
	const char* args = "";

	// Separate command from arguments
	const char* first_space = strchr(line, ' ');
	if (first_space != NULL) {
		size_t cmd_len = first_space - line;
		if (cmd_len < sizeof(command_name)) {
			strncpy(command_name, line, cmd_len);
			command_name[cmd_len] = '\0';
			args									= first_space + 1;
			while (*args == ' ') { // Skip leading whitespace in args
				args++;
			}
		} else {
			// Command too long, treat as unknown
			strncpy(command_name, line, sizeof(command_name) - 1);
			command_name[sizeof(command_name) - 1] = '\0';
		}
	} else {
		// No space, the whole line is the command
		strncpy(command_name, line, sizeof(command_name) - 1);
		command_name[sizeof(command_name) - 1] = '\0';
	}

	// Iterate through the command table
	for (uint8_t i = 0; i < num_commands; i++) {
		// Read command definition from PROGMEM
		// console_command_t current_command;
		// memcpy_P(&current_command, &commands[i], sizeof(console_command_t));

		// Read command name string directly from PROGMEM into RAM buffer for
		// comparison
		char command_name_pgm[CONSOLE_LINE_BUFFER_SIZE]; // Adjust size if needed
		strncpy_P(command_name_pgm, (const char*)pgm_read_ptr(&commands[i].name),
							sizeof(command_name_pgm) - 1);
		command_name_pgm[sizeof(command_name_pgm) - 1] = '\0';

		// Compare input command with command name from PROGMEM (case-insensitive)
		if (strcasecmp(command_name, command_name_pgm) == 0) {
			// Found a match, read the handler function pointer from PROGMEM
			command_handler_t handler_func =
					(command_handler_t)pgm_read_ptr(&commands[i].handler);

			// Call the handler function pointer if it's not NULL
			if (handler_func != NULL) {
				handler_func(args); // Pass arguments to handler
				return;							// Command processed, exit function
			}
		}
	}

	// Command not found
	console_puts_p(PSTR("Unknown command: "));
	console_puts(line);
	console_puts_p(PSTR("\r\n"));
	handle_help(NULL); // Show help on unknown command
}

// --- Command Handlers ---

static void handle_reset(const char* args __attribute__((unused))) {
	console_puts_p(PSTR("Resetting device...\r\n"));
	// Short delay to allow message to be sent before reset
	for (volatile uint32_t i = 0; i < 50000; ++i) {}
	hal_system_reset(); // This function does not return
}

static void handle_help(const char* args __attribute__((unused))) {
	console_puts_p(PSTR("Available commands:\r\n"));
	char buffer[CONSOLE_LINE_BUFFER_SIZE]; // Buffer for formatting output

	for (uint8_t i = 0; i < num_commands; i++) {
		// Read command name and help text directly from PROGMEM
		char command_name_pgm[20]; // Adjust size as needed
		char help_text_pgm[40];		 // Adjust size as needed
		strncpy_P(command_name_pgm, (const char*)pgm_read_ptr(&commands[i].name),
							sizeof(command_name_pgm) - 1);
		command_name_pgm[sizeof(command_name_pgm) - 1] = '\0';
		strncpy_P(help_text_pgm, (const char*)pgm_read_ptr(&commands[i].help_text),
							sizeof(help_text_pgm) - 1);
		help_text_pgm[sizeof(help_text_pgm) - 1] = '\0';

		// Format and print using snprintf (safer than sprintf)
		snprintf(buffer, sizeof(buffer), "  %-10s - %s\r\n", command_name_pgm,
						 help_text_pgm);
		console_puts(buffer);
	}
}

// New command handler for config reset
static void handle_config_reset(const char* args __attribute__((unused))) {
	console_puts_p(PSTR("Performing factory reset...\r\n"));
	struct sys_event evt = {.type = EVT_SYS_REQ_CFG_RESET, .data = NULL};
	event_post(EVENT_CHANNEL_SYS, &evt);
}

// New command handler for signature row display
static void handle_signature(const char* args __attribute__((unused))) {
	console_puts_p(PSTR("Reading production signature row...\r\n"));
	console_print_signature_row();
}

/**
 * @brief Prints the production signature row values to the console.
 *
 * This function reads all non-reserved fields from the production signature row
 * and prints their names and values to the console.
 */
void console_print_signature_row(void) {
	NVM_PROD_SIGNATURES_t sig_data;
	char									buffer[CONSOLE_LINE_BUFFER_SIZE];

	// Read the entire signature row
	signature_read(&sig_data);

	console_puts_p(PSTR("=== Production Signature Row ===\r\n"));

	// Define all format strings in PROGMEM
	static const char PROGMEM rcosc2m_fmt[]			 = "RCOSC2M:    0x%02X\r\n";
	static const char PROGMEM rcosc2ma_fmt[]		 = "RCOSC2MA:   0x%02X\r\n";
	static const char PROGMEM rcosc32k_fmt[]		 = "RCOSC32K:   0x%02X\r\n";
	static const char PROGMEM rcosc32m_fmt[]		 = "RCOSC32M:   0x%02X\r\n";
	static const char PROGMEM rcosc32ma_fmt[]		 = "RCOSC32MA:  0x%02X\r\n";
	static const char PROGMEM lot_number_fmt[]	 = "%c%c%c%c%c%c\r\n";
	static const char PROGMEM wafer_num_fmt[]		 = "Wafer Num:  %d\r\n";
	static const char PROGMEM coords_fmt[]			 = "Coords:     X=%u, Y=%u\r\n";
	static const char PROGMEM usbcal0_fmt[]			 = "USBCAL0:    0x%02X\r\n";
	static const char PROGMEM usbcal1_fmt[]			 = "USBCAL1:    0x%02X\r\n";
	static const char PROGMEM usbrcosc_fmt[]		 = "USBRCOSC:   0x%02X\r\n";
	static const char PROGMEM usbrcosca_fmt[]		 = "USBRCOSCA:  0x%02X\r\n";
	static const char PROGMEM adcacal0_fmt[]		 = "ADCACAL0:   0x%02X\r\n";
	static const char PROGMEM adcacal1_fmt[]		 = "ADCACAL1:   0x%02X\r\n";
	static const char PROGMEM adcbcal0_fmt[]		 = "ADCBCAL0:   0x%02X\r\n";
	static const char PROGMEM adcbcal1_fmt[]		 = "ADCBCAL1:   0x%02X\r\n";
	static const char PROGMEM tempsense0_fmt[]	 = "TEMPSENSE0:  0x%02X\r\n";
	static const char PROGMEM tempsense1_fmt[]	 = "TEMPSENSE1:  0x%02X\r\n";
	static const char PROGMEM daca0offcal_fmt[]	 = "DACA0OFFCAL:  0x%02X\r\n";
	static const char PROGMEM daca0gaincal_fmt[] = "DACA0GAINCAL: 0x%02X\r\n";
	static const char PROGMEM dacb0offcal_fmt[]	 = "DACB0OFFCAL:  0x%02X\r\n";
	static const char PROGMEM dacb0gaincal_fmt[] = "DACB0GAINCAL: 0x%02X\r\n";
	static const char PROGMEM daca1offcal_fmt[]	 = "DACA1OFFCAL:  0x%02X\r\n";
	static const char PROGMEM daca1gaincal_fmt[] = "DACA1GAINCAL: 0x%02X\r\n";
	static const char PROGMEM dacb1offcal_fmt[]	 = "DACB1OFFCAL:  0x%02X\r\n";
	static const char PROGMEM dacb1gaincal_fmt[] = "DACB1GAINCAL: 0x%02X\r\n";

	// Print oscillator calibration values
	snprintf_P(buffer, sizeof(buffer), rcosc2m_fmt, sig_data.RCOSC2M);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), rcosc2ma_fmt, sig_data.RCOSC2MA);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), rcosc32k_fmt, sig_data.RCOSC32K);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), rcosc32m_fmt, sig_data.RCOSC32M);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), rcosc32ma_fmt, sig_data.RCOSC32MA);
	console_puts(buffer);

	// Print lot number (as ASCII characters)
	console_puts_p(PSTR("Lot Number: "));
	snprintf_P(buffer, sizeof(buffer), lot_number_fmt, sig_data.LOTNUM0,
						 sig_data.LOTNUM1, sig_data.LOTNUM2, sig_data.LOTNUM3,
						 sig_data.LOTNUM4, sig_data.LOTNUM5);
	console_puts(buffer);

	// Print wafer information
	snprintf_P(buffer, sizeof(buffer), wafer_num_fmt, sig_data.WAFNUM);
	console_puts(buffer);

	// Print coordinates
	uint16_t coord_x = (sig_data.COORDX1 << 8) | sig_data.COORDX0;
	uint16_t coord_y = (sig_data.COORDY1 << 8) | sig_data.COORDY0;
	snprintf_P(buffer, sizeof(buffer), coords_fmt, coord_x, coord_y);
	console_puts(buffer);

	// Print USB calibration
	snprintf_P(buffer, sizeof(buffer), usbcal0_fmt, sig_data.USBCAL0);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), usbcal1_fmt, sig_data.USBCAL1);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), usbrcosc_fmt, sig_data.USBRCOSC);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), usbrcosca_fmt, sig_data.USBRCOSCA);
	console_puts(buffer);

	// Print ADC calibration values
	snprintf_P(buffer, sizeof(buffer), adcacal0_fmt, sig_data.ADCACAL0);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), adcacal1_fmt, sig_data.ADCACAL1);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), adcbcal0_fmt, sig_data.ADCBCAL0);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), adcbcal1_fmt, sig_data.ADCBCAL1);
	console_puts(buffer);

	// Print temperature sensor calibration
	snprintf_P(buffer, sizeof(buffer), tempsense0_fmt, sig_data.TEMPSENSE0);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), tempsense1_fmt, sig_data.TEMPSENSE1);
	console_puts(buffer);

	// Print DAC calibration values
	snprintf_P(buffer, sizeof(buffer), daca0offcal_fmt, sig_data.DACA0OFFCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), daca0gaincal_fmt, sig_data.DACA0GAINCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), dacb0offcal_fmt, sig_data.DACB0OFFCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), dacb0gaincal_fmt, sig_data.DACB0GAINCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), daca1offcal_fmt, sig_data.DACA1OFFCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), daca1gaincal_fmt, sig_data.DACA1GAINCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), dacb1offcal_fmt, sig_data.DACB1OFFCAL);
	console_puts(buffer);
	snprintf_P(buffer, sizeof(buffer), dacb1gaincal_fmt, sig_data.DACB1GAINCAL);
	console_puts(buffer);

	console_puts_p(PSTR("==============================\r\n"));
}

// System event handler for console
static int console_sys_event_handler(void* event) {
	assert(event);
	struct sys_event* e = (struct sys_event*)event;

	switch (e->type) {
		case EVT_SYS_RES_CFG_RESET:
			if (e->data.ret != SUCCESS) {
				console_puts_p(PSTR("Configuration reset failed.\r\n"));
			} else {
				console_puts_p(PSTR("Configuration reset.\r\n"));
			}
			needs_prompt = true; // Ensure prompt is reprinted after message
			break;

		default:
			// Ignore other system events
			break;
	}
	return 0; // Indicate event handled (or ignored)
}

/**
 * @brief Command handler for reading and displaying the internal temperature
 *
 * This function initializes the ADC with appropriate settings for temperature
 * reading, reads the internal temperature sensor, and displays the value in
 * degrees Celsius with decimal precision.
 *
 * @param args Command arguments (unused)
 */
static void handle_temperature(const char* args __attribute__((unused))) {
	char buffer[CONSOLE_LINE_BUFFER_SIZE];

	// Initialize ADC for temperature reading with 1V internal reference
	adc_init(ADC_REF_INT1V, ADC_RES_12BIT, ADC_PRESCALER_DIV256);

	// Read temperature from internal sensor with floating point precision
	float temperature = adc_read_temperature_float();

	// Format and display the temperature
	// Convert float to integer parts for printing (e.g., 23.5 -> 23 and 5)
	int16_t temp_int	= (int16_t)temperature;
	int16_t temp_frac = (int16_t)((temperature - temp_int) * 10);
	if (temp_frac < 0)
		temp_frac = -temp_frac; // Ensure fraction is positive

	console_puts_p(PSTR("Reading internal temperature...\r\n"));
	snprintf_P(buffer, sizeof(buffer), PSTR("Temperature: %d.%d°C\r\n"), temp_int,
						 temp_frac);
	console_puts(buffer);
}

/**
 * @brief Command handler for displaying the RNG seed value
 *
 * This function retrieves the current random number generator seed value
 * and displays it in hexadecimal format.
 *
 * @param args Command arguments (unused)
 */
static void handle_rng_seed(const char* args __attribute__((unused))) {
	char buffer[CONSOLE_LINE_BUFFER_SIZE];

	// Get the current RNG seed value
	uint32_t seed = rng_get_seed();

	// Display the seed value in both hexadecimal and decimal formats
	console_puts_p(PSTR("Random Number Generator Seed:\r\n"));
	snprintf_P(buffer, sizeof(buffer), PSTR("  0x%08lX (%lu)\r\n"), seed, seed);
	console_puts(buffer);
}

/**
 * @brief Command handler for setting HSV values for vmap
 *
 * This function sets the HSV values for a specified vmap index.
 *
 * @param args Command arguments in the format: <bank> <enc> <vmap_idx> <H
 * (0-1535)> <S (0-255)> <V (0-255)>
 */
static void handle_set_vmap_hsv(const char* args) {
	uint8_t	 bank, enc, vmap_idx;
	uint16_t h;
	uint8_t	 s, v;

	int parsed = sscanf(args, "%hhu %hhu %hhu %hu %hhu %hhu", &bank, &enc,
											&vmap_idx, &h, &s, &v);

	if (parsed != 6) {
		console_puts_p(PSTR("Usage: set_vmap_hsv <bank> <enc> <vmap_idx> <hue "
												"0-1535> <sat 0-255> <val 0-255>\r\n"));
		return;
	}

	// Validate input ranges
	if (bank >= NUM_ENC_BANKS || enc >= NUM_ENCODERS ||
			vmap_idx >= NUM_VMAPS_PER_ENC) {
		console_puts_p(PSTR("Invalid bank, encoder, or vmap index\r\n"));
		return;
	}

	// Set HSV values and update RGB
	color_set_vmap_hsv(bank, enc, vmap_idx, h, s, v);

	console_puts_p(PSTR("HSV color set\r\n"));
}
