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
	TEST(test_sl_init_main_close),
};
// clang-format on

static void timeout(void *arg)
{
	int *err = arg;

	*err = ETIMEDOUT;

	re_cancel();
}


int sl_main_timeout(uint32_t timeout_ms)
{
	struct tmr tmr;
	int err = 0;

	tmr_init(&tmr);
	tmr_start(&tmr, timeout_ms, timeout, &err);
	sl_main();
	tmr_cancel(&tmr);

	return err;
}


int test_sl_init_main_close(void)
{
	int err;
	const char *config = "opus_bitrate 64000\n";

	err = sl_init(NULL);
	ASSERT_TRUE(err);

	err = sl_init((uint8_t *)config);
	ASSERT_TRUE(!err);

	sl_main_timeout(1);

	(void)sl_close();
out:
	return err;
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
