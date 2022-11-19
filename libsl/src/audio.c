#include <studiolink.h>

enum { PTIME = 20, SRATE = 48000, CH = 2 };

struct slaudio {
	struct le le;
	struct sl_track *local_track;
	struct auplay_st *auplay_st;
	struct ausrc_st *ausrc_st;
	struct ausrc_prm ausrc_prm;
	struct auplay_prm auplay_prm;
	struct {
		const struct list *devl;
		int selected;
	} play, src;
	struct mix_source *mix_src; /**< filled by local driver */
	struct aubuf *ab_mix;
};

struct ausrc_st {
	struct le le;
	ausrc_read_h *rh;
	void *arg;
	struct auplay_st *st_play;
};

struct auplay_st {
	struct le le;
	auplay_write_h *wh;
	int16_t *sampv;
	void *arg;
	uint64_t ts;
	struct ausrc_st *st_src;
	struct mix_source *mix_src; /**< filled by remote */
};

static struct mix *mix		= NULL;
static struct ausrc *ausrc	= NULL;
static struct auplay *auplay	= NULL;
static struct list auplayl	= LIST_INIT;
static struct list ausrcl	= LIST_INIT;
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

	return ENODATA;
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

	return ENODATA;
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


static void mix_write_h(struct auframe *af, void *arg)
{
	struct auplay_st *st_play = arg;

	if (!st_play || !st_play->st_src)
		return;

	af->timestamp = st_play->ts;
	st_play->st_src->rh(af, st_play->st_src->arg);

	st_play->ts += PTIME * 1000;
}


static void mix_read_h(struct auframe *af, void *arg)
{
	struct auplay_st *st_play = arg;

	if (!st_play)
		return;

	st_play->wh(af, st_play->arg);
}


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;
	list_unlink(&st->le);
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	struct le *le;
	int err = 0;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm || !rh)
		return EINVAL;

	if (prm->ch != CH || prm->srate != SRATE || prm->fmt != AUFMT_S16LE ||
	    prm->ptime != PTIME)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->rh	= rh;
	st->arg = arg;

	if (list_isempty(&local_tracks)) {
		warning("slaudio: no local tracks available\n");
		return EINVAL;
	}

	/* setup if auplay is started before ausrc */
	for (le = list_head(&auplayl); le; le = le->next) {
		struct auplay_st *st_play = le->data;

		/* compare struct audio arg */
		if (st->arg == st_play->arg) {
			st_play->st_src = st;
			st->st_play	= st_play;

			break;
		}
	}

	list_append(&ausrcl, &st->le, st);

	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	list_unlink(&st->le);
	mem_deref(st->mix_src);
	mem_deref(st->sampv);
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	struct le *le;
	int err = 0;
	(void)ap;
	(void)device;

	if (!prm || !wh)
		return EINVAL;

	if (prm->ch != CH || prm->srate != SRATE || prm->fmt != AUFMT_S16LE ||
	    prm->ptime != PTIME)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->sampv = mem_zalloc((SRATE * CH * PTIME / 1000) * sizeof(int16_t),
			       NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	st->wh	= wh;
	st->arg = arg;

	if (list_isempty(&local_tracks)) {
		warning("slaudio: no local tracks available\n");
		return EINVAL;
	}

	/* setup if ausrc is started before auplay */
	for (le = list_head(&ausrcl); le; le = le->next) {
		struct ausrc_st *st_src = le->data;

		/* compare struct audio arg */
		if (st->arg == st_src->arg) {
			st_src->st_play = st;
			st->st_src	= st_src;

			break;
		}
	}

	err = mix_source_alloc(&st->mix_src, mix, mix_write_h, mix_read_h, st);
	if (err)
		goto out;

	list_append(&auplayl, &st->le, st);

out:
	if (err)
		mem_deref(st);
	else if (stp)
		*stp = st;

	return err;
}


static void driver_mixh(struct auframe *af, void *arg)
{
	struct slaudio *a = arg;

	aubuf_write_auframe(a->ab_mix, af);
}


static void driver_write_handler(struct auframe *af, void *arg)
{
	(void)af;
	struct slaudio *a = arg;

	if (af->fmt != AUFMT_S16LE) {
		warning("auplay wrong format\n");
		return;
	}

	aubuf_read_auframe(a->ab_mix, af);
}


static void driver_read_handler(struct auframe *af, void *arg)
{
	float sampv[4096];
	struct slaudio *a = arg;

	if (af->fmt != AUFMT_S16LE || af->ch > 2) {
		warning("ausrc wrong format\n");
		return;
	}

	auconv_from_s16(AUFMT_FLOAT, sampv, af->sampv, af->sampc);
	sl_meter_process(0, sampv, af->sampc / 2);

	mix_sources(mix, a->mix_src, af);
}


static int driver_start(struct slaudio *a)
{
	char index[ITOA_BUFSZ];
	struct sl_config *conf;
	int err;

	conf = sl_conf();

	if (!conf)
		return EINVAL;

	if (a->auplay_st)
		a->auplay_st = mem_deref(a->auplay_st);

	if (a->ausrc_st)
		a->ausrc_st = mem_deref(a->ausrc_st);

	err = auplay_alloc(&a->auplay_st, baresip_auplayl(), conf->play.mod,
			   &a->auplay_prm,
			   str_itoa(a->play.selected, index, 10),
			   driver_write_handler, a);
	if (err) {
		warning("slaudio: start_player failed: %m\n", err);
		return err;
	}

	err = ausrc_alloc(&a->ausrc_st, baresip_ausrcl(), conf->src.mod,
			  &a->ausrc_prm, str_itoa(a->src.selected, index, 10),
			  driver_read_handler, NULL, a);
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
	mem_deref(audio->auplay_st);
	mem_deref(audio->ausrc_st);
	mem_deref(audio->mix_src);
	mem_deref(audio->ab_mix);
}


static void driver_alloc(struct slaudio *a)
{
	const struct auplay *play;
	const struct ausrc *src;
	struct sl_config *conf;
	struct le *le;

	conf = sl_conf();

	if (!a || !conf)
		return;

	play = auplay_find(baresip_auplayl(), conf->play.mod);
	if (!play)
		return;
	a->play.devl = &play->dev_list;

	src = ausrc_find(baresip_ausrcl(), conf->src.mod);
	if (!src)
		return;
	a->src.devl = &src->dev_list;


	/* set default source */
	LIST_FOREACH(a->src.devl, le)
	{
		struct mediadev *dev = le->data;

		if (!dev->src.is_default)
			continue;

		a->src.selected = dev->device_index;
		break;
	}

	/* set default play */
	LIST_FOREACH(a->play.devl, le)
	{
		struct mediadev *dev = le->data;

		if (!dev->play.is_default)
			continue;

		a->play.selected = dev->device_index;
		break;
	}

	info("slaudio: %s/%s allocated\n", conf->play.mod, conf->src.mod);
}


int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track)
{
	struct slaudio *a;
	int err;

	if (!audiop || !track)
		return EINVAL;

	a = mem_zalloc(sizeof(*a), slaudio_destructor);
	if (!a)
		return ENOMEM;

	a->local_track = track;

	list_append(&local_tracks, &a->le, a);

	a->auplay_prm.srate = SRATE;
	a->auplay_prm.ch    = CH;
	a->auplay_prm.ptime = PTIME;
	a->auplay_prm.fmt   = AUFMT_S16LE;

	a->ausrc_prm.srate = SRATE;
	a->ausrc_prm.ch	   = CH;
	a->ausrc_prm.ptime = PTIME;
	a->ausrc_prm.fmt   = AUFMT_S16LE;

	driver_alloc(a);

	err = aubuf_alloc(&a->ab_mix, 0,
			  (SRATE * CH * PTIME) / 1000 * sizeof(int16_t) * 10);
	if (err)
		goto out;

	err = mix_source_alloc(&a->mix_src, mix, driver_mixh, NULL, a);
	if (err)
		goto out;

out:
	if (err) {
		mem_deref(a);
		warning("sl_audio_alloc: failed %m\n", err);
	}
	else
		*audiop = a;

	return err;
}


int sl_audio_init(void)
{
	struct sl_config *conf = sl_conf();
	int err;

	str_ncpy(conf->baresip->audio.play_mod, "slaudio",
		 sizeof(conf->baresip->audio.play_mod));

	str_ncpy(conf->baresip->audio.src_mod, "slaudio",
		 sizeof(conf->baresip->audio.src_mod));

	err = ausrc_register(&ausrc, baresip_ausrcl(), "slaudio", src_alloc);
	err |= auplay_register(&auplay, baresip_auplayl(), "slaudio",
			       play_alloc);
	if (err) {
		warning("sl_audio_init: register failed %m\n", err);
		return err;
	}

	err = mix_alloc(&mix);

	return err;
}


int sl_audio_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);
	mix    = mem_deref(mix);

	return 0;
}
