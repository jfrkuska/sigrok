/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2010-2012 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/* Message logging helpers with driver-specific prefix string. */
#define DRIVER_LOG_DOMAIN "session: "
#define sr_log(l, s, args...) sr_log(l, DRIVER_LOG_DOMAIN s, ## args)
#define sr_spew(s, args...) sr_spew(DRIVER_LOG_DOMAIN s, ## args)
#define sr_dbg(s, args...) sr_dbg(DRIVER_LOG_DOMAIN s, ## args)
#define sr_info(s, args...) sr_info(DRIVER_LOG_DOMAIN s, ## args)
#define sr_warn(s, args...) sr_warn(DRIVER_LOG_DOMAIN s, ## args)
#define sr_err(s, args...) sr_err(DRIVER_LOG_DOMAIN s, ## args)

/**
 * @file
 *
 * Creating, using, or destroying libsigrok sessions.
 */

/**
 * @defgroup grp_session Session handling
 *
 * Creating, using, or destroying libsigrok sessions.
 *
 * @{
 */

struct source {
	int timeout;
	sr_receive_data_callback_t cb;
	void *cb_data;

	/* This is used to keep track of the object (fd, pollfd or channel) which is
	 * being polled and will be used to match the source when removing it again.
	 */
	gintptr poll_object;
};

/* There can only be one session at a time. */
/* 'session' is not static, it's used elsewhere (via 'extern'). */
struct sr_session *session;

/**
 * Create a new session.
 *
 * @todo Should it use the file-global "session" variable or take an argument?
 *       The same question applies to all the other session functions.
 *
 * @return A pointer to the newly allocated session, or NULL upon errors.
 */
SR_API struct sr_session *sr_session_new(void)
{
	if (!(session = g_try_malloc0(sizeof(struct sr_session)))) {
		sr_err("Session malloc failed.");
		return NULL;
	}

	session->source_timeout = -1;

	return session;
}

/**
 * Destroy the current session.
 *
 * This frees up all memory used by the session.
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_destroy(void)
{
	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	sr_session_dev_remove_all();

	/* TODO: Error checks needed? */

	g_free(session);
	session = NULL;

	return SR_OK;
}

/**
 * Close a device instance.
 *
 * @param sdi The device instance to close. Must not be NULL. Also,
 *            sdi->driver, sdi->driver->priv, and sdi->priv must not be NULL.
 */
static void sr_dev_close(struct sr_dev_inst *sdi)
{
	int ret;

	if (!sdi) {
		sr_err("Invalid device instance, can't close device.");
		return;
	}

	/* In the drivers sdi->priv is a 'struct dev_context *devc'. */
	if (!sdi->priv) {
		/*
		 * Should be sr_err() in theory, but the 'demo' driver has
		 * NULL for sdi->priv, so we use sr_dbg() until that's fixed.
		 */
		sr_dbg("Invalid device context, can't close device.");
		return;
	}

	if (!sdi->driver) {
		sr_err("Invalid driver, can't close device.");
		return;
	}

	if (!sdi->driver->priv) {
		sr_err("Driver not initialized, can't close device.");
		return;
	}

	sr_spew("Closing '%s' device instance %d.", sdi->driver->name,
		sdi->index);

	if ((ret = sdi->driver->dev_close(sdi)) < 0)
		sr_err("Failed to close device instance: %d.", ret);
}

/**
 * Remove all the devices from the current session.
 *
 * The session itself (i.e., the struct sr_session) is not free'd and still
 * exists after this function returns.
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_dev_remove_all(void)
{
	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	g_slist_free_full(session->devs, (GDestroyNotify)sr_dev_close);
	session->devs = NULL;

	return SR_OK;
}

/**
 * Add a device instance to the current session.
 *
 * @param sdi The device instance to add to the current session. Must not
 *            be NULL. Also, sdi->driver and sdi->driver->dev_open must
 *            not be NULL.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments.
 */
SR_API int sr_session_dev_add(const struct sr_dev_inst *sdi)
{
	int ret;

	if (!sdi) {
		sr_err("%s: sdi was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	/* If sdi->driver is NULL, this is a virtual device. */
	if (!sdi->driver) {
		sr_dbg("%s: sdi->driver was NULL, this seems to be "
		       "a virtual device; continuing", __func__);
		/* Just add the device, don't run dev_open(). */
		session->devs = g_slist_append(session->devs, (gpointer)sdi);
		return SR_OK;
	}

	/* sdi->driver is non-NULL (i.e. we have a real device). */
	if (!sdi->driver->dev_open) {
		sr_err("%s: sdi->driver->dev_open was NULL", __func__);
		return SR_ERR_BUG;
	}

	if ((ret = sdi->driver->dev_open((struct sr_dev_inst *)sdi)) != SR_OK) {
		sr_err("%s: dev_open failed (%d)", __func__, ret);
		return ret;
	}

	session->devs = g_slist_append(session->devs, (gpointer)sdi);

	return SR_OK;
}

/**
 * Remove all datafeed callbacks in the current session.
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_datafeed_callback_remove_all(void)
{
	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	g_slist_free(session->datafeed_callbacks);
	session->datafeed_callbacks = NULL;

	return SR_OK;
}

/**
 * Add a datafeed callback to the current session.
 *
 * @param cb Function to call when a chunk of data is received.
 *           Must not be NULL.
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_datafeed_callback_add(sr_datafeed_callback_t cb)
{
	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	if (!cb) {
		sr_err("%s: cb was NULL", __func__);
		return SR_ERR_ARG;
	}

	session->datafeed_callbacks =
	    g_slist_append(session->datafeed_callbacks, cb);

	return SR_OK;
}

static int sr_session_run_poll(void)
{
	unsigned int i;
	int ret;

	while (session->num_sources > 0) {
		ret = g_poll(session->pollfds, session->num_sources,
				session->source_timeout);
		for (i = 0; i < session->num_sources; i++) {
			if (session->pollfds[i].revents > 0 || (ret == 0
				&& session->source_timeout == session->sources[i].timeout)) {
				/*
				 * Invoke the source's callback on an event,
				 * or if the poll timed out and this source
				 * asked for that timeout.
				 */
				if (!session->sources[i].cb(session->pollfds[i].fd,
						session->pollfds[i].revents,
						session->sources[i].cb_data))
					sr_session_source_remove(session->sources[i].poll_object);
			}
		}
	}

	return SR_OK;
}

/**
 * Start a session.
 *
 * There can only be one session at a time.
 *
 * @return SR_OK upon success, SR_ERR upon errors.
 */
SR_API int sr_session_start(void)
{
	struct sr_dev_inst *sdi;
	GSList *l;
	int ret;

	if (!session) {
		sr_err("%s: session was NULL; a session must be "
		       "created before starting it.", __func__);
		return SR_ERR_BUG;
	}

	if (!session->devs) {
		sr_err("%s: session->devs was NULL; a session "
		       "cannot be started without devices.", __func__);
		return SR_ERR_BUG;
	}

	sr_info("Starting.");

	ret = SR_OK;
	for (l = session->devs; l; l = l->next) {
		sdi = l->data;
		if ((ret = sdi->driver->dev_acquisition_start(sdi, sdi)) != SR_OK) {
			sr_err("%s: could not start an acquisition "
			       "(%d)", __func__, ret);
			break;
		}
	}

	/* TODO: What if there are multiple devices? Which return code? */

	return ret;
}

/**
 * Run the session.
 *
 * @return SR_OK upon success, SR_ERR_BUG upon errors.
 */
SR_API int sr_session_run(void)
{
	if (!session) {
		sr_err("%s: session was NULL; a session must be "
		       "created first, before running it.", __func__);
		return SR_ERR_BUG;
	}

	if (!session->devs) {
		/* TODO: Actually the case? */
		sr_err("%s: session->devs was NULL; a session "
		       "cannot be run without devices.", __func__);
		return SR_ERR_BUG;
	}

	sr_info("Running.");

	/* Do we have real sources? */
	if (session->num_sources == 1 && session->pollfds[0].fd == -1) {
		/* Dummy source, freewheel over it. */
		while (session->num_sources)
			session->sources[0].cb(-1, 0, session->sources[0].cb_data);
	} else {
		/* Real sources, use g_poll() main loop. */
		sr_session_run_poll();
	}

	return SR_OK;
}

/**
 * Halt the current session.
 *
 * This function is deprecated and should not be used in new code, use
 * sr_session_stop() instead. The behaviour of this function is identical to
 * sr_session_stop().
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_halt(void)
{
	return sr_session_stop();
}

/**
 * Stop the current session.
 *
 * The current session is stopped immediately, with all acquisition sessions
 * being stopped and hardware drivers cleaned up.
 *
 * @return SR_OK upon success, SR_ERR_BUG if no session exists.
 */
SR_API int sr_session_stop(void)
{
	struct sr_dev_inst *sdi;
	GSList *l;

	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_BUG;
	}

	sr_info("Stopping.");

	for (l = session->devs; l; l = l->next) {
		sdi = l->data;
		if (sdi->driver) {
			if (sdi->driver->dev_acquisition_stop)
				sdi->driver->dev_acquisition_stop(sdi, sdi);
		}
	}

	/*
	 * Some sources may not be necessarily associated with a device.
	 * Those sources may still be present even after stopping all devices.
	 * We need to make sure all sources are removed, or we risk running the
	 * session in an infinite loop.
	 */
	while (session->num_sources)
		sr_session_source_remove(session->sources[0].poll_object);

	return SR_OK;
}

/**
 * Debug helper.
 *
 * @param packet The packet to show debugging information for.
 */
static void datafeed_dump(const struct sr_datafeed_packet *packet)
{
	const struct sr_datafeed_logic *logic;
	const struct sr_datafeed_analog *analog;

	switch (packet->type) {
	case SR_DF_HEADER:
		sr_dbg("bus: Received SR_DF_HEADER packet.");
		break;
	case SR_DF_TRIGGER:
		sr_dbg("bus: Received SR_DF_TRIGGER packet.");
		break;
	case SR_DF_META:
		sr_dbg("bus: Received SR_DF_META packet.");
		break;
	case SR_DF_LOGIC:
		logic = packet->payload;
		sr_dbg("bus: Received SR_DF_LOGIC packet (%" PRIu64 " bytes).",
		       logic->length);
		break;
	case SR_DF_ANALOG:
		analog = packet->payload;
		sr_dbg("bus: Received SR_DF_ANALOG packet (%d samples).",
		       analog->num_samples);
		break;
	case SR_DF_END:
		sr_dbg("bus: Received SR_DF_END packet.");
		break;
	case SR_DF_FRAME_BEGIN:
		sr_dbg("bus: Received SR_DF_FRAME_BEGIN packet.");
		break;
	case SR_DF_FRAME_END:
		sr_dbg("bus: Received SR_DF_FRAME_END packet.");
		break;
	default:
		sr_dbg("bus: Received unknown packet type: %d.", packet->type);
		break;
	}
}

/**
 * Send a packet to whatever is listening on the datafeed bus.
 *
 * Hardware drivers use this to send a data packet to the frontend.
 *
 * @param sdi TODO.
 * @param packet The datafeed packet to send to the session bus.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments.
 *
 * @private
 */
SR_PRIV int sr_session_send(const struct sr_dev_inst *sdi,
			    const struct sr_datafeed_packet *packet)
{
	GSList *l;
	sr_datafeed_callback_t cb;

	if (!sdi) {
		sr_err("%s: sdi was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!packet) {
		sr_err("%s: packet was NULL", __func__);
		return SR_ERR_ARG;
	}

	for (l = session->datafeed_callbacks; l; l = l->next) {
		if (sr_log_loglevel_get() >= SR_LOG_DBG)
			datafeed_dump(packet);
		cb = l->data;
		cb(sdi, packet);
	}

	return SR_OK;
}

/**
 * Add an event source for a file descriptor.
 *
 * @param pollfd The GPollFD.
 * @param timeout Max time to wait before the callback is called, ignored if 0.
 * @param cb Callback function to add. Must not be NULL.
 * @param cb_data Data for the callback function. Can be NULL.
 * @param poll_object TODO.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors.
 */
static int _sr_session_source_add(GPollFD *pollfd, int timeout,
	sr_receive_data_callback_t cb, void *cb_data, gintptr poll_object)
{
	struct source *new_sources, *s;
	GPollFD *new_pollfds;

	if (!cb) {
		sr_err("%s: cb was NULL", __func__);
		return SR_ERR_ARG;
	}

	/* Note: cb_data can be NULL, that's not a bug. */

	new_pollfds = g_try_realloc(session->pollfds,
			sizeof(GPollFD) * (session->num_sources + 1));
	if (!new_pollfds) {
		sr_err("%s: new_pollfds malloc failed", __func__);
		return SR_ERR_MALLOC;
	}

	new_sources = g_try_realloc(session->sources, sizeof(struct source) *
			(session->num_sources + 1));
	if (!new_sources) {
		sr_err("%s: new_sources malloc failed", __func__);
		return SR_ERR_MALLOC;
	}

	new_pollfds[session->num_sources] = *pollfd;
	s = &new_sources[session->num_sources++];
	s->timeout = timeout;
	s->cb = cb;
	s->cb_data = cb_data;
	s->poll_object = poll_object;
	session->pollfds = new_pollfds;
	session->sources = new_sources;

	if (timeout != session->source_timeout && timeout > 0
	    && (session->source_timeout == -1 || timeout < session->source_timeout))
		session->source_timeout = timeout;

	return SR_OK;
}

/**
 * Add an event source for a file descriptor.
 *
 * @param fd The file descriptor.
 * @param events Events to check for.
 * @param timeout Max time to wait before the callback is called, ignored if 0.
 * @param cb Callback function to add. Must not be NULL.
 * @param cb_data Data for the callback function. Can be NULL.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors.
 */
SR_API int sr_session_source_add(int fd, int events, int timeout,
		sr_receive_data_callback_t cb, void *cb_data)
{
	GPollFD p;

	p.fd = fd;
	p.events = events;

	return _sr_session_source_add(&p, timeout, cb, cb_data, (gintptr)fd);
}

/**
 * Add an event source for a GPollFD.
 *
 * @param pollfd The GPollFD.
 * @param timeout Max time to wait before the callback is called, ignored if 0.
 * @param cb Callback function to add. Must not be NULL.
 * @param cb_data Data for the callback function. Can be NULL.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors.
 */
SR_API int sr_session_source_add_pollfd(GPollFD *pollfd, int timeout,
		sr_receive_data_callback_t cb, void *cb_data)
{
	return _sr_session_source_add(pollfd, timeout, cb,
				      cb_data, (gintptr)pollfd);
}

/**
 * Add an event source for a GIOChannel.
 *
 * @param channel The GIOChannel.
 * @param events Events to poll on.
 * @param timeout Max time to wait before the callback is called, ignored if 0.
 * @param cb Callback function to add. Must not be NULL.
 * @param cb_data Data for the callback function. Can be NULL.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors.
 */
SR_API int sr_session_source_add_channel(GIOChannel *channel, int events,
		int timeout, sr_receive_data_callback_t cb, void *cb_data)
{
	GPollFD p;

#ifdef _WIN32
	g_io_channel_win32_make_pollfd(channel, events, &p);
#else
	p.fd = g_io_channel_unix_get_fd(channel);
	p.events = events;
#endif

	return _sr_session_source_add(&p, timeout, cb, cb_data, (gintptr)channel);
}

/**
 * Remove the source belonging to the specified channel.
 *
 * @todo Add more error checks and logging.
 *
 * @param channel The channel for which the source should be removed.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors, SR_ERR_BUG upon
 *         internal errors.
 */
static int _sr_session_source_remove(gintptr poll_object)
{
	struct source *new_sources;
	GPollFD *new_pollfds;
	unsigned int old;

	if (!session->sources || !session->num_sources) {
		sr_err("%s: sources was NULL", __func__);
		return SR_ERR_BUG;
	}

	for (old = 0; old < session->num_sources; old++) {
		if (session->sources[old].poll_object == poll_object)
			break;
	}

	/* fd not found, nothing to do */
	if (old == session->num_sources)
		return SR_OK;

	session->num_sources -= 1;

	if (old != session->num_sources) {
		memmove(&session->pollfds[old], &session->pollfds[old+1],
			(session->num_sources - old) * sizeof(GPollFD));
		memmove(&session->sources[old], &session->sources[old+1],
			(session->num_sources - old) * sizeof(struct source));
	}

	new_pollfds = g_try_realloc(session->pollfds, sizeof(GPollFD) * session->num_sources);
	if (!new_pollfds && session->num_sources > 0) {
		sr_err("%s: new_pollfds malloc failed", __func__);
		return SR_ERR_MALLOC;
	}

	new_sources = g_try_realloc(session->sources, sizeof(struct source) * session->num_sources);
	if (!new_sources && session->num_sources > 0) {
		sr_err("%s: new_sources malloc failed", __func__);
		return SR_ERR_MALLOC;
	}

	session->pollfds = new_pollfds;
	session->sources = new_sources;

	return SR_OK;
}

/**
 * Remove the source belonging to the specified file descriptor.
 *
 * @param fd The file descriptor for which the source should be removed.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors, SR_ERR_BUG upon
 *         internal errors.
 */
SR_API int sr_session_source_remove(int fd)
{
	return _sr_session_source_remove((gintptr)fd);
}

/**
 * Remove the source belonging to the specified poll descriptor.
 *
 * @param pollfd The poll descriptor for which the source should be removed.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors, SR_ERR_BUG upon
 *         internal errors.
 */
SR_API int sr_session_source_remove_pollfd(GPollFD *pollfd)
{
	return _sr_session_source_remove((gintptr)pollfd);
}

/**
 * Remove the source belonging to the specified channel.
 *
 * @param channel The channel for which the source should be removed.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments, or
 *         SR_ERR_MALLOC upon memory allocation errors, SR_ERR_BUG upon
 *         internal errors.
 */
SR_API int sr_session_source_remove_channel(GIOChannel *channel)
{
	return _sr_session_source_remove((gintptr)channel);
}

/** @} */
