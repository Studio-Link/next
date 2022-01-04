#include <re.h>
#include <studiolink.h>


int main(void)
{
	int err;

	err = sl_init(NULL);
	if (err)
		return err;

	err = sl_main();

	sl_close();

	return err;
}
