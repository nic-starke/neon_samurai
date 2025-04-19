/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "common/console/console.h"
#include "hal/avr/xmega/128a4u/sys.h"
#include "usb/usb.h"
#include "event/event.h" // Add event header
#include "event/sys.h"   // Add sys event header

#include <LUFA/Drivers/USB/Class/Device/CDCClassDevice.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define CONSOLE_LINE_BUFFER_SIZE 64
#define CONSOLE_PROMPT PSTR("> ")
// Helper macro to define commands in PROGMEM
#define DEFINE_COMMAND(name_str, handler_func, help_str) \
	(console_command_t){ .name = PSTR(name_str), .handler = handler_func, .help_text = PSTR(help_str) }


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef VSER_ENABLE
// Need the LUFA CDC device info structure
extern USB_ClassInfo_CDC_Device_t lufa_usb_cdc_device;
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Enum defining command identifiers (conceptual, not directly used for indexing here)
typedef enum {
	CMD_RESET,
	CMD_HELP,
	// Add new command IDs here
	CMD_COUNT // Keep this last for array sizing if needed elsewhere
} console_command_id_e;

// Function pointer type for command handlers
typedef void (*command_handler_t)(const char* args);

// Structure to define a console command
typedef struct {
	const char* name; // Command name (stored in PROGMEM)
	command_handler_t handler; // Function pointer to the handler (stored in PROGMEM)
	const char* help_text; // Help text for the command (stored in PROGMEM)
} console_command_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void process_line(const char* line);

static void handle_reset(const char* args);
static void handle_help(const char* args);
static void handle_config_reset(const char* args);

static int console_sys_event_handler(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static char line_buffer[CONSOLE_LINE_BUFFER_SIZE];
static uint8_t line_buffer_index = 0;
static bool needs_prompt = true;

// Command table stored in PROGMEM, initialized using the macro
// Define command strings in PROGMEM
static const char reset_command_name[] PROGMEM = "reset";
static const char reset_command_help[] PROGMEM = "Resets the device";

static const char help_command_name[] PROGMEM = "help";
static const char help_command_help[] PROGMEM = "Shows this help message";

static const char config_reset_command_name[] PROGMEM = "reset_cfg";
static const char config_reset_command_help[] PROGMEM = "Performs reset to factory defaults";

static const console_command_t commands[] PROGMEM = {
	{ .name = help_command_name,  .handler = handle_help,  .help_text = help_command_help },
	{ .name = reset_command_name, .handler = handle_reset, .help_text = reset_command_help },
	{ .name = config_reset_command_name, .handler = handle_config_reset, .help_text = config_reset_command_help },
};

static const uint8_t num_commands = sizeof(commands) / sizeof(commands[0]);

// Event handler structure for system events
static event_ch_handler_s console_sys_evt_handler_def = {
	.handler = &console_sys_event_handler,
	.next = NULL,
	.priority = 1,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void console_init(void) {
	line_buffer_index = 0;
	line_buffer[0] = '\0';
	needs_prompt = true;
	// Subscribe to system events
	event_channel_subscribe(EVENT_CHANNEL_SYS, &console_sys_evt_handler_def);

}

void console_update(void) {
#ifdef VSER_ENABLE
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
			handle_help(NULL); // Call help handler directly
			line_buffer_index = 0; // Reset buffer
			needs_prompt = true;   // Need a new prompt
		} else if (c == '\r' || c == '\n') {
			// End of line
			console_putc('\r'); // Echo CR
			console_putc('\n'); // Echo LF
			line_buffer[line_buffer_index] = '\0'; // Null-terminate
			if (line_buffer_index > 0) { // Only process if not empty
				process_line(line_buffer); // Use renamed function
			}
			line_buffer_index = 0; // Reset buffer
			needs_prompt = true;   // Need a new prompt
		} else if (isprint(c) && line_buffer_index < (CONSOLE_LINE_BUFFER_SIZE - 1)) {
			// Store printable characters
			line_buffer[line_buffer_index++] = c;
			console_putc(c); // Echo character
		}
		// Flush output buffer periodically or after specific actions
		CDC_Device_Flush(&lufa_usb_cdc_device);
	}
#endif // VSER_ENABLE
}

void console_putc(char c) {
#ifdef VSER_ENABLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendByte(&lufa_usb_cdc_device, c);
		// Consider flushing here or let console_update handle it
	}
#endif
}

void console_puts(const char* str) {
#ifdef VSER_ENABLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendString(&lufa_usb_cdc_device, str);
		CDC_Device_Flush(&lufa_usb_cdc_device); // Flush after sending a string
	}
#endif
}

void console_puts_p(const char* str_p) {
#ifdef VSER_ENABLE
	if (usb_cdc_is_active()) {
		CDC_Device_SendString_P(&lufa_usb_cdc_device, str_p);
		CDC_Device_Flush(&lufa_usb_cdc_device); // Flush after sending a string
	}
#endif
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void process_line(const char* line) {
	char command_name[CONSOLE_LINE_BUFFER_SIZE];
	const char* args = "";

	// Separate command from arguments
	const char* first_space = strchr(line, ' ');
	if (first_space != NULL) {
		size_t cmd_len = first_space - line;
		if (cmd_len < sizeof(command_name)) {
			strncpy(command_name, line, cmd_len);
			command_name[cmd_len] = '\0';
			args = first_space + 1;
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

		// Read command name string directly from PROGMEM into RAM buffer for comparison
		char command_name_pgm[CONSOLE_LINE_BUFFER_SIZE]; // Adjust size if needed
		strncpy_P(command_name_pgm, (const char*)pgm_read_ptr(&commands[i].name), sizeof(command_name_pgm) - 1);
		command_name_pgm[sizeof(command_name_pgm) - 1] = '\0';

		// Compare input command with command name from PROGMEM (case-insensitive)
		if (strcasecmp(command_name, command_name_pgm) == 0) {
			// Found a match, read the handler function pointer from PROGMEM
			command_handler_t handler_func = (command_handler_t)pgm_read_ptr(&commands[i].handler);

			// Call the handler function pointer if it's not NULL
			if (handler_func != NULL) {
				handler_func(args); // Pass arguments to handler
				return; // Command processed, exit function
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
		char help_text_pgm[40]; // Adjust size as needed
		strncpy_P(command_name_pgm, (const char*)pgm_read_ptr(&commands[i].name), sizeof(command_name_pgm) - 1);
		command_name_pgm[sizeof(command_name_pgm) - 1] = '\0';
		strncpy_P(help_text_pgm, (const char*)pgm_read_ptr(&commands[i].help_text), sizeof(help_text_pgm) - 1);
		help_text_pgm[sizeof(help_text_pgm) - 1] = '\0';

		// Format and print using snprintf (safer than sprintf)
		snprintf(buffer, sizeof(buffer), "  %-10s - %s\r\n", command_name_pgm, help_text_pgm);
		console_puts(buffer);
	}
}

// New command handler for config reset
static void handle_config_reset(const char* args __attribute__((unused))) {
	console_puts_p(PSTR("Performing factory reset...\r\n"));
	sys_event_s evt = { .type = EVT_SYS_REQ_CFG_RESET, .data = NULL };
	event_post(EVENT_CHANNEL_SYS, &evt);
}

// System event handler for console
static int console_sys_event_handler(void* event) {
	assert(event);
	sys_event_s* e = (sys_event_s*)event;

	switch (e->type) {
		case EVT_SYS_RES_CFG_RESET:
			if(e->data.ret != SUCCESS) {
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
