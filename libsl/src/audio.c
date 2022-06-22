#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <studiolink.h>

/* TODO: allow multiple sl_audio objects */
static struct sl_audio {
	struct sl_track *track;
	struct ausrc *ausrc;
	struct auplay *auplay;
} slaudio = {NULL, NULL, NULL};

struct ausrc_st {
	struct tmr tmr;
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
	size_t num_bytes;
	auplay_write_h *wh;
	void *arg;
};


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
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

	/* TODO */

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
	int err = 0;
	(void)ap;
	(void)device;

	if (!prm || !wh)
		return EINVAL;

	if (!prm->ch || !prm->srate || !prm->ptime)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), NULL);
	if (!st)
		return ENOMEM;

	st->wh	= wh;
	st->arg = arg;
	st->prm = *prm;

	/* TODO */

	if (err)
		mem_deref(st);
	else if (stp)
		*stp = st;

	return err;
}


int sl_audio_init(void)
{
	int err;

	err = ausrc_register(&slaudio.ausrc, baresip_ausrcl(), "slaudio",
			     src_alloc);
	err |= auplay_register(&slaudio.auplay, baresip_auplayl(), "slaudio",
			       play_alloc);
	return err;
}


int sl_audio_close(void)
{
	return 0;
}
