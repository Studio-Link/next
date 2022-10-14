#include <studiolink.h>


int main(int argc, char *argv[])
{
	struct config *conf;
	int err;

	err = sl_getopt(argc, argv);
	if (err)
		return err;

	err = sl_baresip_init(NULL);
	if (err)
		return err;

	conf = conf_config();

	re_snprintf(conf->audio.play_mod, sizeof(conf->audio.play_mod),
		    "portaudio");

	re_snprintf(conf->audio.src_mod, sizeof(conf->audio.src_mod),
		    "portaudio");

	err = sl_init();
	if (err)
		return err;

	(void)sl_open_webui();

	err = sl_main();

	sl_close();

	return err;
}
