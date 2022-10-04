#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "test.h"


int test_sl_db(void)
{
	int err;

	err = sl_db_init();
	TEST_ERR(err);

	sl_db_close();

out:
	return err;
}
