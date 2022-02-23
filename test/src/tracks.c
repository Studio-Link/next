#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#include "test.h"

static int test_track_add(void)
{
	char json_str[8192] = {0};
	struct odict *o	    = NULL;
	int err;

	err = sl_tracks_init();
	TEST_ERR(err);

	/* Create new remote track */
	err = sl_track_add(SL_TRACK_REMOTE);
	TEST_ERR(err);
	ASSERT_EQ(3, sl_track_next_id());
	ASSERT_EQ(SL_TRACK_IDLE, sl_track_status(2));
	ASSERT_EQ(2, list_count(sl_tracks()));

	/* Get all tracks as json string */
	err = re_snprintf(json_str, sizeof(json_str), "%H", sl_tracks_json);
	ASSERT_TRUE(-1 != err);
	ASSERT_TRUE(0 == str_cmp("{\"1\":{\"type\":\"local\",\"name\":\"\"},"
				 "\"2\":{\"type\":"
				 "\"remote\",\"name\":\"\"}}",
				 json_str));
	/* Validate json */
	err = json_decode_odict(&o, 32, json_str, sizeof(json_str), 8);
	TEST_ERR(err);
	mem_deref(o);

	/* Add max >99 tracks */
	for (int i = 0; i < 97; i++) {
		err = sl_track_add(SL_TRACK_REMOTE);
		TEST_ERR(err);
		ASSERT_EQ((i + 4), sl_track_next_id());
	}
	ASSERT_TRUE(list_count(sl_tracks()) == 99);
	err = sl_track_add(SL_TRACK_REMOTE);
	ASSERT_EQ(E2BIG, err);

	/* Test delete and re-add after max tracks reached */
	err = sl_track_del(18);
	TEST_ERR(err);

	ASSERT_EQ(18, sl_track_next_id());

	err = sl_track_add(SL_TRACK_REMOTE);
	TEST_ERR(err);

	err = sl_track_add(SL_TRACK_REMOTE);
	ASSERT_EQ(E2BIG, err);

	/* Validate json */
	err = re_snprintf(json_str, sizeof(json_str), "%H", sl_tracks_json);
	ASSERT_TRUE(-1 != err);
	err = json_decode_odict(&o, 32, json_str, sizeof(json_str), 8);
	TEST_ERR(err);
	mem_deref(o);

	err = sl_tracks_close();
	TEST_ERR(err);

out:
	return err;
}


static int test_track_delete(void)
{
	int err;

	err = sl_tracks_init();
	TEST_ERR(err);

	ASSERT_TRUE(sl_track_status(2) == SL_TRACK_NOT_EXISTS);

	/* Create new remote track */
	err = sl_track_add(SL_TRACK_REMOTE);
	TEST_ERR(err);
	ASSERT_TRUE(sl_track_status(2) == SL_TRACK_IDLE);

	ASSERT_TRUE(list_count(sl_tracks()) == 2);

	/* Remove track */
	err = sl_track_del(2);
	TEST_ERR(err);

	ASSERT_TRUE(list_count(sl_tracks()) == 1);
	ASSERT_TRUE(sl_track_status(2) == SL_TRACK_NOT_EXISTS);

	/* Test remove last local track - not allowed */
	err = sl_track_del(1);
	ASSERT_TRUE(err == EACCES);

	err = sl_tracks_close();
	TEST_ERR(err);

out:
	return err;
}


static int test_tracks_init(void)
{
	int err;

	ASSERT_TRUE(list_count(sl_tracks()) == 0);

	err = sl_tracks_init();
	TEST_ERR(err);

	/* Check local track init */
	ASSERT_TRUE(list_count(sl_tracks()) == 1);

	err = sl_tracks_close();
	TEST_ERR(err);
out:
	return err;
}


int test_tracks(void)
{
	int err = 0;

	err = sl_tracks_close(); /* initialized by sl_init() */
	TEST_ERR(err);

	err = test_tracks_init();
	TEST_ERR(err);

	err = test_track_add();
	TEST_ERR(err);

	err = test_track_delete();
	TEST_ERR(err);

out:
	return err;
}
