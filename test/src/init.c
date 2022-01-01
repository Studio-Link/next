#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "test.h"


int test_sl_init_main_close(void)
{
	int err;
	const char *config = "opus_bitrate 64000\n";

	err = sl_init(NULL);
	TEST_ERR(err);
	(void)sl_close();

	err = sl_init((uint8_t *)config);
	TEST_ERR(err);
	sl_main_timeout(1);
	(void)sl_close();
out:
	return err;
}
