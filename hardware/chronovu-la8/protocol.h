/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011-2012 Uwe Hermann <uwe@hermann-uwe.de>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef LIBSIGROK_HARDWARE_CHRONOVU_LA8_PROTOCOL_H
#define LIBSIGROK_HARDWARE_CHRONOVU_LA8_PROTOCOL_H

#include <glib.h>
#include <ftdi.h>
#include <stdint.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/* Message logging helpers with driver-specific prefix string. */
#define DRIVER_LOG_DOMAIN "la8: "
#define sr_log(l, s, args...) sr_log(l, DRIVER_LOG_DOMAIN s, ## args)
#define sr_spew(s, args...) sr_spew(DRIVER_LOG_DOMAIN s, ## args)
#define sr_dbg(s, args...) sr_dbg(DRIVER_LOG_DOMAIN s, ## args)
#define sr_info(s, args...) sr_info(DRIVER_LOG_DOMAIN s, ## args)
#define sr_warn(s, args...) sr_warn(DRIVER_LOG_DOMAIN s, ## args)
#define sr_err(s, args...) sr_err(DRIVER_LOG_DOMAIN s, ## args)

#define USB_VENDOR_ID			0x0403
#define USB_DESCRIPTION			"ChronoVu LA8"
#define USB_VENDOR_NAME			"ChronoVu"
#define USB_MODEL_NAME			"LA8"
#define USB_MODEL_VERSION		""

#define NUM_PROBES			8
#define TRIGGER_TYPE 			"01"
#define SDRAM_SIZE			(8 * 1024 * 1024)
#define MIN_NUM_SAMPLES			1

#define BS				4096 /* Block size */
#define NUM_BLOCKS			2048 /* Number of blocks */

/* Private, per-device-instance driver context. */
struct dev_context {
	/** FTDI device context (used by libftdi). */
	struct ftdi_context *ftdic;

	/** The currently configured samplerate of the device. */
	uint64_t cur_samplerate;

	/** The current sampling limit (in ms). */
	uint64_t limit_msec;

	/** The current sampling limit (in number of samples). */
	uint64_t limit_samples;

	void *cb_data;

	/**
	 * A buffer containing some (mangled) samples from the device.
	 * Format: Pretty mangled-up (due to hardware reasons), see code.
	 */
	uint8_t mangled_buf[BS];

	/**
	 * An 8MB buffer where we'll store the de-mangled samples.
	 * Format: Each sample is 1 byte, MSB is channel 7, LSB is channel 0.
	 */
	uint8_t *final_buf;

	/**
	 * Trigger pattern (MSB = channel 7, LSB = channel 0).
	 * A 1 bit matches a high signal, 0 matches a low signal on a probe.
	 * Only low/high triggers (but not e.g. rising/falling) are supported.
	 */
	uint8_t trigger_pattern;

	/**
	 * Trigger mask (MSB = channel 7, LSB = channel 0).
	 * A 1 bit means "must match trigger_pattern", 0 means "don't care".
	 */
	uint8_t trigger_mask;

	/** Time (in seconds) before the trigger times out. */
	uint64_t trigger_timeout;

	/** Tells us whether an SR_DF_TRIGGER packet was already sent. */
	int trigger_found;

	/** TODO */
	time_t done;

	/** Counter/index for the data block to be read. */
	int block_counter;

	/** The divcount value (determines the sample period) for the LA8. */
	uint8_t divcount;

	/** This ChronoVu LA8's USB PID (multiple versions exist). */
	uint16_t usb_pid;
};

/* protocol.c */
extern SR_PRIV uint64_t supported_samplerates[];
extern SR_PRIV const int hwcaps[];
extern SR_PRIV const char *probe_names[];
extern const struct sr_samplerates samplerates;
SR_PRIV void fill_supported_samplerates_if_needed(void);
SR_PRIV int is_valid_samplerate(uint64_t samplerate);
SR_PRIV uint8_t samplerate_to_divcount(uint64_t samplerate);
SR_PRIV int la8_write(struct dev_context *devc, uint8_t *buf, int size);
SR_PRIV int la8_read(struct dev_context *devc, uint8_t *buf, int size);
SR_PRIV int la8_close(struct dev_context *devc);
SR_PRIV int la8_close_usb_reset_sequencer(struct dev_context *devc);
SR_PRIV int la8_reset(struct dev_context *devc);
SR_PRIV int configure_probes(const struct sr_dev_inst *sdi);
SR_PRIV int set_samplerate(const struct sr_dev_inst *sdi, uint64_t samplerate);
SR_PRIV int la8_read_block(struct dev_context *devc);
SR_PRIV void send_block_to_session_bus(struct dev_context *devc, int block);

#endif
