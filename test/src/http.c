#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "test.h"

static int test_err = 0;

static void http_resp_handler(int err, const struct http_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	test_err = err;

	re_cancel();
}


int test_sl_http(void)
{
	int err;
	struct sl_httpc *http = NULL;

	err = sl_httpc_alloc(&http, http_resp_handler);
	TEST_ERR(err);

	/* Test index page */
	err = sl_httpc_req(http, SL_HTTP_GET, "http://127.0.0.1:9999/");
	TEST_ERR(err);

	err = sl_main_timeout(100);
	TEST_ERR(err);

	err = test_err;
	TEST_ERR(err);

	/* Test 404 page */
	err = sl_httpc_req(http, SL_HTTP_GET,
			  "http://127.0.0.1:9999/not_exists");
	TEST_ERR(err);

	err = sl_main_timeout(100);
	TEST_ERR(err);

	err = test_err;
	TEST_ERR(err);

out:
	mem_deref(http);
	return err;
}
