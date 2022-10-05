#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <studiolink.h>


struct slaudio_device {
	struct le le;
	int idx;
	char *name;
	uint16_t in_channels;
	uint16_t out_channels;
	uint32_t srate;
};

struct remote_track {
	struct le le;
	struct sl_track *track;
	struct ausrc_st *ausrc_st;
	struct auplay_st *auplay_st;
};

struct slaudio {
	struct le le;
	struct sl_track *local_track;
	struct list remotel;
	struct list devicel; /**< slaudio_device list */
	struct slaudio_device *src;
	struct slaudio_device *play;
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
}


int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track)
{
	struct slaudio *audio;

	if (!audiop || !track)
		return EINVAL;

	audio = mem_zalloc(sizeof(*audio), slaudio_destructor);
	if (!audio)
		return ENOMEM;

	audio->local_track = track;

	list_append(&local_tracks, &audio->le, audio);

	*audiop = audio;

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
