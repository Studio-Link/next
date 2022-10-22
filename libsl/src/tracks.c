#include <re.h>
#include <baresip.h>
#include <studiolink.h>

#define SL_MAX_TRACKS 16

/* Local audio device track */
struct local {
	struct slaudio *slaudio;
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
	char error[128];
	enum sl_track_status status;
	union
	{
		struct local local;
		struct remote remote;
	} u;
};

static struct list tracks = LIST_INIT;

/* TODO: refactor allow multiple local tracks */
static struct sl_track *local_track = NULL;


const struct list *sl_tracks(void)
{
	return &tracks;
}


int sl_tracks_json(struct re_printf *pf)
{
	struct le *le;
	struct odict *o_tracks;
	struct odict *o_track;
	char id[ITOA_BUFSZ];
	int err;

	if (!pf)
		return EINVAL;

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


		if (track->type == SL_TRACK_LOCAL) {
			struct odict *o_slaudio;
			odict_entry_add(o_track, "type", ODICT_STRING,
					"local");

			err = slaudio_odict(&o_slaudio,
					    track->u.local.slaudio);
			if (err)
				return err;

			odict_entry_add(o_track, "audio", ODICT_OBJECT,
					o_slaudio);
			o_slaudio = mem_deref(o_slaudio);
		}

		if (track->type == SL_TRACK_REMOTE) {
			odict_entry_add(o_track, "type", ODICT_STRING,
					"remote");
		}

		odict_entry_add(o_track, "name", ODICT_STRING, track->name);
		odict_entry_add(o_track, "status", ODICT_INT, track->status);
		odict_entry_add(o_track, "error", ODICT_STRING, track->error);
		odict_entry_add(o_tracks, str_itoa(track->id, id, 10),
				ODICT_OBJECT, o_track);
		o_track = mem_deref(o_track);
	}

	err = json_encode_odict(pf, o_tracks);
	mem_deref(o_tracks);

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


static void track_destructor(void *data)
{
	struct sl_track *track = data;

	if (track->type == SL_TRACK_LOCAL)
		mem_deref(track->u.local.slaudio);

	if (track->type == SL_TRACK_REMOTE) {
		if (track->u.remote.call)
			ua_hangup(sl_account_ua(), track->u.remote.call, 0,
				  NULL);
		sl_audio_del_remote_track(track);
	}
}


int sl_track_add(struct sl_track **trackp, enum sl_track_type type)
{
	struct sl_track *track;

	if (!trackp)
		return EINVAL;

	if (list_count(&tracks) >= SL_MAX_TRACKS) {
		warning("sl_track_add: max. %d tracks reached\n",
			SL_MAX_TRACKS);
		return E2BIG;
	}

	track = mem_zalloc(sizeof(struct sl_track), track_destructor);
	if (!track)
		return ENOMEM;

	track->id     = sl_track_next_id();
	track->type   = type;
	track->status = SL_TRACK_IDLE;

	list_append(&tracks, &track->le, track);
	list_sort(&tracks, sort_handler, NULL);

	if (local_track)
		sl_audio_add_remote_track(local_track->u.local.slaudio, track);

	*trackp = track;

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


struct sl_track *sl_track_by_id(int id)
{
	struct le *le;

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;
		if (track->id == id) {
			return track;
		}
	}
	return NULL;
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

	return SL_TRACK_INVALID;
}


struct slaudio *sl_track_audio(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_LOCAL)
		return NULL;

	return track->u.local.slaudio;
}


int sl_track_dial(struct sl_track *track, struct pl *peer)
{
	int err;
	char *peerc = NULL;

	if (!track || track->type != SL_TRACK_REMOTE)
		return EINVAL;

	track->error[0] = '\0';

	err = account_uri_complete_strdup(ua_account(sl_account_ua()), &peerc,
					  peer);
	if (err)
		goto out;

	err = ua_connect(sl_account_ua(), &track->u.remote.call, NULL, peerc,
			 VIDMODE_OFF);
	if (err)
		goto out;

	track->status = SL_TRACK_REMOTE_CALLING;
	pl_strcpy(peer, track->name, sizeof(track->name));


out:
	if (err) {
		re_snprintf(track->error, sizeof(track->error), "%m", err);
	}

	if (err == EINVAL)
		str_ncpy(track->error, "Invalid ID", sizeof(track->error));


	sl_track_ws_send();

	mem_deref(peerc);

	return err;
}


void sl_track_accept(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_REMOTE)
		return;

	ua_answer(sl_account_ua(), track->u.remote.call, VIDMODE_OFF);
}


void sl_track_hangup(struct sl_track *track)
{
	if (!track || track->type != SL_TRACK_REMOTE)
		return;

	ua_hangup(sl_account_ua(), track->u.remote.call, 0, "");

	track->name[0]	     = '\0';
	track->u.remote.call = NULL;
}


void sl_track_ws_send(void)
{
	char *json_str = mem_zalloc(SL_MAX_JSON + 1, NULL);
	re_snprintf(json_str, SL_MAX_JSON, "%H", sl_tracks_json);
	sl_ws_send_str(WS_TRACKS, json_str);
	mem_deref(json_str);
}


static void call_incoming(struct call *call)
{
	struct le *le;
	struct sl_track *track;

	if (!call)
		return;

	LIST_FOREACH(&tracks, le)
	{
		track = le->data;

		if (track->type != SL_TRACK_REMOTE)
			continue;

		if (track->u.remote.call)
			continue;

		goto out;
	}

	/* Add new track if no empty remote track is found */
	sl_track_add(&track, SL_TRACK_REMOTE);

out:
	track->u.remote.call = call;
	track->status	     = SL_TRACK_REMOTE_INCOMING;
	str_ncpy(track->name, call_peeruri(call), sizeof(track->name));
}


static void eventh(struct ua *ua, enum ua_event ev, struct call *call,
		   const char *prm, void *arg)
{
	struct le *le;
	bool changed = false;
	(void)ua;
	(void)arg;

	if (ev == UA_EVENT_CALL_INCOMING) {
		call_incoming(call);
		sl_track_ws_send();
		return;
	}

	if (ev == UA_EVENT_REGISTERING) {
		if (local_track) {
			str_ncpy(local_track->name,
				 account_aor(ua_account(sl_account_ua())),
				 sizeof(local_track->name));
			local_track->status = SL_TRACK_LOCAL_REGISTERING;
			sl_track_ws_send();
		}
		return;
	}

	if (ev == UA_EVENT_REGISTER_OK) {
		if (local_track) {
			local_track->status = SL_TRACK_LOCAL_REGISTER_OK;
			sl_track_ws_send();
		}
		return;
	}

	if (ev == UA_EVENT_REGISTER_FAIL) {
		if (local_track) {
			local_track->status = SL_TRACK_LOCAL_REGISTER_FAIL;
			sl_track_ws_send();
		}
		return;
	}

	LIST_FOREACH(&tracks, le)
	{
		struct sl_track *track = le->data;

		if (track->type != SL_TRACK_REMOTE)
			continue;

		if (track->u.remote.call != call)
			continue;


		if (ev == UA_EVENT_CALL_RINGING) {
			track->status = SL_TRACK_REMOTE_CALLING;
			changed	      = true;
		}

		if (ev == UA_EVENT_CALL_ESTABLISHED) {
			track->status = SL_TRACK_REMOTE_CONNECTED;
			changed	      = true;
		}

		if (ev == UA_EVENT_CALL_CLOSED) {
			track->status	     = SL_TRACK_IDLE;
			track->u.remote.call = NULL;
			track->name[0]	     = '\0';
			if (call_scode(call) != 200)
				str_ncpy(track->error, prm,
					 sizeof(track->error));
			changed = true;
		}
	}

	if (!changed)
		return;

	sl_track_ws_send();
}


int sl_tracks_init(void)
{
	int err;

	uag_event_register(eventh, NULL);

	err = sl_track_add(&local_track, SL_TRACK_LOCAL);
	if (err)
		return err;

	(void)sl_audio_alloc(&local_track->u.local.slaudio, local_track);

	return 0;
}


int sl_tracks_close(void)
{
	list_flush(&tracks);
	uag_event_unregister(eventh);

	local_track = NULL;

	return 0;
}
