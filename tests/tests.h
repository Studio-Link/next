#define ASSERT_TRUE(cond)                                       \
        if (!(cond)) {                                          \
                warning("selftest: ASSERT_TRUE: %s:%u:\n",      \
                        __FILE__, __LINE__);                    \
                err = EINVAL;                                   \
                goto out;                                       \
        }

#define ASSERT_EQ(expected, actual)                             \
        if ((expected) != (actual)) {                           \
                warning("selftest: ASSERT_EQ: %s:%u: %s():"     \
                        " expected=%d(0x%x), actual=%d(0x%x)\n",\
                        __FILE__, __LINE__, __func__,           \
                        (expected), (expected),                 \
                        (actual), (actual));                    \
                err = EINVAL;                                   \
                goto out;                                       \
        }


/* test cases */
int test_sl_init(void);
int test_sl_close(void);
