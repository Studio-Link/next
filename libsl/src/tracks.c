#include <re.h>
#include <baresip.h>
#include <studiolink.h>

struct sl_track {
	struct le le;
	enum sl_track_type type;
	char name[32];
	enum sl_track_status status;
};

static struct list tracks;


const struct list *sl_tracks(void)
{
	return &tracks;
}


int sl_tracks_json(struct re_printf *pf)
{
	struct le *le;
	struct odict *o_tracks;
	struct odict *o_track;
	char id_str[3]; /*max "99\0" tracks*/
	int id = 0;
	int err;

	err = odict_alloc(&o_tracks, 32);
	if (err)
		return ENOMEM;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (!track)
			continue;

		err = odict_alloc(&o_track, 32);
		if (err)
			return ENOMEM;


		if (track->type == SL_TRACK_LOCAL)
			odict_entry_add(o_track, "type", ODICT_STRING,
					"local");

		if (track->type == SL_TRACK_REMOTE)
			odict_entry_add(o_track, "type", ODICT_STRING,
					"remote");

		err = re_snprintf(id_str, sizeof(id_str), "%d", id);
		if (err == -1)
			goto max;
		odict_entry_add(o_tracks, id_str, ODICT_OBJECT, o_track);
		o_track = mem_deref(o_track);

		++id;
	}

	err = json_encode_odict(pf, o_tracks);
	mem_deref(o_tracks);

	return err;

max:
	json_encode_odict(pf, o_tracks);
	mem_deref(o_tracks);
	mem_deref(o_track);

	return err;
}


int sl_track_add(enum sl_track_type type)
{
	struct sl_track *track;
	track = mem_zalloc(sizeof(struct sl_track), NULL);
	if (!track)
		return ENOMEM;

	track->type = type;
	list_append(&tracks, &track->le, track);

	return 0;
}


int sl_tracks_init(void)
{
	int err;

	list_init(&tracks);
	err = sl_track_add(SL_TRACK_LOCAL);

	return err;
}


int sl_tracks_close(void)
{
	list_flush(&tracks);
	return 0;
}
