#include <studiolink.h>


int main(int argc, char *argv[])
{
	int err;

	err = sl_getopt(argc, argv);
	if (err)
		return err;

	err = sl_baresip_init(NULL);
	if (err)
		return err;

	err = sl_init();
	if (err)
		return err;

	(void)sl_open_webui();

	err = sl_main();

	sl_close();

	return err;
}
