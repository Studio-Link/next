/**
 * @file test.h  Unit Tests for StudioLink
 */

#define ASSERT_TRUE(cond)					\
	if (!(cond)) {						\
		warning("selftest: ASSERT_TRUE: %s:%u:\n",	\
			__FILE__, __LINE__);			\
		err = EINVAL;					\
		goto out;					\
	}

#define ASSERT_EQ(expected, actual)				\
	if ((expected) != (actual)) {				\
		warning("selftest: ASSERT_EQ: %s:%u:"		\
			" expected=%d, actual=%d\n",		\
			__FILE__, __LINE__,			\
			(int)(expected), (int)(actual));	\
		err = EINVAL;					\
		goto out;					\
	}

#define ASSERT_DOUBLE_EQ(expected, actual, prec)			\
	if (!test_cmp_double((expected), (actual), (prec))) {		\
		warning("selftest: ASSERT_DOUBLE_EQ: %s:%u:"		\
			" expected=%f, actual=%f\n",			\
			__FILE__, __LINE__,				\
			(double)(expected), (double)(actual));		\
		err = EINVAL;						\
		goto out;						\
	}

#define ASSERT_STREQ(expected, actual)					\
	if (0 != str_cmp((expected), (actual))) {			\
		warning("selftest: ASSERT_STREQ: %s:%u:"		\
			" expected = '%s', actual = '%s'\n",		\
			__FILE__, __LINE__,				\
			(expected), (actual));				\
		err = EBADMSG;						\
		goto out;						\
	}

#define TEST_ERR(err)							\
	if ((err)) {							\
		(void)re_fprintf(stderr, "\n");				\
		warning("TEST_ERR: %s:%u:"			\
			      " (%m)\n",				\
			      __FILE__, __LINE__,			\
			      (err));					\
		goto out;						\
	}

#define TEST_MEMCMP(expected, expn, actual, actn)			\
	if (expn != actn ||						\
	    0 != memcmp((expected), (actual), (expn))) {		\
		(void)re_fprintf(stderr, "\n");				\
		warning("TEST_MEMCMP: %s:%u:"				\
			" %s(): failed\n",				\
			__FILE__, __LINE__, __func__);			\
		test_hexdump_dual(stderr,				\
				  expected, expn,			\
				  actual, actn);			\
		err = EINVAL;						\
		goto out;						\
	}

#define TEST_STRCMP(expected, expn, actual, actn)			\
	if (expn != actn ||						\
	    0 != memcmp((expected), (actual), (expn))) {		\
		(void)re_fprintf(stderr, "\n");				\
		warning("TEST_STRCMP: %s:%u:"				\
			" failed\n",					\
			__FILE__, __LINE__);				\
		(void)re_fprintf(stderr,				\
				 "expected string: (%zu bytes)\n"	\
				 "\"%b\"\n",				\
				 (size_t)(expn),			\
				 (expected), (size_t)(expn));		\
		(void)re_fprintf(stderr,				\
				 "actual string: (%zu bytes)\n"		\
				 "\"%b\"\n",				\
				 (size_t)(actn),			\
				 (actual), (size_t)(actn));		\
		err = EINVAL;						\
		goto out;						\
	}


/* helpers */

int re_main_timeout(uint32_t timeout_ms);
bool test_cmp_double(double a, double b, double precision);
void test_hexdump_dual(FILE *f,
		       const void *ep, size_t elen,
		       const void *ap, size_t alen);

/* test cases */

int test_test(void);
