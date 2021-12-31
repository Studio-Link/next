#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "tests.h"

typedef int(test_exec_h)(void);

struct test {
	test_exec_h *exec;
	const char *name;
};


// clang-format off
#define TEST(a) {a, #a}

static const struct test tests[] = {
	TEST(test_sl_init),
	TEST(test_sl_close)
};
// clang-format on


int test_sl_init(void)
{
	int err;

	err = sl_init(NULL);
	if (!err)
		return EINVAL;

	return 0;
}


int test_sl_close(void)
{
	(void)sl_close();
	
	return 0;
}


static int run_tests(void)
{
	size_t i;
	int err;

	for (i = 0; i < ARRAY_SIZE(tests); i++) {

		re_printf(".");

		err = tests[i].exec();
		if (err) {
			re_printf("\n");
			warning("%s: test failed (%m)\n", tests[i].name, err);
			return err;
		}
	}

	re_printf("\n");
	return 0;
}


int main(void)
{
	int err;
	size_t ntests = ARRAY_SIZE(tests);

	(void)sys_coredump_set(true);

	err = run_tests();
	IF_ERR_GOTO_OUT(err);

	re_printf("\x1b[32mOK. %zu tests passed successfully\x1b[;m\n",
		  ntests);

out:
	if (err) {
		warning("test failed (%m)\n", err);
		re_printf("%H\n", re_debug, 0);
	}

	tmr_debug();
	mem_debug();

	return err;
}
