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
	struct aumix_source *mix_src; /**< filled by local driver */
	struct aubuf *ab_mix;
};

struct amix {
	struct le le;
	struct le sle;
	struct ausrc_st *src;
	struct auplay_st *play;
	struct aumix_source *aumix_src;
	char *device;
	uint16_t speaker_id;
};

struct ausrc_st {
	struct ausrc_prm prm;
	ausrc_read_h *rh;
	struct auplay_st *st_play;
	void *arg;
	struct amix *amix;
};

struct auplay_st {
	struct auplay_prm prm;
	auplay_write_h *wh;
	int16_t *sampv;
	uint64_t ts;
	void *arg;
	struct amix *amix;
};

static struct ausrc *ausrc   = NULL;
static struct auplay *auplay = NULL;
static struct hash *amixl    = NULL;
static struct aumix *aumix   = NULL;


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


static void amix_destructor(void *arg)
{
	struct amix *amix = arg;

	mem_deref(amix->aumix_src);
	hash_unlink(&amix->le);
	list_unlink(&amix->sle);
	mem_deref(amix->device);
}


static void mix_handler(const int16_t *sampv, size_t sampc, void *arg)
{
	struct amix *amix = arg;
	struct auframe af;

	if (!amix || !amix->src || !amix->play)
		return;

	auframe_init(&af, amix->src->prm.fmt, (int16_t *)sampv, sampc,
		     amix->src->prm.srate, amix->src->prm.ch);
	af.timestamp = amix->play->ts;

	amix->src->rh(&af, amix->src->arg);
}


static void mix_readh(struct auframe *af, void *arg)
{
	struct amix *amix = arg;

	if (!amix || !af || !amix->play)
		return;

	amix->play->wh(af, amix->play->arg);
	af->id = amix->speaker_id;

#if 0
	re_atomic_rlx_set(&amix->play->level,
			  amix_level(af->sampv, af->sampc / CH));
#endif
}


static int amix_alloc(struct amix **amixp, const char *device)
{
	struct amix *amix;
	int err;

	if (!amixp)
		return EINVAL;

	amix = mem_zalloc(sizeof(struct amix), amix_destructor);
	if (!amix)
		return ENOMEM;

	err = str_dup(&amix->device, device);
	if (err) {
		err = ENOMEM;
		goto out;
	}

	err = aumix_source_alloc(&amix->aumix_src, aumix, mix_handler, amix);
	if (err) {
		err = ENOMEM;
		goto out;
	}

	struct pl *id = pl_alloc_str(device);
	if (!id) {
		err = ENOMEM;
		goto out;
	}
	aumix_source_set_id(amix->aumix_src, id);
	mem_deref(id);

	aumix_source_readh(amix->aumix_src, mix_readh);

out:
	if (err) {
		mem_deref(amix);
		return err;
	}

	*amixp = amix;
	hash_append(amixl, hash_joaat_str(amix->device), &amix->le, amix);

	return 0;
}


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	if (!st)
		return;

	if (st->amix)
		st->amix->src = NULL;

	aumix_source_enable(st->amix->aumix_src, false);

	mem_deref(st->amix);
}


static bool dev_cmp_h(struct le *le, void *arg)
{
	struct amix *amix = le->data;

	return 0 == str_cmp(amix->device, arg);
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	struct amix *amix = NULL;
	struct le *le;
	int err = 0;

	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->prm = *prm;
	st->rh	= rh;
	st->arg = arg;


	/* setup if auplay is started before ausrc */
	le = hash_lookup(amixl, hash_joaat_str(device), dev_cmp_h,
			 (void *)device);
	if (le) {
		amix = le->data;

		st->amix = mem_ref(amix);

		aumix_source_enable(amix->aumix_src, true);

		warning("debug %H\n", aumix_debug, aumix);

		goto out;
	}

	err = amix_alloc(&amix, device);
	if (err)
		goto out;

	st->amix = amix;

out:
	if (err)
		mem_deref(st);
	else {
		amix->src = st;
		*stp	  = st;
	}

	return err;
}


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	aumix_source_enable(st->amix->aumix_src, false);

	mem_deref(st->sampv);
	if (st->amix)
		st->amix->play = NULL;
	mem_deref(st->amix);
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	struct amix *amix = NULL;
	struct le *le;
	int err = 0;

	if (!stp || !ap || !prm)
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

	st->prm = *prm;
	st->wh	= wh;
	st->arg = arg;

	/* setup if ausrc is started before auplay */
	le = hash_lookup(amixl, hash_joaat_str(device), dev_cmp_h,
			 (void *)device);
	if (le) {
		amix = le->data;

		st->amix = mem_ref(amix);

		aumix_source_enable(amix->aumix_src, true);

		goto out;
	}

	err = amix_alloc(&amix, device);
	if (err)
		goto out;

	st->amix = amix;

out:
	if (err)
		mem_deref(st);
	else {
		amix->play = st;
		*stp	   = st;
	}

	return err;
}


static void driver_mixh(const int16_t *sampv, size_t sampc, void *arg)
{
	struct slaudio *a = arg;

	struct auframe af = {.sampv = (void *)sampv,
			     .sampc = sampc,
			     .srate = SRATE,
			     .ch    = CH,
			     .fmt   = AUFMT_S16LE};

	aubuf_write_auframe(a->ab_mix, &af);
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

	if (af->fmt != AUFMT_S16LE || af->ch != CH) {
		warning("ausrc wrong format\n");
		return;
	}

	auconv_from_s16(AUFMT_FLOAT, sampv, af->sampv, af->sampc);
	sl_meter_process(0, sampv, af->sampc / CH);

	aumix_source_put(a->mix_src, af->sampv, af->sampc);
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

	aubuf_flush(a->ab_mix);

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

	a->auplay_prm.srate = SRATE;
	a->auplay_prm.ch    = CH;
	a->auplay_prm.ptime = PTIME;
	a->auplay_prm.fmt   = AUFMT_S16LE;

	a->ausrc_prm.srate = SRATE;
	a->ausrc_prm.ch	   = CH;
	a->ausrc_prm.ptime = PTIME;
	a->ausrc_prm.fmt   = AUFMT_S16LE;

	driver_alloc(a);

	err = aubuf_alloc(&a->ab_mix,
			  (SRATE * CH * PTIME) / 1000 * sizeof(int16_t),
			  (SRATE * CH * PTIME) / 1000 * sizeof(int16_t) * 10);
	if (err)
		goto out;

	aubuf_set_live(a->ab_mix, true);

	struct pl *id = pl_alloc_str("slaudio_mix");
	if (!id) {
		err = ENOMEM;
		goto out;
	}

	aubuf_set_id(a->ab_mix, id);
	id = mem_deref(id);

	err = aumix_source_alloc(&a->mix_src, aumix, driver_mixh, a);
	if (err)
		goto out;

	id = pl_alloc_str("slaudio_source");
	if (!id) {
		err = ENOMEM;
		goto out;
	}

	aumix_source_set_id(a->mix_src, id);
	id = mem_deref(id);

	aumix_source_enable(a->mix_src, true);

out:
	if (err) {
		mem_deref(a);
		warning("sl_audio_alloc: failed %m\n", err);
	}
	else
		*audiop = a;

	return err;
}


void sl_audio_mute(struct slaudio *audio, bool mute)
{
	aumix_source_mute(audio->mix_src, mute);
}


int sl_audio_init(void)
{
	struct sl_config *conf = sl_conf();
	int err;

	hash_alloc(&amixl, 32);

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

	err = aumix_alloc(&aumix, SRATE, CH, PTIME);

	return err;
}


int sl_audio_close(void)
{
	amixl  = mem_deref(amixl);
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);
	aumix  = mem_deref(aumix);

	return 0;
}
