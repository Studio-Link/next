/**
 * @file io.c Per plugin instance audio bridge
 *
 * Converts between the DAW world (deinterleaved float, host samplerate,
 * host block size) and the StudioLink world (interleaved S16LE, 48kHz,
 * stereo, 20ms frames). Everything here runs on the realtime audio thread
 * and must not allocate, lock or log.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#include <string.h>
#include <studiolink.h>
#include <sl_vst.h>

enum { SLACK = 8 }; /**< frames of headroom for rounding and filter delay */

struct sl_vst_io {
	uint32_t srate;	   /**< DAW samplerate            */
	size_t maxframes;  /**< DAW max. block size       */
	size_t slmax;	   /**< Max. 48kHz frames / block */
	bool owner;	   /**< Carries audio?            */

	struct sl_rsmp *rs_in;	/**< DAW rate -> 48kHz */
	struct sl_rsmp *rs_out; /**< 48kHz -> DAW rate */

	float *fl_daw;	  /**< Interleaved float at DAW rate */
	float *fl_sl;	  /**< Interleaved float at 48kHz    */
	int16_t *s16_sl;  /**< Interleaved S16 at 48kHz      */
};


static void io_destructor(void *arg)
{
	struct sl_vst_io *io = arg;

	if (io->owner)
		sl_vst_driver_release();

	mem_deref(io->rs_in);
	mem_deref(io->rs_out);
	mem_deref(io->fl_daw);
	mem_deref(io->fl_sl);
	mem_deref(io->s16_sl);
}


int sl_vst_io_alloc(struct sl_vst_io **iop, uint32_t srate, size_t maxframes)
{
	struct sl_vst_io *io;
	int err = 0;

	if (!iop || !srate || !maxframes)
		return EINVAL;

	io = mem_zalloc(sizeof(*io), io_destructor);
	if (!io)
		return ENOMEM;

	io->srate     = srate;
	io->maxframes = maxframes;
	io->slmax = (size_t)((double)maxframes * (double)SL_VST_SRATE /
			     (double)srate) +
		    SLACK;

	io->fl_daw = mem_zalloc(maxframes * SL_VST_CH * sizeof(float), NULL);
	io->fl_sl  = mem_zalloc(io->slmax * SL_VST_CH * sizeof(float), NULL);
	io->s16_sl = mem_zalloc(io->slmax * SL_VST_CH * sizeof(int16_t),
				NULL);
	if (!io->fl_daw || !io->fl_sl || !io->s16_sl) {
		err = ENOMEM;
		goto out;
	}

	err = sl_rsmp_alloc(&io->rs_in, SL_VST_CH, srate, SL_VST_SRATE,
			    maxframes + SLACK);
	if (err)
		goto out;

	err = sl_rsmp_alloc(&io->rs_out, SL_VST_CH, SL_VST_SRATE, srate,
			    io->slmax + SLACK);
	if (err)
		goto out;

	io->owner = sl_vst_driver_claim();

	if (io->owner)
		info("sl_vst: audio bridge active (%u Hz, %zu frames)\n",
		     srate, maxframes);
	else
		info("sl_vst: further instance, staying in bypass\n");

out:
	if (err)
		mem_deref(io);
	else
		*iop = io;

	return err;
}


void sl_vst_io_free(struct sl_vst_io *io)
{
	mem_deref(io);
}


bool sl_vst_io_owner(const struct sl_vst_io *io)
{
	return io ? io->owner : false;
}


static void silence(float *const *out, unsigned ch_out, size_t nframes)
{
	unsigned ch;

	for (ch = 0; ch < ch_out; ch++) {
		if (out[ch])
			memset(out[ch], 0, nframes * sizeof(float));
	}
}


/* deinterleaved DAW channels -> interleaved stereo */
static void interleave(float *dst, const float *const *in, unsigned ch_in,
		       size_t nframes)
{
	const float *l, *r;
	size_t i;

	l = (ch_in > 0) ? in[0] : NULL;
	r = (ch_in > 1) ? in[1] : l; /* mono in feeds both sides */

	for (i = 0; i < nframes; i++) {
		dst[i * 2]     = l ? l[i] : 0.0f;
		dst[i * 2 + 1] = r ? r[i] : 0.0f;
	}
}


/* interleaved stereo -> deinterleaved DAW channels */
static void deinterleave(float *const *out, unsigned ch_out, const float *src,
			 size_t nframes)
{
	unsigned ch;
	size_t i;

	for (ch = 0; ch < ch_out; ch++) {

		if (!out[ch])
			continue;

		/* extra channels beyond stereo stay silent */
		if (ch >= SL_VST_CH) {
			memset(out[ch], 0, nframes * sizeof(float));
			continue;
		}

		for (i = 0; i < nframes; i++)
			out[ch][i] = src[i * SL_VST_CH + ch];
	}
}


void sl_vst_io_process(struct sl_vst_io *io, const float *const *in,
		       float *const *out, unsigned ch_in, unsigned ch_out,
		       size_t nframes)
{
	size_t n, need, staged;

	if (!out || !ch_out || !nframes)
		return;

	/* only one instance carries audio, and only up to the block size
	   we were set up for */
	if (!io || !io->owner || nframes > io->maxframes) {
		silence(out, ch_out, nframes);
		return;
	}

	/* ---- DAW input -> StudioLink ---- */
	if (in && ch_in) {
		interleave(io->fl_daw, in, ch_in, nframes);

		n = sl_rsmp_process(io->rs_in, io->fl_daw, nframes, io->fl_sl,
				    io->slmax);
		if (n) {
			auconv_to_s16(io->s16_sl, AUFMT_FLOAT, io->fl_sl,
				      n * SL_VST_CH);
			sl_vst_driver_write(io->s16_sl, n * SL_VST_CH);
		}
	}

	/* how many 48kHz frames the output resampler still needs */
	need = (size_t)((double)nframes * (double)SL_VST_SRATE /
			(double)io->srate) +
	       2;
	if (need > io->slmax)
		need = io->slmax;

	staged = sl_rsmp_staged(io->rs_out);
	need   = (need > staged) ? need - staged : 0;

	sl_vst_driver_pump(need * SL_VST_CH);

	/* ---- StudioLink -> DAW output ---- */
	if (need) {
		sl_vst_driver_read(io->s16_sl, need * SL_VST_CH);
		auconv_to_float(io->fl_sl, AUFMT_S16LE, io->s16_sl,
				need * SL_VST_CH);
	}

	n = sl_rsmp_process(io->rs_out, need ? io->fl_sl : NULL, need,
			    io->fl_daw, nframes);

	/* startup transient of the sinc resampler */
	if (n < nframes)
		memset(io->fl_daw + n * SL_VST_CH, 0,
		       (nframes - n) * SL_VST_CH * sizeof(float));

	deinterleave(out, ch_out, io->fl_daw, nframes);
}
