#include <re.h>
#include <baresip.h>
#include <studiolink.h>


static int ws_init(void)
{
	return 0;
}


/**
 * Init StudioLink
 *
 * @return int
 */
int sl_init(void)
{
	int err;

	err = ws_init();

	return err;
}


/**
 * StudioLink Main function
 *
 * @return int
 */
int sl_main(void)
{
	int err = 0;

	return err;
}


/**
 * Close/Exit StudioLink
 *
 * @return int
 */
void sl_close(void)
{
}
