#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "test.h"

typedef int(test_exec_h)(void);

struct test {
	test_exec_h *exec;
	const char *name;
};


/* clang-format off */
#define TEST(a) {a, #a}

static const struct test tests[] = {
	TEST(test_sl_http),
	TEST(test_tracks),
};
/* clang-format on */


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
	struct config *conf;
	size_t ntests	    = ARRAY_SIZE(tests);
	struct auplay *play = NULL;
	struct ausrc *src   = NULL;
	int err;

	log_enable_info(false);

	warning("hello world\n");

	err = sl_baresip_init(NULL);
	TEST_ERR(err);

	conf = conf_config();

	re_snprintf(conf->audio.play_mod, sizeof(conf->audio.play_mod),
		    "mock-auplay");

	re_snprintf(conf->audio.src_mod, sizeof(conf->audio.src_mod),
		    "mock-auplay");

	err = mock_auplay_register(&play, baresip_auplayl(), NULL, NULL);
	TEST_ERR(err);
	err = mock_ausrc_register(&src, baresip_ausrcl(), NULL, NULL);
	TEST_ERR(err);

	err = sl_init();
	TEST_ERR(err);

	err = run_tests();
	if (err)
		goto out;

	re_printf("\x1b[32mOK. %zu tests passed successfully\x1b[;m\n",
		  ntests);

out:
	if (err) {
		warning("test failed (%m)\n", err);
		re_printf("%H\n", re_debug, 0);
	}

	mem_deref(play);
	mem_deref(src);

	sl_close();

	return err;
}
