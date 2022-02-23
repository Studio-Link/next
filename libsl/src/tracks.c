#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#define SL_MAX_TRACKS 99

/* Local audio device track */
struct local {
	struct audio *audio;
};

/* Remote audio call track */
struct remote {
	struct call *call;
};

struct sl_track {
	struct le le;
	int id;
	enum sl_track_type type;
	char name[64];
	enum sl_track_status status;
	union
	{
		struct local local;
		struct remote remote;
	} u;
};

static struct list tracks = LIST_INIT;


const struct list *sl_tracks(void)
{
	return &tracks;
}


int sl_tracks_json(struct re_printf *pf)
{
	struct le *le;
	struct odict *o_tracks;
	struct odict *o_track;
	char id_str[6];
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

		err = re_snprintf(id_str, sizeof(id_str), "%d", track->id);
		if (err == -1)
			goto max;

		odict_entry_add(o_track, "name", ODICT_STRING, track->name);

		odict_entry_add(o_tracks, id_str, ODICT_OBJECT, o_track);
		o_track = mem_deref(o_track);
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


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct sl_track *track1 = le1->data;
	struct sl_track *track2 = le2->data;
	(void)arg;

	/* NOTE: important to use less than OR equal to, otherwise
	   the list_sort function may be stuck in a loop */
	return track1->id <= track2->id;
}


int sl_track_next_id(void)
{
	int id = 1;
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			++id;
			continue;
		}
		break;
	}

	return id;
}


int sl_track_add(enum sl_track_type type)
{
	struct sl_track *track;

	if (list_count(&tracks) >= SL_MAX_TRACKS) {
		warning("sl_track_add: max. %d tracks reached\n",
			SL_MAX_TRACKS);
		return E2BIG;
	}

	track = mem_zalloc(sizeof(struct sl_track), NULL);
	if (!track)
		return ENOMEM;

	track->id     = sl_track_next_id();
	track->type   = type;
	track->status = SL_TRACK_IDLE;
	list_append(&tracks, &track->le, track);
	list_sort(&tracks, sort_handler, NULL);

	return 0;
}


int sl_track_del(int id)
{
	struct le *le;

	/* do not delete last local track */
	if (id == 1)
		return EACCES;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			list_unlink(le);
			mem_deref(track);
			return 0;
		}
	}
	return ENOENT;
}


enum sl_track_status sl_track_status(int id)
{
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id)
			return track->status;
	}

	return SL_TRACK_NOT_EXISTS;
}


int sl_tracks_init(void)
{
	int err;

	err = sl_track_add(SL_TRACK_LOCAL);

	return err;
}


int sl_tracks_close(void)
{
	list_flush(&tracks);

	return 0;
}
