/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "system/error.h"
#include "event/event.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	The channel table is file-scope state inside event.c with no teardown, so a
	channel cannot be un-registered once claimed. Each test therefore takes a
	channel of its own rather than sharing one.
*/

#define TEST_QUEUE_LEN (4)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct test_evt {
	u8 value;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8		seen[64];
static uint seen_count;
static char order[16];
static uint order_count;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int record_handler(void* event) {
	if (seen_count < COUNTOF(seen)) {
		seen[seen_count++] = ((struct test_evt*)event)->value;
	}
	return 0;
}

static int order_a(void* event) {
	(void)event;
	if (order_count < COUNTOF(order)) {
		order[order_count++] = 'a';
	}
	return 0;
}

static int order_b(void* event) {
	(void)event;
	if (order_count < COUNTOF(order)) {
		order[order_count++] = 'b';
	}
	return 0;
}

static int order_c(void* event) {
	(void)event;
	if (order_count < COUNTOF(order)) {
		order[order_count++] = 'c';
	}
	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(a_registered_channel_queues_and_delivers_in_order) {
	static u8										queue[TEST_QUEUE_LEN * sizeof(struct test_evt)];
	static struct event_channel ch = {
			.queue			= queue,
			.queue_size = TEST_QUEUE_LEN,
			.data_size	= sizeof(struct test_evt),
	};
	EVT_HANDLER(0, handler, record_handler);

	CHECK_EQ(event_channel_register(EVENT_CHANNEL_MIDI_IN, &ch), SUCCESS);
	CHECK_EQ(event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &handler), SUCCESS);

	seen_count = 0;
	for (u8 i = 0; i < TEST_QUEUE_LEN; ++i) {
		struct test_evt e = {.value = (u8)(0x10 + i)};
		CHECK_EQ(event_post(EVENT_CHANNEL_MIDI_IN, &e), SUCCESS);
	}

	CHECK_EQ(event_channel_process(EVENT_CHANNEL_MIDI_IN), SUCCESS);

	CHECK_EQ(seen_count, TEST_QUEUE_LEN);
	for (u8 i = 0; i < TEST_QUEUE_LEN; ++i) {
		CHECK_EQ(seen[i], 0x10 + i);
	}
}

TEST(the_queue_holds_exactly_its_stated_size) {
	static u8										queue[TEST_QUEUE_LEN * sizeof(struct test_evt)];
	static struct event_channel ch = {
			.queue			= queue,
			.queue_size = TEST_QUEUE_LEN,
			.data_size	= sizeof(struct test_evt),
	};
	EVT_HANDLER(0, handler, record_handler);

	CHECK_EQ(event_channel_register(EVENT_CHANNEL_MIDI_OUT, &ch), SUCCESS);
	CHECK_EQ(event_channel_subscribe(EVENT_CHANNEL_MIDI_OUT, &handler), SUCCESS);

	// The last usable slot must be accepted, not refused one short.
	struct test_evt e = {.value = 1};
	for (uint i = 0; i < TEST_QUEUE_LEN; ++i) {
		CHECK_EQ(event_post(EVENT_CHANNEL_MIDI_OUT, &e), SUCCESS);
	}

	// And only then is the queue full.
	CHECK_EQ(event_post(EVENT_CHANNEL_MIDI_OUT, &e), ERR_NO_MEM);

	// Draining it makes room again.
	seen_count = 0;
	CHECK_EQ(event_channel_process(EVENT_CHANNEL_MIDI_OUT), SUCCESS);
	CHECK_EQ(seen_count, TEST_QUEUE_LEN);
	CHECK_EQ(event_post(EVENT_CHANNEL_MIDI_OUT, &e), SUCCESS);
}

TEST(handlers_run_highest_priority_first) {
	static u8										queue[TEST_QUEUE_LEN * sizeof(struct test_evt)];
	static struct event_channel ch = {
			.queue			= queue,
			.queue_size = TEST_QUEUE_LEN,
			.data_size	= sizeof(struct test_evt),
	};
	EVT_HANDLER(5, high, order_a);
	EVT_HANDLER(3, mid, order_b);
	EVT_HANDLER(0, last, order_c);

	CHECK_EQ(event_channel_register(EVENT_CHANNEL_ANIMATION, &ch), SUCCESS);

	// Subscribed out of order on purpose.
	CHECK_EQ(event_channel_subscribe(EVENT_CHANNEL_ANIMATION, &mid), SUCCESS);
	CHECK_EQ(event_channel_subscribe(EVENT_CHANNEL_ANIMATION, &last), SUCCESS);
	CHECK_EQ(event_channel_subscribe(EVENT_CHANNEL_ANIMATION, &high), SUCCESS);

	struct test_evt e = {.value = 0};
	CHECK_EQ(event_post(EVENT_CHANNEL_ANIMATION, &e), SUCCESS);

	order_count = 0;
	CHECK_EQ(event_channel_process(EVENT_CHANNEL_ANIMATION), SUCCESS);

	order[order_count] = '\0';
	// Priority 0 means "run last", not "run first".
	CHECK_STR_EQ(order, "abc");
}

TEST(posting_to_an_unregistered_channel_is_refused) {
	struct test_evt e = {.value = 0};
	CHECK_EQ(event_post(EVENT_CHANNEL_SYS, &e), ERR_BAD_PARAM);
}

TEST(processing_an_unregistered_channel_is_harmless) {
	// event_update() walks every channel, including ones nothing registered.
	CHECK_EQ(event_channel_process(EVENT_CHANNEL_SYS), SUCCESS);
	CHECK_EQ(event_update(), SUCCESS);
}

TEST(a_channel_cannot_be_registered_twice) {
	static u8										queue[TEST_QUEUE_LEN * sizeof(struct test_evt)];
	static struct event_channel ch = {
			.queue			= queue,
			.queue_size = TEST_QUEUE_LEN,
			.data_size	= sizeof(struct test_evt),
	};

	CHECK_EQ(event_channel_register(EVENT_CHANNEL_MIDI_IN, &ch), ERR_DUPLICATE);
}

TEST(an_incomplete_channel_definition_is_refused) {
	static u8						 queue[8];
	struct event_channel no_queue = {
			.queue = NULL, .queue_size = 4, .data_size = 1};
	struct event_channel no_size = {
			.queue = queue, .queue_size = 0, .data_size = 1};
	struct event_channel no_data = {
			.queue = queue, .queue_size = 4, .data_size = 0};

	CHECK_EQ(event_channel_register(EVENT_CHANNEL_SYS, &no_queue), ERR_BAD_PARAM);
	CHECK_EQ(event_channel_register(EVENT_CHANNEL_SYS, &no_size), ERR_BAD_PARAM);
	CHECK_EQ(event_channel_register(EVENT_CHANNEL_SYS, &no_data), ERR_BAD_PARAM);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(a_registered_channel_queues_and_delivers_in_order);
						RUN(the_queue_holds_exactly_its_stated_size);
						RUN(handlers_run_highest_priority_first);
						RUN(posting_to_an_unregistered_channel_is_refused);
						RUN(processing_an_unregistered_channel_is_harmless);
						RUN(a_channel_cannot_be_registered_twice);
						RUN(an_incomplete_channel_definition_is_refused);)
