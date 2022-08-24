#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

#include <re.h>
#include <baresip.h>
#include <studiolink.h>


enum { ASYNC_WORKERS = 4 };

static struct http_sock *httpsock = NULL;
static pthread_t open;
static bool headless = false;

static const char *modv[] = {
	"turn",
	"ice",
	"dtls_srtp",

	/* audio */
	"opus",
	"g711",
};

static void signal_handler(int signum)
{
	(void)signum;

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
			" Copyright (C) 2013 - 2022"
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


int sl_init(const uint8_t *conf)
{
	struct config *config;
	const char *conf_ = "opus_bitrate       64000\n";
	int err;

	/*
	 * turn off buffering on stdout
	 */
	setbuf(stdout, NULL);

	err = libre_init();
	if (err)
		return err;

	re_thread_async_init(ASYNC_WORKERS);

	(void)sys_coredump_set(true);

	if (!conf) {
		conf = (const uint8_t *)conf_;
	}

	err = conf_configure_buf(conf, str_len((char *)conf));
	if (err) {
		warning("sl_init: conf_configure failed: %m\n", err);
		goto out;
	}

	config = conf_config();
	if (!config) {
		err = ENOENT;
		warning("sl_init: conf_config failed");
		goto out;
	}

	err = baresip_init(config);
	if (err) {
		warning("sl_init: baresip init failed (%m)\n", err);
		goto out;
	}

	err = ua_init("StudioLink", true, true, true);
	if (err) {
		warning("sl_init: ua_init failed (%m)\n", err);
		goto out;
	}

	for (size_t i = 0; i < ARRAY_SIZE(modv); i++) {

		err = module_load(".", modv[i]);
		if (err) {
			warning("could not pre-load module"
				" '%s' (%m)\n",
				modv[i], err);
		}
	}

	err = sl_ws_init();
	if (err) {
		warning("sl_init: ws init failed (%m)\n", err);
		goto out;
	}

	err = sl_http_listen(&httpsock);
	if (err) {
		warning("sl_init: http_listen failed (%m)\n", err);
		goto out;
	}

	err = sl_tracks_init();
	if (err) {
		warning("sl_init: tracks init failed (%m)\n", err);
		goto out;
	}

out:
	if (err)
		sl_close();

	return err;
}


static void *open_ui(void *arg)
{
	(void)arg;

	/* @TODO: add google-chrome and xdg-open fallback */
	/* @TODO: use permanent browser dir */
	(void)system("chromium --app=http://127.0.0.1:9999 "
		     "--user-data-dir=/tmp/.studio-link/browser "
		     "--window-size=1060,768 >/dev/null 2>&1");

	/* @TODO: call re_cancel() (must be done from main thread) */
	return NULL;
}


int sl_open_webui(void)
{
	if (headless)
		return 0;

	return pthread_create(&open, NULL, open_ui, NULL);
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
	mem_deref(httpsock);

	sl_tracks_close();

	ua_stop_all(true);
	ua_close();
	module_app_unload();
	conf_close();

	baresip_close();
	mod_close();

	re_thread_async_close();

	/* Check for open timers */
	tmr_debug();

	libre_close();

	/* Check for memory leaks */
	mem_debug();
}
