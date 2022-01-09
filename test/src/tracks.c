#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#include "test.h"

int test_tracks(void)
{
	char json_str[8192] = {0};
	int err		    = 0;
	struct odict *o	    = NULL;

	/* Check local track init */
	ASSERT_TRUE(list_count(sl_tracks()) == 1);

	/* Create new remote track */
	err = sl_track_add(SL_TRACK_REMOTE);
	TEST_ERR(err);
	ASSERT_TRUE(sl_track_last_id() == 1);
	ASSERT_TRUE(sl_track_status(1) == SL_TRACK_IDLE);
	ASSERT_TRUE(list_count(sl_tracks()) == 2);

	/* Get all tracks as json string */
	err = re_snprintf(json_str, sizeof(json_str), "%H", sl_tracks_json);
	ASSERT_TRUE(-1 != err);
	ASSERT_TRUE(0 == str_cmp("{\"0\":{\"type\":\"local\"},\"1\":{\"type\":"
				 "\"remote\"}}",
				 json_str));
	/* Validate json */
	err = json_decode_odict(&o, 32, json_str, sizeof(json_str), 8);
	TEST_ERR(err);
	mem_deref(o);

	/* Add max >99 tracks */
	for (int i = 0; i < 97; i++) {
		err = sl_track_add(SL_TRACK_REMOTE);
		TEST_ERR(err);
	}
	ASSERT_TRUE(list_count(sl_tracks()) == 99);
	err = sl_track_add(SL_TRACK_REMOTE);
	ASSERT_TRUE(err == E2BIG);

	/* Validate json */
	err = re_snprintf(json_str, sizeof(json_str), "%H", sl_tracks_json);
	ASSERT_TRUE(-1 != err);
	err = json_decode_odict(&o, 32, json_str, sizeof(json_str), 8);
	TEST_ERR(err);
	mem_deref(o);

	/* Remove track */
	err = sl_track_del(98);
	TEST_ERR(err);
	ASSERT_TRUE(sl_track_status(98) == SL_TRACK_NOT_EXISTS);
out:
	if (err)
		warning("json_str: %s\n", json_str);

	return err;
}
