/**
 * @file driver.c baresip audio driver "vst"
 *
 * Replaces the portaudio hardware driver of the local StudioLink track, so
 * that the DAW becomes the audio device. The DAW audio thread pushes its
 * input into ab_in, pulls the StudioLink mix out of ab_out and drives the
 * 20ms frame exchange with baresip in between.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#include <string.h>
#include <studiolink.h>
#include <sl_vst.h>

/* One 20ms frame: 48000 * 2 * 20 / 1000 = 1920 samples */
enum {
	FRAME_SAMPC = SL_VST_SRATE * SL_VST_CH * SL_VST_PTIME / 1000,
	FRAME_SZ    = FRAME_SAMPC * sizeof(int16_t),
	PREBUF_SZ   = FRAME_SZ * 2,  /**< target fill of ab_out      */
	MAX_SZ	    = FRAME_SZ * 16, /**< ~320ms hard limit          */
	TS_STEP	    = SL_VST_PTIME * 1000, /**< AUDIO_TIMEBASE units */
};

struct ausrc_st {
	struct ausrc_prm prm;
	ausrc_read_h *rh;
	void *arg;
};

struct auplay_st {
	struct auplay_prm prm;
	auplay_write_h *wh;
	void *arg;
};

static struct {
	struct ausrc *ausrc;
	struct auplay *auplay;

	/* owned by the StudioLink thread, read by the audio thread */
	struct ausrc_st *src_st;
	struct auplay_st *play_st;
	mtx_t mtx; /**< guards src_st/play_st against driver restarts */
	bool mtx_ready;

	struct aubuf *ab_in;  /**< DAW  -> StudioLink */
	struct aubuf *ab_out; /**< StudioLink -> DAW  */

	uint64_t ts_in;
	uint64_t ts_out;

	RE_ATOMIC bool claimed;
} d;


static void src_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	mtx_lock(&d.mtx);
	if (d.src_st == st)
		d.src_st = NULL;
	mtx_unlock(&d.mtx);
}


static int src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	if (prm->srate != SL_VST_SRATE || prm->ch != SL_VST_CH ||
	    prm->fmt != AUFMT_S16LE) {
		warning("sl_vst: ausrc unsupported format "
			"(%u Hz, %u ch, fmt %d)\n",
			prm->srate, prm->ch, prm->fmt);
		return ENOTSUP;
	}

	st = mem_zalloc(sizeof(*st), src_destructor);
	if (!st)
		return ENOMEM;

	st->prm = *prm;
	st->rh	= rh;
	st->arg = arg;

	mtx_lock(&d.mtx);
	d.src_st = st;
	d.ts_in	 = 0;
	mtx_unlock(&d.mtx);

	aubuf_flush(d.ab_in);

	info("sl_vst: ausrc started\n");

	*stp = st;

	return 0;
}


static void play_destructor(void *arg)
{
	struct auplay_st *st = arg;

	mtx_lock(&d.mtx);
	if (d.play_st == st)
		d.play_st = NULL;
	mtx_unlock(&d.mtx);
}


static int play_alloc(struct auplay_st **stp, const struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	(void)device;

	if (!stp || !ap || !prm)
		return EINVAL;

	if (prm->srate != SL_VST_SRATE || prm->ch != SL_VST_CH ||
	    prm->fmt != AUFMT_S16LE) {
		warning("sl_vst: auplay unsupported format "
			"(%u Hz, %u ch, fmt %d)\n",
			prm->srate, prm->ch, prm->fmt);
		return ENOTSUP;
	}

	st = mem_zalloc(sizeof(*st), play_destructor);
	if (!st)
		return ENOMEM;

	st->prm = *prm;
	st->wh	= wh;
	st->arg = arg;

	mtx_lock(&d.mtx);
	d.play_st = st;
	d.ts_out  = 0;
	mtx_unlock(&d.mtx);

	aubuf_flush(d.ab_out);

	info("sl_vst: auplay started\n");

	*stp = st;

	return 0;
}


static int mediadev_register(void)
{
	struct mediadev *dev;
	int err;

	err = mediadev_add(&d.ausrc->dev_list, "DAW");
	if (err)
		return err;

	dev = mediadev_find(&d.ausrc->dev_list, "DAW");
	if (!dev)
		return ENODATA;

	dev->device_index  = 0;
	dev->src.channels  = SL_VST_CH;
	dev->src.is_default = true;

	err = mediadev_add(&d.auplay->dev_list, "DAW");
	if (err)
		return err;

	dev = mediadev_find(&d.auplay->dev_list, "DAW");
	if (!dev)
		return ENODATA;

	dev->device_index    = 0;
	dev->play.channels   = SL_VST_CH;
	dev->play.is_default = true;

	return 0;
}


int sl_vst_driver_register(void)
{
	struct sl_config *conf = sl_conf();
	int err;

	if (!conf)
		return EINVAL;

	if (d.ausrc || d.auplay)
		return EALREADY;

	if (mtx_init(&d.mtx, mtx_plain) != thrd_success)
		return ENOMEM;

	d.mtx_ready = true;

	err = aubuf_alloc(&d.ab_in, 0, MAX_SZ);
	err |= aubuf_alloc(&d.ab_out, FRAME_SZ, MAX_SZ);
	if (err)
		goto out;

	/* drop stale audio instead of drifting when the DAW stalls */
	aubuf_set_live(d.ab_in, true);
	aubuf_set_live(d.ab_out, true);

	err = ausrc_register(&d.ausrc, baresip_ausrcl(), "vst", src_alloc);
	err |= auplay_register(&d.auplay, baresip_auplayl(), "vst",
			       play_alloc);
	if (err)
		goto out;

	err = mediadev_register();
	if (err)
		goto out;

	/* the local track now opens "vst" instead of "portaudio" */
	str_ncpy(conf->play.mod, "vst", sizeof(conf->play.mod));
	str_ncpy(conf->src.mod, "vst", sizeof(conf->src.mod));

	info("sl_vst: audio driver registered\n");

out:
	if (err)
		sl_vst_driver_unregister();

	return err;
}


void sl_vst_driver_unregister(void)
{
	d.ausrc	 = mem_deref(d.ausrc);
	d.auplay = mem_deref(d.auplay);
	d.ab_in	 = mem_deref(d.ab_in);
	d.ab_out = mem_deref(d.ab_out);

	if (d.mtx_ready) {
		mtx_destroy(&d.mtx);
		d.mtx_ready = false;
	}

	re_atomic_rlx_set(&d.claimed, false);
}


bool sl_vst_driver_claim(void)
{
	bool expected = false;

	return re_atomic_compare_exchange_strong(&d.claimed, &expected, true,
						 re_memory_order_acquire,
						 re_memory_order_relaxed);
}


void sl_vst_driver_release(void)
{
	re_atomic_rlx_set(&d.claimed, false);

	sl_vst_driver_flush();
}


void sl_vst_driver_flush(void)
{
	if (d.ab_in)
		aubuf_flush(d.ab_in);
	if (d.ab_out)
		aubuf_flush(d.ab_out);
}


void sl_vst_driver_write(const int16_t *sampv, size_t sampc)
{
	struct auframe af;

	if (!d.ab_in || !sampv || !sampc)
		return;

	auframe_init(&af, AUFMT_S16LE, (void *)sampv, sampc, SL_VST_SRATE,
		     SL_VST_CH);

	(void)aubuf_write_auframe(d.ab_in, &af);
}


void sl_vst_driver_read(int16_t *sampv, size_t sampc)
{
	struct auframe af;

	if (!sampv || !sampc)
		return;

	if (!d.ab_out) {
		memset(sampv, 0, sampc * sizeof(int16_t));
		return;
	}

	auframe_init(&af, AUFMT_S16LE, sampv, sampc, SL_VST_SRATE,
		     SL_VST_CH);

	/* zero fills on underrun */
	aubuf_read_auframe(d.ab_out, &af);
}


void sl_vst_driver_pump(size_t need_sampc)
{
	int16_t sampv[FRAME_SAMPC];
	struct auframe af;
	size_t target;

	if (!d.mtx_ready || !d.ab_in || !d.ab_out)
		return;

	/*
	 * Never block the DAW thread. A contended lock only happens while
	 * the StudioLink thread (re)starts the driver, aubuf zero fills for
	 * the few blocks this takes.
	 */
	if (mtx_trylock(&d.mtx) != thrd_success)
		return;

	/* DAW input -> baresip, one 20ms frame at a time */
	while (d.src_st && aubuf_cur_size(d.ab_in) >= FRAME_SZ) {

		auframe_init(&af, AUFMT_S16LE, sampv, FRAME_SAMPC,
			     SL_VST_SRATE, SL_VST_CH);
		aubuf_read_auframe(d.ab_in, &af);

		af.timestamp = d.ts_in;
		d.ts_in += TS_STEP;

		d.src_st->rh(&af, d.src_st->arg);
	}

	/* baresip -> DAW output, keep the block plus a prebuffer ready */
	target = need_sampc * sizeof(int16_t) + PREBUF_SZ;
	if (target > MAX_SZ)
		target = MAX_SZ;

	while (d.play_st && aubuf_cur_size(d.ab_out) < target) {

		memset(sampv, 0, sizeof(sampv));
		auframe_init(&af, AUFMT_S16LE, sampv, FRAME_SAMPC,
			     SL_VST_SRATE, SL_VST_CH);

		af.timestamp = d.ts_out;
		d.ts_out += TS_STEP;

		d.play_st->wh(&af, d.play_st->arg);

		if (aubuf_write_auframe(d.ab_out, &af))
			break;
	}

	mtx_unlock(&d.mtx);
}
