/**
 * @file mock/mock_auplay.c Mock audio player
 *
 * Copyright (C) 2010 - 2016 Alfred E. Heggestad
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "../test.h"


struct ausrc_st {
	struct tmr tmr;
	struct ausrc_prm prm;
	void *sampv;
	size_t sampc;
	ausrc_read_h *rh;
	void *arg;
};


static struct {
	mock_sample_h *sampleh;
	void *arg;
} mock;


static void auplay_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	tmr_cancel(&st->tmr);
	mem_deref(st->sampv);
}


static void tmr_handler(void *arg)
{
	struct ausrc_st *st = arg;
	struct auframe af;

	tmr_start(&st->tmr, st->prm.ptime, tmr_handler, st);

	auframe_init(&af, st->prm.fmt, st->sampv, st->sampc, st->prm.srate,
		     st->prm.ch);

	if (st->rh)
		st->rh(&af, st->arg);

	/* feed the audio-samples back to the test */
	if (mock.sampleh)
		mock.sampleh(st->sampv, st->sampc, mock.arg);
}


static int mock_ausrc_alloc(struct ausrc_st **stp, const struct ausrc *as,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err = 0;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->prm  = *prm;
	st->rh   = rh;
	st->arg  = arg;

	st->sampc = prm->srate * prm->ch * prm->ptime / 1000;

	st->sampv = mem_zalloc(aufmt_sample_size(prm->fmt) * st->sampc, NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	tmr_start(&st->tmr, 0, tmr_handler, st);

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


int mock_ausrc_register(struct ausrc **ausrcp, struct list *ausrcl,
			 mock_sample_h *sampleh, void *arg)
{
	mock.sampleh = sampleh;
	mock.arg = arg;

	return ausrc_register(ausrcp, ausrcl,
			      "mock-auplay", mock_ausrc_alloc);
}
