#include <string.h>
#include <math.h>
#include <studiolink.h>

#define MAX_METERS 32

static float bias		    = 1.0f;
static float peaks[MAX_METERS]	    = {0};
static float sent_peaks[MAX_METERS] = {0};
static mtx_t *mutex = NULL;

static struct tmr tmr = {.le = LE_INIT};


void sl_meter_process(unsigned int ch, float *in, unsigned long nframes)
{
	mtx_lock(mutex);
	for (unsigned int i = ch; i < nframes; i = i + 2) {
		const float s = fabs(in[i]);
		if (s > peaks[ch]) {
			peaks[ch] = s;
		}
	}
	mtx_unlock(mutex);
}


static void write_ws(void)
{
	unsigned long n;
	int i;
	float db;
	char one_peak[100];
	char p[2048];

	p[0] = '\0';

	/* Record time */
	re_snprintf(one_peak, sizeof(one_peak), "%llu ", sl_record_msecs());
	strcat((char *)p, one_peak);

	for (i = 0; i < MAX_METERS; i++) {
		db = 20.0f * log10f(sent_peaks[i] * bias);
		re_snprintf(one_peak, sizeof(one_peak), "%f ", db);
		strcat((char *)p, one_peak);
	}

	n	 = strlen(p);
	p[n - 1] = '\0'; /* remove trailing space */

	sl_ws_send_str(WS_METERS, p);
}


static void tmr_handler(void *arg)
{
	(void)arg;
	tmr_start(&tmr, 100, tmr_handler, NULL);

	mtx_lock(mutex);
	memcpy(sent_peaks, peaks, sizeof(peaks));
	memset(peaks, 0, sizeof(peaks));
	mtx_unlock(mutex);

	write_ws();
}


void sl_meter_init(void)
{
	mutex_alloc(&mutex);
	tmr_init(&tmr);
	tmr_start(&tmr, 150, tmr_handler, NULL);
}


void sl_meter_close(void)
{
	mutex = mem_deref(mutex);
	tmr_cancel(&tmr);
}
