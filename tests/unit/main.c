/**
 * @file tests/main.c  Unit Tests for StudioLink
 */
 
#include <getopt.h>
#include <re.h>
#include <baresip.h>
#include "test.h"


typedef int (test_exec_h)(void);

struct test {
	test_exec_h *exec;
	const char *name;
};

#define TEST(a) {a, #a}

static const struct test tests[] = {
	TEST(test_test),
};


static int run_one_test(const struct test *test)
{
	int err;

	re_printf("[ RUN      ] %s\n", test->name);

	err = test->exec();
	if (err) {
		warning("%s: test failed (%m)\n",
			test->name, err);
		return err;
	}

	re_printf("[       OK ]\n");

	return 0;
}


static int run_tests(void)
{
	size_t i;
	int err;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		re_printf("[ RUN      ] %s\n", tests[i].name);

		err = tests[i].exec();
		if (err) {
			warning("%s: test failed (%m)\n",
				tests[i].name, err);
			return err;
		}

		re_printf("[       OK ]\n");
	}

	return 0;
}


static void test_listcases(void)
{
	size_t i, n;

	n = ARRAY_SIZE(tests);

	(void)re_printf("\n%zu test cases:\n", n);

	for (i=0; i<(n+1)/2; i++) {

		(void)re_printf("    %-32s    %s\n",
				tests[i].name,
				(i+(n+1)/2) < n ? tests[i+(n+1)/2].name : "");
	}

	(void)re_printf("\n");
}


static const struct test *find_test(const char *name)
{
	size_t i;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		if (0 == str_casecmp(name, tests[i].name))
			return &tests[i];
	}

	return NULL;
}


static void usage(void)
{
	(void)re_fprintf(stderr,
			 "Usage: sltest [options] <testcases..>\n"
			 "options:\n"
			 "\t-l               List all testcases and exit\n"
			 "\t-v               Verbose output (INFO level)\n"
			 );
}


int main(int argc, char *argv[])
{
	size_t i, ntests;
	bool verbose = false;
	int err;

	for (;;) {
		const int c = getopt(argc, argv, "hlv");
		if (0 > c)
			break;

		switch (c) {

		case '?':
		case 'h':
			usage();
			return -2;

		case 'l':
			test_listcases();
			return 0;

		case 'v':
			if (verbose)
				log_enable_debug(true);
			else
				log_enable_info(true);
			verbose = true;
			break;

		default:
			break;
		}
	}

	if (argc >= (optind + 1))
		ntests = argc - optind;
	else
		ntests = ARRAY_SIZE(tests);

	re_printf("running StudioLink selftest version %s with %zu tests\n",
		  SL_VERSION, ntests);


	if (argc >= (optind + 1)) {

		for (i=0; i<ntests; i++) {
			const char *name = argv[optind + i];
			const struct test *test;

			test = find_test(name);
			if (test) {
				err = run_one_test(test);
				if (err)
					goto out;
			}
			else {
				re_fprintf(stderr,
					   "testcase not found: `%s'\n",
					   name);
				err = ENOENT;
				goto out;
			}
		}
	}
	else {
		err = run_tests();
		if (err)
			goto out;
	}

	re_printf("\x1b[32mOK. %zu tests passed successfully\x1b[;m\n",
		  ntests);

 out:
	if (err) {
		warning("test failed (%m)\n", err);
		re_printf("%H\n", re_debug, 0);
	}

	return err;
}
