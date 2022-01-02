#include <re.h>
#include <baresip.h>
#include <studiolink.h>


static struct http_sock *httpsock = NULL;


static void signal_handler(int signum)
{
	(void)signum;

	re_fprintf(stderr, "terminated on signal %d\n", signum);

	re_cancel();
}


/**
 * Init StudioLink
 *
 * Initializes Libre, Baresip and StudioLink
 *
 * @param conf Baresip config
 *
 * @return int
 */
int sl_init(const uint8_t *conf)
{
	struct config *config;
	int err;

	err = libre_init();
	if (err)
		return err;

	(void)sys_coredump_set(true);

	if (conf) {
		err = conf_configure_buf(conf, str_len((char *)conf));
		if (err) {
			warning("sl_init: conf_configure failed: %m\n", err);
			goto out;
		}
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

	err = sl_http_listen(&httpsock);
	if (err) {
		warning("sl_init: http_listen failed (%m)\n", err);
		goto out;
	}

out:
	if (err)
		sl_close();

	return err;
}


/**
 * StudioLink Main function
 *
 * @return int
 */
int sl_main(void)
{
	int err;

	err = re_main(signal_handler);

	return err;
}


/**
 * Close/Exit StudioLink
 */
void sl_close(void)
{
	mem_deref(httpsock);

	ua_stop_all(true);
	ua_close();
	module_app_unload();
	conf_close();

	baresip_close();
	mod_close();

	libre_close();

	tmr_debug();
	mem_debug();
}
