/**
 * @file engine.c Shared StudioLink engine
 *
 * libsl keeps global state - one libre main loop, one HTTP server, one
 * track list - so every plugin instance in a host process shares a single
 * engine. It is reference counted and runs re_main() on its own thread.
 *
 * Control commands from the DAW (GUI or audio thread) are marshalled onto
 * the StudioLink thread through an mqueue, because libre objects must only
 * be touched from there.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#include <studiolink.h>
#include <sl_vst.h>

enum { CMD_QUIT = 1000 };

static once_flag engine_once = ONCE_FLAG_INIT;

static struct {
	mtx_t mtx;
	cnd_t cnd;
	thrd_t thrd;
	struct mqueue *mq;
	int refs;
	bool running; /**< thread created            */
	bool ready;   /**< startup handshake done    */
	int err;      /**< startup error             */
} e;


static void engine_init(void)
{
	mtx_init(&e.mtx, mtx_plain);
	cnd_init(&e.cnd);
}


static void cmd_handler(int id, void *data, void *arg)
{
	intptr_t value	       = (intptr_t)data;
	struct sl_track *track = sl_track_by_id(1);
	(void)arg;

	switch (id) {

	case SL_VST_CMD_MUTE:
		if (!track || track->muted == (bool)value)
			break;

		track->muted = (bool)value;
		sl_audio_mute(sl_track_audio(track), track->muted);
		sl_track_ws_send();
		break;

	case SL_VST_CMD_RECORD:
		if ((sl_record_msecs() > 0) == (bool)value)
			break;

		sl_record_toggle();
		break;

	case SL_VST_CMD_WEBUI:
		(void)sl_open_webui();
		break;

	case CMD_QUIT:
		re_cancel();
		break;

	default:
		warning("sl_vst: unknown command %d\n", id);
		break;
	}
}


/**
 * Open the "vst" driver on the local track.
 *
 * libsl only starts the hardware driver once a device has been selected,
 * which normally happens from the web frontend. The plugin has exactly one
 * device (index 0, "DAW"), so select it right away.
 */
static int audio_start(void)
{
	struct sl_track *track;
	struct slaudio *audio;

	track = sl_track_by_id(1);
	if (!track)
		return ENOENT;

	audio = sl_track_audio(track);
	if (!audio)
		return ENOENT;

	return sl_audio_set_device(audio, 0, 0);
}


static void handshake(int err)
{
	mtx_lock(&e.mtx);
	e.err	= err;
	e.ready = true;
	cnd_signal(&e.cnd);
	mtx_unlock(&e.mtx);
}


static int engine_thread(void *arg)
{
	int err;
	(void)arg;

	err = sl_baresip_init(NULL);
	if (err) {
		warning("sl_vst: baresip init failed (%m)\n", err);
		goto fail;
	}

	/* must happen before sl_init(), the local track picks its driver
	   during sl_tracks_init() */
	err = sl_vst_driver_register();
	if (err) {
		warning("sl_vst: driver register failed (%m)\n", err);
		goto fail;
	}

	err = sl_init();
	if (err) {
		warning("sl_vst: init failed (%m)\n", err);
		goto fail;
	}

	err = mqueue_alloc(&e.mq, cmd_handler, NULL);
	if (err)
		goto fail;

	err = audio_start();
	if (err) {
		warning("sl_vst: audio start failed (%m)\n", err);
		goto fail;
	}

	handshake(0);

	(void)sl_main();

	e.mq = mem_deref(e.mq);
	sl_vst_driver_unregister();
	sl_close();

	return 0;

fail:
	e.mq = mem_deref(e.mq);
	sl_vst_driver_unregister();
	sl_close();
	handshake(err);

	return err;
}


int sl_vst_engine_ref(void)
{
	int err = 0;

	call_once(&engine_once, engine_init);

	mtx_lock(&e.mtx);

	if (e.refs > 0) {
		++e.refs;
		err = 0;
		goto out;
	}

	e.ready = false;
	e.err	= 0;

	if (thrd_create(&e.thrd, engine_thread, NULL) != thrd_success) {
		err = ENOMEM;
		goto out;
	}

	e.running = true;

	while (!e.ready)
		cnd_wait(&e.cnd, &e.mtx);

	err = e.err;

	if (err) {
		mtx_unlock(&e.mtx);
		thrd_join(e.thrd, NULL);
		mtx_lock(&e.mtx);
		e.running = false;
		goto out;
	}

	e.refs = 1;

out:
	mtx_unlock(&e.mtx);

	return err;
}


void sl_vst_engine_deref(void)
{
	bool stop = false;

	mtx_lock(&e.mtx);

	if (e.refs > 0 && --e.refs == 0)
		stop = e.running;

	mtx_unlock(&e.mtx);

	if (!stop)
		return;

	if (e.mq)
		(void)mqueue_push(e.mq, CMD_QUIT, NULL);

	thrd_join(e.thrd, NULL);

	mtx_lock(&e.mtx);
	e.running = false;
	e.ready	  = false;
	mtx_unlock(&e.mtx);
}


bool sl_vst_engine_running(void)
{
	bool running;

	mtx_lock(&e.mtx);
	running = e.running && e.ready && !e.err;
	mtx_unlock(&e.mtx);

	return running;
}


int sl_vst_engine_cmd(enum sl_vst_cmd cmd, int32_t value)
{
	if (!e.mq)
		return ENOENT;

	return mqueue_push(e.mq, (int)cmd, (void *)(intptr_t)value);
}
