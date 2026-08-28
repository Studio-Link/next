/**
 * @file resample.c Arbitrary ratio samplerate conversion
 *
 * libre's auresamp only handles integer ratios, which rules out 44.1kHz
 * hosts and, more importantly, the tiny fractional ratios that clock drift
 * correction needs. libsamplerate is used instead. src_process() does not
 * allocate once the state exists, which keeps this realtime safe.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#include <string.h>
#include <samplerate.h>
#include <studiolink.h>


struct sl_rsmp {
	SRC_STATE *st;	    /**< NULL when no conversion is needed  */
	double ratio;	    /**< orate / irate                      */
	unsigned ch;	    /**< Interleaved channels               */
	float *stage;	    /**< Input not yet consumed by src      */
	size_t stage_frames;
	size_t stage_max;
};


static void rsmp_destructor(void *arg)
{
	struct sl_rsmp *r = arg;

	if (r->st)
		src_delete(r->st);

	mem_deref(r->stage);
}


int sl_rsmp_alloc(struct sl_rsmp **rsmpp, unsigned ch, uint32_t irate,
		  uint32_t orate, size_t in_max)
{
	struct sl_rsmp *r;
	int err = 0;
	int serr;

	if (!rsmpp || !ch || !irate || !orate || !in_max)
		return EINVAL;

	r = mem_zalloc(sizeof(*r), rsmp_destructor);
	if (!r)
		return ENOMEM;

	r->ch	   = ch;
	r->ratio   = (double)orate / (double)irate;
	r->stage_max = in_max;

	r->stage = mem_zalloc(in_max * ch * sizeof(float), NULL);
	if (!r->stage) {
		err = ENOMEM;
		goto out;
	}

	/* always allocate: the ratio can be steered at runtime, even when
	   the nominal rates are identical */
	r->st = src_new(SRC_SINC_FASTEST, (int)ch, &serr);
	if (!r->st) {
		warning("sl_rsmp: src_new failed: %s\n", src_strerror(serr));
		err = ENOMEM;
		goto out;
	}

out:
	if (err)
		mem_deref(r);
	else
		*rsmpp = r;

	return err;
}


void sl_rsmp_reset(struct sl_rsmp *rsmp)
{
	if (!rsmp)
		return;

	rsmp->stage_frames = 0;

	if (rsmp->st)
		src_reset(rsmp->st);
}


void sl_rsmp_set_ratio(struct sl_rsmp *rsmp, double ratio)
{
	if (!rsmp || ratio <= 0.0)
		return;

	/* src_process() interpolates towards the new ratio across the
	   block, so stepping it here does not click */
	rsmp->ratio = ratio;
}


double sl_rsmp_get_ratio(const struct sl_rsmp *rsmp)
{
	return rsmp ? rsmp->ratio : 1.0;
}


size_t sl_rsmp_staged(const struct sl_rsmp *rsmp)
{
	return rsmp ? rsmp->stage_frames : 0;
}


size_t sl_rsmp_process(struct sl_rsmp *rsmp, const float *in, size_t in_frm,
		       float *out, size_t out_max)
{
	SRC_DATA sd;
	size_t n, rest;

	if (!rsmp || !out || !out_max)
		return 0;

	/* stage the new input behind whatever src did not consume yet */
	if (in && in_frm) {
		n = rsmp->stage_max - rsmp->stage_frames;
		if (in_frm < n)
			n = in_frm;

		memcpy(rsmp->stage + rsmp->stage_frames * rsmp->ch, in,
		       n * rsmp->ch * sizeof(float));
		rsmp->stage_frames += n;
	}

	if (!rsmp->stage_frames)
		return 0;

	memset(&sd, 0, sizeof(sd));
	sd.data_in	 = rsmp->stage;
	sd.input_frames	 = (long)rsmp->stage_frames;
	sd.data_out	 = out;
	sd.output_frames = (long)out_max;
	sd.src_ratio	 = rsmp->ratio;
	sd.end_of_input	 = 0;

	if (src_process(rsmp->st, &sd))
		return 0;

	n    = (size_t)sd.input_frames_used;
	rest = rsmp->stage_frames - n;
	if (rest)
		memmove(rsmp->stage, rsmp->stage + n * rsmp->ch,
			rest * rsmp->ch * sizeof(float));
	rsmp->stage_frames = rest;

	return (size_t)sd.output_frames_gen;
}
