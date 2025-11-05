#include <stdlib.h>
#include <getopt.h>
#include <studiolink.h>

enum { ASYNC_WORKERS = 6, JITTER_INTERVAL = 10 };

static bool headless	    = false;
static uint64_t jitter_last = 0;
static uint64_t jitter_last_report = 0;
static int64_t max_jitter   = 0; /* Mainloop jitter */
static struct tmr tmr_jitter;


static const char *modv[] = {"turn", "ice", "dtls_srtp", "netroam",
			     /* video codecs */
			     "vp8",

			     /* audio codecs */
			     "opus", "g711",

			     /* audio filter */
			     "auconv", "auresamp",

			     /* audio drivers */
			     "portaudio"};


static void jitter_stats(void *arg)
{
	(void)arg;

	if (jitter_last) {
		int64_t jitter =
			(tmr_jiffies() - jitter_last) - JITTER_INTERVAL;
		if (jitter > max_jitter)
			max_jitter = jitter;
	}

	if ((tmr_jiffies() - jitter_last_report) > 1000) {
		RE_TRACE_INSTANT_I("slmain", "max_jitter", max_jitter);
		max_jitter = 0;
		jitter_last_report = tmr_jiffies();
	}

	jitter_last = tmr_jiffies();
	tmr_start(&tmr_jitter, JITTER_INTERVAL, jitter_stats, NULL);
}


static void signal_handler(int signum)
{
	re_fprintf(stderr, "terminated on signal %d\n", signum);

	re_cancel();
}


static void usage(void)
{
	(void)re_fprintf(stderr, "Usage: studiolink [options]\n"
				 "options:\n"
				 "\t-h               Help\n"
				 "\t-H --headless    Headless mode\n"
				 "\t-v               Verbose debug\n");
}


int sl_getopt(int argc, char *const argv[])
{
#ifdef HAVE_GETOPT
	int index		= 0;
	struct option options[] = {
		{"headless", 0, 0, 'H'}, {"help", 0, 0, 'h'}, {0, 0, 0, 0}};
	(void)re_printf(
		"   _____ __            ___         __    _       __\n"
		"  / ___// /___  ______/ (_)___    / /   (_)___  / /__\n"
		"  \\__ \\/ __/ / / / __  / / __ \\  / /   / / __ \\/ //_/\n"
		" ___/ / /_/ /_/ / /_/ / / /_/ / / /___/ / / / / ,<\n"
		"/____/\\__/\\__,_/\\__,_/_/\\____(_)_____/_/_/ /_/_/|_|"
		"\n");

	(void)re_printf("v%s"
			" Copyright (C) 2013 - 2025"
			" Sebastian Reimers\n\n",
			SL_VERSION);

	for (;;) {
		const int c = getopt_long(argc, argv, "hvH", options, &index);
		if (c < 0)
			break;

		switch (c) {

		case 'h':
			usage();
			return -2;
		case 'v':
			log_enable_debug(true);
			break;
		case 'H':
			headless = true;
			break;
		default:
			usage();
			return EINVAL;
		}
	}
#else
	(void)argc;
	(void)argv;
#endif

	return 0;
}


int sl_baresip_init(const uint8_t *conf)
{
	struct sl_config *slconf;
	const char *conf_ = "call_max_calls	16\n"
			    "sip_verify_server	yes\n"
			    "audio_buffer	20-100\n"
			    "audio_buffer_mode	fixed\n"
			    "audio_silence	-35.0\n"
			    "audio_jitter_buffer_type	adaptive\n"
			    "audio_jitter_buffer_ms	60-200\n"
			    "video_jitter_buffer_type	adaptive\n"
			    "video_jitter_buffer_ms	100-200\n"
			    "video_jitter_buffer_size 1000\n"
			    "audio_txmode	thread\n"
			    "opus_bitrate	64000\n"
			    "dns_getaddrinfo	yes\n"
			    "ice_policy		relay\n";
	int err;

	err = sl_conf_init();
	if (err) {
		warning("sl_conf_init: failed: %m\n", err);
		return err;
	}

	slconf = sl_conf();

	err = sl_conf_cacert();
	if (err) {
		warning("sl_conf_cacert: failed (%m)\n", err);
		return err;
	}

	/*
	 * turn off buffering on stdout
	 */
	setbuf(stdout, NULL);

	err = libre_init();
	if (err)
		return err;

#ifdef RE_TRACE_ENABLED
	err = re_trace_init("re_trace.json");
	if (err)
		return err;
#endif

	re_thread_async_init(ASYNC_WORKERS);

	(void)sys_coredump_set(true);

	if (!conf) {
		conf = (const uint8_t *)conf_;
	}

	err = conf_configure_buf(conf, str_len((char *)conf));
	if (err) {
		warning("sl_baresip_init: conf_configure failed: %m\n", err);
		goto out;
	}

	slconf->baresip->net.use_linklocal = false;

	re_snprintf(slconf->baresip->sip.cafile,
		    sizeof(slconf->baresip->sip.cafile),
		    "%s" DIR_SEP "cacert.pem", sl_conf_path());

	str_ncpy(slconf->baresip->sip.uuid, sl_conf_uuid(),
		 sizeof(slconf->baresip->sip.uuid));

	err = baresip_init(slconf->baresip);
	if (err) {
		warning("sl_baresip_init: baresip_init failed (%m)\n", err);
		goto out;
	}

	err = ua_init("StudioLink", true, true, true);
	if (err) {
		warning("sl_baresip_init: ua_init failed (%m)\n", err);
		goto out;
	}

	for (size_t i = 0; i < RE_ARRAY_SIZE(modv); i++) {

		err = module_load(".", modv[i]);
		if (err) {
			warning("could not pre-load module"
				" '%s' (%m)\n",
				modv[i], err);
		}
	}

out:
	if (err)
		sl_close();

	return err;
}


int sl_init(void)
{
	int err;

	err = sl_ws_init();
	if (err) {
		warning("sl_init: ws init failed (%m)\n", err);
		goto out;
	}

	err = sl_account_init();
	if (err) {
		warning("sl_init: account init failed (%m)\n", err);
		goto out;
	}

	err = sl_http_listen();
	if (err) {
		warning("sl_init: http_listen failed (%m)\n", err);
		goto out;
	}

	err = sl_audio_init();
	if (err) {
		warning("sl_init: audio init failed (%m)\n", err);
		goto out;
	}

	err = sl_tracks_init();
	if (err) {
		warning("sl_init: tracks init failed (%m)\n", err);
		goto out;
	}

	sl_meter_init();

	tmr_init(&tmr_jitter);
	tmr_start(&tmr_jitter, JITTER_INTERVAL, jitter_stats, NULL);

out:
	if (err)
		sl_close();

	return err;
}


/* blocking webui */
static int webui_open(void *arg)
{
	(void)arg;

	/* @TODO: add google-chrome and xdg-open fallback */
	/* @TODO: use permanent browser dir */
	return system("chromium --app=http://127.0.0.1:9999 "
		      "--user-data-dir=/tmp/.studio-link/browser "
		      "--window-size=1060,800 >/dev/null 2>&1");
}


static void webui_closed(int err, void *arg)
{
	(void)arg;

	if (err)
		warning("webui/browser open failed! %m\n", err);
	else
		info("webui/browser closed\n");

	re_cancel();
}


int sl_open_webui(void)
{
	if (headless)
		return 0;

	/* will block one async thread until closed */
	return re_thread_async(webui_open, webui_closed, NULL);
}


int sl_main(void)
{
	int err;

	err = re_main(signal_handler);

	return err;
}


void sl_close(void)
{
	sl_ws_close();
	sl_http_close();

	sl_tracks_close();
	sl_account_close();

	ua_stop_all(true);
	ua_close();
	sl_audio_close();
	sl_meter_close();
	module_app_unload();
	conf_close();

	baresip_close();
	mod_close();

	re_thread_async_close();
#ifdef RE_TRACE_ENABLED
	re_trace_close();
#endif

	/* Check for open timers */
	tmr_debug();

	libre_close();

	/* Check for memory leaks */
	mem_debug();
}
