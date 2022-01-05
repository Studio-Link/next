#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#include "test.h"

int test_sl_ws(void)
{
	int err;

	err = sl_ws_init();
	TEST_ERR(err);

	err = sl_ws_close();
	TEST_ERR(err);
out:
	return err;
}
