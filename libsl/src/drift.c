/**
 * @file drift.c Clock drift estimation and correction
 *
 * The audio mixer (aumix) is paced by the OS monotonic clock, the sound
 * card by its own crystal. The two never agree exactly - a few hundred ppm
 * apart is normal - so the buffer between them slowly fills or empties
 * until aubuf drops or zero fills a whole 20ms frame. That is an audible
 * click every couple of minutes, on a timer.
 *
 * This is a control loop that watches the fill level of that buffer and
 * returns a resample ratio which cancels the drift.
 *
 * The plant is an integrator: buffer fill is the integral of the rate
 * difference between producer and consumer. A proportional controller is
 * therefore enough to fully cancel a constant rate offset - the price is a
 * small standing offset in fill level, where
 *
 *     kp * offset = drift
 *
 * With the gain below, 200ppm of drift parks the buffer 10ms away from its
 * target, which is well inside the 40ms..200ms that ab_mix provides. No
 * integral term is used on purpose: it would remove that offset but adds
 * windup, and the offset is not what costs us anything here.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#include <studiolink.h>

/**
 * Loop gain in 1/s. Also the reciprocal of the closed loop time constant,
 * so 0.02 means the loop settles over ~50s. Slow is deliberate: the
 * correction is a pitch shift, and it must stay far below anything a
 * listener could hear as wow or flutter.
 */
static const double drift_kp = 0.02;

/** Measurement filter time constant in s. The fill level is quantised to
 *  whole mixer frames, so the raw signal jitters by tens of ms. Well
 *  separated from the loop time constant to keep the loop stable. */
static const double drift_tau = 8.0;

/** Hard limit of the correction. Two devices at the usual +/-100ppm cannot
 *  exceed this; if we hit the clamp something else is wrong and dropping a
 *  frame is the honest fallback. */
static const double drift_max = 0.001; /* 1000 ppm */

/** Do not steer until the buffer has settled after start or a flush. */
enum { DRIFT_WARMUP_MS = 3000, DRIFT_DT_MAX_MS = 1000 };


struct sl_drift {
	double target;	 /**< Wanted fill level in seconds      */
	double bps;	 /**< Bytes per second                  */
	double avg;	 /**< Filtered fill error in seconds     */
	double ratio;	 /**< Correction, 1.0 means no drift     */
	uint64_t ts;	 /**< Last update                        */
	uint64_t start;	 /**< First update after reset           */
	bool init;
};


int sl_drift_alloc(struct sl_drift **driftp, uint32_t srate, uint8_t ch,
		   size_t target_sz)
{
	struct sl_drift *d;

	if (!driftp || !srate || !ch || !target_sz)
		return EINVAL;

	d = mem_zalloc(sizeof(*d), NULL);
	if (!d)
		return ENOMEM;

	d->bps	  = (double)srate * (double)ch * (double)sizeof(int16_t);
	d->target = (double)target_sz / d->bps;
	d->ratio  = 1.0;

	*driftp = d;

	return 0;
}


void sl_drift_set_target(struct sl_drift *d, size_t target_sz)
{
	double target;

	if (!d || !target_sz)
		return;

	target = (double)target_sz / d->bps;

	if (target == d->target)
		return;

	/* keep the filtered error consistent with the new setpoint instead
	   of making the loop chase a step */
	d->avg += d->target - target;
	d->target = target;
}


void sl_drift_reset(struct sl_drift *d)
{
	if (!d)
		return;

	d->init	 = false;
	d->avg	 = 0.0;
	d->ratio = 1.0;
}


void sl_drift_update(struct sl_drift *d, size_t cur_sz, uint64_t now)
{
	double err, dt, alpha, ratio;

	if (!d)
		return;

	err = (double)cur_sz / d->bps - d->target;

	if (!d->init) {
		/* seed the filter, do not ramp up from zero */
		d->avg	 = err;
		d->ts	 = now;
		d->start = now;
		d->init	 = true;
		return;
	}

	if (now <= d->ts)
		return;

	dt = (double)(now - d->ts) / 1000.0;
	d->ts = now;

	/* a long gap means the stream was stalled, not that it drifted */
	if (dt > (double)DRIFT_DT_MAX_MS / 1000.0) {
		sl_drift_reset(d);
		return;
	}

	/* first order lowpass, sample interval aware */
	alpha = dt / (drift_tau + dt);
	d->avg += alpha * (err - d->avg);

	if ((now - d->start) < DRIFT_WARMUP_MS)
		return;

	ratio = 1.0 + drift_kp * d->avg;

	if (ratio > 1.0 + drift_max)
		ratio = 1.0 + drift_max;
	else if (ratio < 1.0 - drift_max)
		ratio = 1.0 - drift_max;

	d->ratio = ratio;
}


double sl_drift_ratio(const struct sl_drift *d)
{
	return d ? d->ratio : 1.0;
}


double sl_drift_ppm(const struct sl_drift *d)
{
	return d ? (d->ratio - 1.0) * 1e6 : 0.0;
}


bool sl_drift_locked(const struct sl_drift *d)
{
	if (!d || !d->init)
		return false;

	return (d->ts - d->start) >= DRIFT_WARMUP_MS;
}
