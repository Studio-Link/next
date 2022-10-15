#include <studiolink.h>


struct slaudio {
	struct le le;
	struct sl_track *local_track;
	struct list remotel;
	struct auplay_st *auplay_st;
	struct ausrc_st *ausrc_st;
	struct ausrc_prm ausrc_prm;
	struct auplay_prm auplay_prm;
	struct {
		const struct list *devl;
		int selected;
	} play, src;
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


static int device_play_by_index(struct slaudio *a, int index, char **device)
{
	struct le *le;

	LIST_FOREACH(a->play.devl, le)
	{
		struct mediadev *dev = le->data;

		if (dev->device_index == index) {
			str_dup(device, dev->name);
			return 0;
		}
	}

	return ENOKEY;
}


static int device_src_by_index(struct slaudio *a, int index, char **device)
{
	struct le *le;

	LIST_FOREACH(a->src.devl, le)
	{
		struct mediadev *dev = le->data;

		if (dev->device_index == index) {
			*device = dev->name;
			str_dup(device, dev->name);
			return 0;
		}
	}

	return ENOKEY;
}


int slaudio_odict(struct odict **o, struct slaudio *a)
{
	struct le *le;
	uint32_t i = 0;
	struct odict *o_slaudio;
	struct odict *o_slaudio_src;
	struct odict *o_slaudio_play;
	struct odict *o_entry;
	char id[ITOA_BUFSZ];
	int err;

	if (!a || !o)
		return EINVAL;

	err = odict_alloc(&o_slaudio, 32);
	if (err)
		return ENOMEM;

	err = odict_alloc(&o_slaudio_src, 32);
	if (err)
		return ENOMEM;

	err = odict_alloc(&o_slaudio_play, 32);
	if (err)
		return ENOMEM;

	LIST_FOREACH(a->src.devl, le)
	{
		struct mediadev *dev = le->data;

		err = odict_alloc(&o_entry, 32);
		if (err) {
			mem_deref(o_slaudio);
			return ENOMEM;
		}

		odict_entry_add(o_entry, "idx", ODICT_INT, dev->device_index);
		odict_entry_add(o_entry, "name", ODICT_STRING, dev->name);
		odict_entry_add(o_slaudio_src, str_itoa(i++, id, 10),
				ODICT_OBJECT, o_entry);

		o_entry = mem_deref(o_entry);
	}

	i = 0;
	LIST_FOREACH(a->play.devl, le)
	{
		struct mediadev *dev = le->data;

		err = odict_alloc(&o_entry, 32);
		if (err) {
			mem_deref(o_slaudio);
			return ENOMEM;
		}

		odict_entry_add(o_entry, "idx", ODICT_INT, dev->device_index);
		odict_entry_add(o_entry, "name", ODICT_STRING, dev->name);
		odict_entry_add(o_slaudio_play, str_itoa(i++, id, 10),
				ODICT_OBJECT, o_entry);

		o_entry = mem_deref(o_entry);
	}

	odict_entry_add(o_slaudio, "src", ODICT_OBJECT, o_slaudio_src);
	odict_entry_add(o_slaudio, "play", ODICT_OBJECT, o_slaudio_play);

	o_slaudio_src  = mem_deref(o_slaudio_src);
	o_slaudio_play = mem_deref(o_slaudio_play);

	err = odict_alloc(&o_slaudio_src, 32);
	err |= odict_alloc(&o_slaudio_play, 32);
	if (err)
		return ENOMEM;

	odict_entry_add(o_slaudio, "src_dev", ODICT_INT, a->src.selected);
	odict_entry_add(o_slaudio, "play_dev", ODICT_INT, a->play.selected);

	mem_deref(o_slaudio_src);
	mem_deref(o_slaudio_play);

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


static int driver_start(struct slaudio *a)
{
	char index[ITOA_BUFSZ];
	struct config *conf;
	int err;

	conf = conf_config();

	if (!conf)
		return EINVAL;

	if (a->auplay_st)
		a->auplay_st = mem_deref(a->auplay_st);

	if (a->ausrc_st)
		a->ausrc_st = mem_deref(a->ausrc_st);

	err = auplay_alloc(&a->auplay_st, baresip_auplayl(),
			   conf->audio.play_mod, &a->auplay_prm,
			   str_itoa(a->play.selected, index, 10),
			   auplay_write_handler, NULL);
	if (err) {
		warning("slaudio: start_player failed: %m\n", err);
		return err;
	}

	err = ausrc_alloc(&a->ausrc_st, baresip_ausrcl(), conf->audio.src_mod,
			  &a->ausrc_prm, str_itoa(a->src.selected, index, 10),
			  ausrc_read_handler, NULL, NULL);
	if (err) {
		warning("slaudio: start_src failed: %m\n", err);
		return err;
	}

	info("slaudio: driver started\n");

	return err;
}


int sl_audio_set_device(struct slaudio *audio, int play_idx, int src_idx)
{
	char *play_dev = NULL;
	char *src_dev  = NULL;

	if (!audio || play_idx < 0 || src_idx < 0)
		return EINVAL;

	audio->play.selected = play_idx;
	audio->src.selected  = src_idx;

	device_play_by_index(audio, play_idx, &play_dev);
	device_src_by_index(audio, src_idx, &src_dev);

	info("slaudio: set device speaker: %s\n", play_dev);
	info("slaudio: set device mic: %s\n", src_dev);

	driver_start(audio);

	mem_deref(play_dev);
	mem_deref(src_dev);

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


static void driver_alloc(struct slaudio *a)
{
	const struct auplay *play;
	const struct ausrc *src;
	struct config *conf;
	struct le *le;

	conf = conf_config();

	if (!a || !conf)
		return;


	play = auplay_find(baresip_auplayl(), conf->audio.play_mod);
	if (!play)
		return;
	a->play.devl = &play->dev_list;

	src = ausrc_find(baresip_ausrcl(), conf->audio.src_mod);
	if (!src)
		return;
	a->src.devl = &src->dev_list;


	LIST_FOREACH(a->src.devl, le)
	{
		struct mediadev *dev = le->data;

		if (!dev->src.is_default)
			continue;

		a->src.selected = dev->device_index;
		break;
	}

	LIST_FOREACH(a->play.devl, le)
	{
		struct mediadev *dev = le->data;

		if (!dev->play.is_default)
			continue;

		a->play.selected = dev->device_index;
		break;
	}

	info("slaudio: %s/%s allocated\n", conf->audio.play_mod,
	     conf->audio.src_mod);
}


int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track)
{
	struct slaudio *a;

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

	driver_alloc(a);

	*audiop = a;

	return 0;
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
