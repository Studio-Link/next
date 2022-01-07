#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#include "test.h"

int test_tracks(void)
{
	char json_str[8192] = {0};
	int err		    = 0;
	struct odict *o	    = NULL;

	ASSERT_TRUE(list_count(sl_tracks()) == 1);

	/* Create new remote track */
	err = sl_track_add(SL_TRACK_REMOTE);
	TEST_ERR(err);

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

	/* Add max >99 tracks (char id_str[3]) */
	for (int i = 0; i < 200; i++) {
		err = sl_track_add(SL_TRACK_REMOTE);
		TEST_ERR(err);
	}

	/* Validate json */
	err = re_snprintf(json_str, sizeof(json_str), "%H", sl_tracks_json);
	if (-1 == err)
		err = 0;
	TEST_ERR(err);

	err = json_decode_odict(&o, 32, json_str, sizeof(json_str), 8);
	TEST_ERR(err);
	mem_deref(o);

out:
	if (err)
		warning("json_str: %s\n", json_str);

	return err;
}
