#define ASSERT_TRUE(cond)                                                     \
	if (!(cond)) {                                                        \
		warning("selftest: ASSERT_TRUE: %s:%u:\n", __FILE__,          \
			__LINE__);                                            \
		err = EINVAL;                                                 \
		goto out;                                                     \
	}

#define ASSERT_EQ(expected, actual)                                           \
	if ((expected) != (actual)) {                                         \
		warning("selftest: ASSERT_EQ: %s:%u: %s():"                   \
			" expected=%d(0x%x), actual=%d(0x%x)\n",              \
			__FILE__, __LINE__, __func__, (expected), (expected), \
			(actual), (actual));                                  \
		err = EINVAL;                                                 \
		goto out;                                                     \
	}

#define TEST_ERR(err)                                                         \
	if ((err)) {                                                          \
		(void)re_fprintf(stderr, "\n");                               \
		warning("TEST_ERR: %s:%u:"                              \
			      " (%m)\n",                                      \
			      __FILE__, __LINE__, (err));                     \
		goto out;                                                     \
	}

/* functions */
int sl_main_timeout(uint32_t timeout_ms);

/* test cases */
int test_sl_http(void);
int test_tracks(void);
