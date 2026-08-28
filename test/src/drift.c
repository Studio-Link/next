/**
 * @file drift.c Clock drift controller tests
 *
 * Simulates a mixer producing 20ms frames on the OS clock against a sound
 * card consuming blocks on a crystal that is deliberately off by a known
 * amount, and checks that the controller cancels it.
 */

#include <math.h>
#include <re.h>
#include <baresip.h>
#include <studiolink.h>
#include "test.h"

enum { SRATE = 48000, CH = 2, PTIME = 20 };

#define FRAME_SZ ((SRATE * CH * PTIME / 1000) * (int)sizeof(int16_t))
#define BPS ((double)SRATE * CH * (double)sizeof(int16_t))


/**
 * Run the loop for a simulated duration
 *
 * @param ppm      Clock offset of the sound card
 * @param block    Device block size in frames
 * @param secs     Simulated seconds
 * @param ppm_out  Measured correction
 * @param min_ms   Lowest fill level seen after settling
 * @param max_ms   Highest fill level seen after settling
 *
 * @return 0 if success, otherwise errorcode
 */
static int simulate(double ppm, unsigned block, unsigned secs,
		    double *ppm_out, double *min_ms, double *max_ms)
{
	struct sl_drift *drift = NULL;
	double target = 2.0 * FRAME_SZ + (double)block * CH * 2;
	double fill = target;
	double hw_rate = SRATE * (1.0 + ppm * 1e-6);
	double dt_cb = (double)block / hw_rate;
	double t = 0.0, next_mix = 0.0, next_cb = 0.0;
	double lo = 1e18, hi = -1e18;
	int err;

	err = sl_drift_alloc(&drift, SRATE, CH, (size_t)target);
	if (err)
		return err;

	while (t < (double)secs) {

		if (next_mix <= next_cb) {
			t = next_mix;
			next_mix += (double)PTIME / 1000.0;
			fill += FRAME_SZ;
		}
		else {
			t = next_cb;
			next_cb += dt_cb;

			sl_drift_update(drift, (size_t)(fill < 0 ? 0 : fill),
					(uint64_t)(t * 1000.0));

			fill -= (double)block * CH * 2 *
				sl_drift_ratio(drift);

			if (fill < 0)
				fill = 0;
			if (fill > 10.0 * FRAME_SZ)
				fill = 10.0 * FRAME_SZ;
		}

		if (t > (double)secs * 0.6) {
			if (fill < lo)
				lo = fill;
			if (fill > hi)
				hi = fill;
		}
	}

	*ppm_out = sl_drift_ppm(drift);
	*min_ms	 = lo / BPS * 1000.0;
	*max_ms	 = hi / BPS * 1000.0;

	mem_deref(drift);

	return 0;
}


int test_sl_drift(void)
{
	static const double ppmv[] = {0, 25, -25, 100, -100, 200, -200};
	static const unsigned blockv[] = {64, 256, 1024, 2048};
	struct sl_drift *drift = NULL;
	double got, lo, hi;
	size_t i, b;
	int err;

	/* a fresh controller must not steer */
	err = sl_drift_alloc(&drift, SRATE, CH, 2 * FRAME_SZ);
	TEST_ERR(err);
	ASSERT_TRUE(sl_drift_ratio(drift) == 1.0);
	ASSERT_TRUE(!sl_drift_locked(drift));
	drift = mem_deref(drift);

	for (b = 0; b < RE_ARRAY_SIZE(blockv); b++) {

		for (i = 0; i < RE_ARRAY_SIZE(ppmv); i++) {

			err = simulate(ppmv[i], blockv[b], 600, &got, &lo,
				       &hi);
			TEST_ERR(err);

			/* the correction is the negative of the drift */
			if (fabs(got + ppmv[i]) > 15.0) {
				warning("drift: %f ppm block %u:"
					" correction %f ppm\n",
					ppmv[i], blockv[b], got);
				err = EINVAL;
				goto out;
			}

			/* buffer must stay clear of both ends of aubuf */
			if (lo < 2.0 || hi > 190.0) {
				warning("drift: %f ppm block %u:"
					" fill %f..%f ms\n",
					ppmv[i], blockv[b], lo, hi);
				err = EINVAL;
				goto out;
			}
		}
	}

out:
	mem_deref(drift);

	return err;
}
