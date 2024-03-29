#include <studiolink.h>

#include <stdlib.h>

#include "index.html.h"
#include "index.js.h"
#include "vendor.js.h"
#include "index.css.h"
#include "roboto-mono-latin-400.woff2.h"
#include "roboto-mono-latin-500.woff2.h"
#include "roboto-mono-latin-600.woff2.h"
#include "roboto-mono-latin-700.woff2.h"
#include "logo_standalone.svg.h"
#include "logo_solo.svg.h"

static struct http_sock *server_sock = NULL;


static void http_sreply(struct http_conn *conn, uint16_t scode,
			const char *reason, const char *ctype, const char *fmt,
			size_t size)
{
	struct mbuf *mb;
	int err = 0;
	(void)scode;
	(void)reason;

	mb = mbuf_alloc(size);
	if (!mb) {
		warning("http_sreply: ENOMEM\n");
		return;
	}

	err = mbuf_write_mem(mb, (const uint8_t *)fmt, size);
	if (err) {
		warning("http_sreply: mbuf_write_mem err %m\n", err);
		goto out;
	}

	err = http_reply(
		conn, 200, "OK",
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Referrer-Policy: no-referrer\r\n"

		"Content-Security-Policy: default-src 'self' ws:; "
		"frame-ancestors 'self'; form-action 'self'; img-src * data:;"
		"style-src 'self' 'unsafe-inline';\r\n"
#ifndef RELEASE
		/* Only allow CORS on DEV Builds
		 * @TODO add release test */
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: *\r\n"
#endif
		"\r\n"
		"%b",
		ctype, mb->end, mb->buf, mb->end);
	if (err)
		warning("http_sreply: http_reply err %m\n", err);
out:
	mem_deref(mb);
}


static void http_req_handler(struct http_conn *conn,
			     const struct http_msg *msg, void *arg)
{
	char *json_str;
	int id;
	struct pl pl;
	struct sl_track *track;
	int err = 0;
	(void)arg;


	if (!conn || !msg)
		return;


	/*
	 * Static Requests
	 */
	if (0 == pl_strcasecmp(&msg->path, "/")) {
		http_sreply(conn, 200, "OK", "text/html",
			    (const char *)dist_index_html,
			    dist_index_html_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/index.js")) {
		http_sreply(conn, 200, "OK", "application/javascript",
			    (const char *)dist_index_js, dist_index_js_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/vendor.js")) {
		http_sreply(conn, 200, "OK", "application/javascript",
			    (const char *)dist_vendor_js, dist_vendor_js_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/index.css")) {
		http_sreply(conn, 200, "OK", "text/css",
			    (const char *)dist_index_css, dist_index_css_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/roboto-mono-latin-400.woff2")) {
		http_sreply(conn, 200, "OK", "font/woff2",
			    (const char *)dist_roboto_mono_latin_400_woff2,
			    dist_roboto_mono_latin_400_woff2_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/roboto-mono-latin-500.woff2")) {
		http_sreply(conn, 200, "OK", "font/woff2",
			    (const char *)dist_roboto_mono_latin_500_woff2,
			    dist_roboto_mono_latin_500_woff2_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/roboto-mono-latin-600.woff2")) {
		http_sreply(conn, 200, "OK", "font/woff2",
			    (const char *)dist_roboto_mono_latin_600_woff2,
			    dist_roboto_mono_latin_600_woff2_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/roboto-mono-latin-700.woff2")) {
		http_sreply(conn, 200, "OK", "font/woff2",
			    (const char *)dist_roboto_mono_latin_700_woff2,
			    dist_roboto_mono_latin_700_woff2_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/logo_standalone.svg")) {
		http_sreply(conn, 200, "OK", "image/svg+xml",
			    (const char *)dist_logo_standalone_svg,
			    dist_logo_standalone_svg_len);
		return;
	}
	if (0 == pl_strcasecmp(&msg->path, "/logo_solo.svg")) {
		http_sreply(conn, 200, "OK", "image/svg+xml",
			    (const char *)dist_logo_solo_svg,
			    dist_logo_solo_svg_len);
		return;
	}


	json_str = mem_zalloc(SL_MAX_JSON + 1, NULL);
	if (!json_str) {
		http_ereply(conn, 500, "Not enough RAM");
		return;
	}


	/*
	 * Websocket Requests
	 */
	if (0 == pl_strcasecmp(&msg->path, "/ws/v1/tracks")) {
		sl_ws_open(conn, WS_TRACKS, msg, sl_ws_dummyh);

		re_snprintf(json_str, SL_MAX_JSON, "%H", sl_tracks_json, NULL);
		sl_ws_send_str(WS_TRACKS, json_str);
		goto out;
	}

	if (0 == pl_strcasecmp(&msg->path, "/ws/v1/meters")) {
		sl_ws_open(conn, WS_METERS, msg, sl_ws_dummyh);
		goto out;
	}


	/*
	 * API Requests
	 */
	if (0 == pl_strcasecmp(&msg->path, "/api/v1/tracks/remote") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		sl_track_add(&track, SL_TRACK_REMOTE);
		re_snprintf(json_str, SL_MAX_JSON, "%H", sl_tracks_json, NULL);
		sl_ws_send_str(WS_TRACKS, json_str);

		http_sreply(conn, 200, "OK", "text/html", "", 0);
		goto out;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/dial") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		struct pl pltrack = PL_INIT;
		struct pl peer;

		pl_set_mbuf(&peer, msg->mb);

		err = re_regex(msg->prm.p, msg->prm.l, "track=[0-9]+",
			       &pltrack);
		if (err)
			goto err;

		info("dial %r on track %d\n", &peer, pl_i32(&pltrack));

		err = sl_track_dial(sl_track_by_id(pl_i32(&pltrack)), &peer);
		if (err)
			goto err;

		sl_track_ws_send();

		http_sreply(conn, 200, "OK", "text/html", "", 0);

		goto out;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/accept") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		struct pl pltrack = PL_INIT;

		err = re_regex(msg->prm.p, msg->prm.l, "track=[0-9]+",
			       &pltrack);
		if (err)
			goto err;

		sl_track_accept(sl_track_by_id(pl_i32(&pltrack)));

		sl_track_ws_send();

		http_sreply(conn, 200, "OK", "text/html", "", 0);

		goto out;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/hangup") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		struct pl pltrack = PL_INIT;

		err = re_regex(msg->prm.p, msg->prm.l, "track=[0-9]+",
			       &pltrack);
		if (err)
			goto err;

		sl_track_hangup(sl_track_by_id(pl_i32(&pltrack)));

		sl_track_ws_send();

		http_sreply(conn, 200, "OK", "text/html", "", 0);

		goto out;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/audio/device") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {
		struct pl pltrack = PL_INIT;
		struct pl pl_mic, pl_speaker;
		struct slaudio *audio;
		uint32_t track_id;
		int mic_id, speaker_id;
		pl_set_mbuf(&pl, msg->mb);

		err = re_regex(pl.p, pl.l, "[0-9]+;[0-9]+", &pl_mic,
			       &pl_speaker);
		if (err)
			goto err;

		mic_id	   = pl_i32(&pl_mic);
		speaker_id = pl_i32(&pl_speaker);


		err = re_regex(msg->prm.p, msg->prm.l, "track=[0-9]+",
			       &pltrack);
		if (err)
			goto err;

		track_id = pl_u32(&pltrack);

		track = sl_track_by_id(track_id);
		audio = sl_track_audio(track);

		err = sl_audio_set_device(audio, speaker_id, mic_id);
		if (err) {
			http_ereply(conn, 404, "Not found");
			goto out;
		}

		http_sreply(conn, 200, "OK", "text/html", "", 0);
		goto out;
	}


	if (0 == pl_strcasecmp(&msg->path, "/api/v1/tracks") &&
	    0 == pl_strcasecmp(&msg->met, "DELETE")) {
		pl_set_mbuf(&pl, msg->mb);
		id = pl_i32(&pl);

		err = sl_track_del(id);
		if (err) {
			http_ereply(conn, 404, "Not found");
			goto out;
		}

		re_snprintf(json_str, SL_MAX_JSON, "%H", sl_tracks_json, NULL);
		sl_ws_send_str(WS_TRACKS, json_str);
		http_sreply(conn, 200, "OK", "text/html", "", 0);
		goto out;
	}

#ifndef RELEASE
	/* Default return OPTIONS - needed on dev for preflight CORS Check
	 * @TODO: add release test */
	if (0 == pl_strcasecmp(&msg->met, "OPTIONS")) {
		http_sreply(conn, 200, "OK", "text/html", "", 0);
	}
#endif

	/* Default 404 return */
	http_ereply(conn, 404, "Not found");

out:
	mem_deref(json_str);

	return;

err:
	mem_deref(json_str);
	http_ereply(conn, 500, "Error");
}


int sl_http_listen(void)
{
	int err;
	struct sa srv;

#ifdef RELEASE
	err = sa_set_str(&srv, "127.0.0.1", 9999);
#else
	err = sa_set_str(&srv, "0.0.0.0", 9999);
#endif
	if (err)
		return err;

	info("listen webui: http://%J\n", &srv);
	err = http_listen(&server_sock, &srv, http_req_handler, NULL);

	return err;
}


void sl_http_close(void)
{
	server_sock = mem_deref(server_sock);
}
