#include <studiolink.h>


struct slaudio {
	struct le le;
	struct sl_track *local_track;
	struct list remotel;
	const struct list *devl_src;
	const struct list *devl_play;
	struct auplay_st *auplay_st;
	struct ausrc_st *ausrc_st;
	struct ausrc_prm ausrc_prm;
	struct auplay_prm auplay_prm;
};

struct remote_track {
	struct le le;
	struct sl_track *track;
	struct ausrc_st *ausrc_st;
	struct auplay_st *auplay_st;
};

struct ausrc_st {
	struct aubuf *aubuf;
	enum aufmt fmt;
	struct ausrc_prm *prm;
	uint32_t ptime;
	size_t sampc;
	volatile bool run;
	ausrc_read_h *rh;
	ausrc_error_h *errh;
	void *arg;
};

struct auplay_st {
	struct auplay_prm prm;
	volatile bool run;
	void *sampv;
	size_t sampc;
	auplay_write_h *wh;
	void *arg;
};

static struct ausrc *ausrc;
static struct auplay *auplay;
static struct list local_tracks = LIST_INIT;


int slaudio_odict(struct odict **o, struct slaudio *a)
{
	struct le *le;
	uint32_t i = 0;
	struct odict *o_slaudio;
	struct odict *o_entry;
	char id[ITOA_BUFSZ];
	int err;

	if (!a || !o)
		return EINVAL;

	err = odict_alloc(&o_slaudio, 32);
	if (err)
		return ENOMEM;

	LIST_FOREACH(a->devl_play, le)
	{
		struct mediadev *dev = le->data;

		err = odict_alloc(&o_entry, 32);
		if (err) {
			mem_deref(o_slaudio);
			return ENOMEM;
		}

		odict_entry_add(o_entry, "idx", ODICT_INT, dev->device_index);
		odict_entry_add(o_entry, "name", ODICT_STRING, dev->name);
		odict_entry_add(o_slaudio, str_itoa(i++, id, 10), ODICT_OBJECT,
				o_entry);

		o_entry = mem_deref(o_entry);
	}

	*o = o_slaudio;

	return 0;
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	struct le *le;
	struct slaudio *audio;
	int err = 0;
	(void)device;

	if (!stp || !as || !prm || !rh)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), NULL);
	if (!st)
		return ENOMEM;

	st->rh	  = rh;
	st->errh  = errh;
	st->arg	  = arg;
	st->prm	  = prm;
	st->ptime = prm->ptime;

	if (list_isempty(&local_tracks)) {
		warning("slaudio: no local tracks available");
		return EINVAL;
	}

	/* TODO: refactor for multiple local tracks */
	audio = local_tracks.head->data;

	LIST_FOREACH(&audio->remotel, le)
	{
		struct remote_track *remote = le->data;

		if (remote->ausrc_st)
			continue;

		remote->ausrc_st = st;
		break;
	}

	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	struct le *le;
	struct slaudio *audio;
	int err = 0;
	(void)ap;
	(void)device;

	if (!prm || !wh || !prm->ch || !prm->srate || !prm->ptime)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), NULL);
	if (!st)
		return ENOMEM;

	st->wh	= wh;
	st->arg = arg;
	st->prm = *prm;

	if (list_isempty(&local_tracks)) {
		warning("slaudio: no local tracks available");
		return EINVAL;
	}

	/* TODO: refactor for multiple local tracks */
	audio = local_tracks.head->data;

	LIST_FOREACH(&audio->remotel, le)
	{
		struct remote_track *remote = le->data;

		if (remote->auplay_st)
			continue;

		remote->auplay_st = st;
		break;
	}

	if (err)
		mem_deref(st);
	else if (stp)
		*stp = st;

	return err;
}


int sl_audio_del_remote_track(struct sl_track *track)
{
	struct le *le;

	if (!track)
		return EINVAL;

	LIST_FOREACH(&local_tracks, le)
	{
		struct slaudio *audio = le->data;
		struct le *rle;

		LIST_FOREACH(&audio->remotel, rle)
		{
			struct remote_track *remote = rle->data;

			if (remote->track != track)
				continue;

			list_unlink(&remote->le);
			mem_deref(remote);

			return 0;
		}
	}

	return ENODATA;
}


int sl_audio_add_remote_track(struct slaudio *audio, struct sl_track *track)
{
	struct remote_track *remote;

	if (!audio || !track)
		return EINVAL;

	remote = mem_zalloc(sizeof(*remote), NULL);
	if (!remote)
		return ENOMEM;

	remote->track = track;

	list_append(&audio->remotel, &remote->le, remote);

	return 0;
}


static void slaudio_destructor(void *data)
{
	struct slaudio *audio = data;

	list_unlink(&audio->le);
	list_flush(&audio->remotel);

	mem_deref(audio->auplay_st);
	mem_deref(audio->ausrc_st);
}


static void auplay_write_handler(struct auframe *af, void *arg)
{
	(void)af;
	(void)arg;
}


static void ausrc_read_handler(struct auframe *af, void *arg)
{
	(void)af;
	(void)arg;
}


int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track)
{
	struct slaudio *a;
	const struct auplay *play;
	const struct ausrc *src;
	int err;

	if (!audiop || !track)
		return EINVAL;

	a = mem_zalloc(sizeof(*a), slaudio_destructor);
	if (!a)
		return ENOMEM;

	a->local_track = track;

	list_append(&local_tracks, &a->le, a);

	a->auplay_prm.srate = 48000;
	a->auplay_prm.ch    = 2;
	a->auplay_prm.ptime = 20;
	a->auplay_prm.fmt   = AUFMT_S16LE;

	a->ausrc_prm.srate = 48000;
	a->ausrc_prm.ch	   = 2;
	a->ausrc_prm.ptime = 20;
	a->ausrc_prm.fmt   = AUFMT_S16LE;


	err = auplay_alloc(&a->auplay_st, baresip_auplayl(), "portaudio",
			   &a->auplay_prm, NULL, auplay_write_handler, NULL);
	if (err) {
		warning("slaudio: start_player failed: %m\n", err);
		goto out;
	}
	err = ausrc_alloc(&a->ausrc_st, baresip_ausrcl(), "portaudio",
			  &a->ausrc_prm, NULL, ausrc_read_handler, NULL, NULL);
	if (err) {
		warning("slaudio: start_src failed: %m\n", err);
		goto out;
	}

	play = auplay_find(baresip_auplayl(), "portaudio");
	src  = ausrc_find(baresip_ausrcl(), "portaudio");

	a->devl_play = &play->dev_list;
	a->devl_src  = &src->dev_list;

	*audiop = a;
	info("slaudio: portaudio started\n");

out:
	if (err)
		*audiop = mem_deref(a);

	return err;
}


int sl_audio_init(void)
{
	int err = 0;

	err = ausrc_register(&ausrc, baresip_ausrcl(), "slaudio", src_alloc);
	err |= auplay_register(&auplay, baresip_auplayl(), "slaudio",
			       play_alloc);
	return err;
}


int sl_audio_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}
