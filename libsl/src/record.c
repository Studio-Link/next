/**
 * @file record.c generic recording
 *
 * Copyright (C) 2026 Sebastian Reimers
 */

#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <studiolink.h>


static struct {
	struct list tracks;
	RE_ATOMIC bool run;
	thrd_t thread;
	struct aubuf *ab;
	char *folder;
	RE_ATOMIC uint64_t start_time;
} record = {.tracks = LIST_INIT, .run = false};

struct record_entry {
	struct le le;
	struct mbuf *mb;
	size_t size;
};

enum {
	SRATE = 48000,
	CH    = 2,
	PTIME = 20,
};


uint64_t sl_record_msecs(void)
{
	if (!re_atomic_rlx(&record.start_time))
		return 0;

	return tmr_jiffies() - re_atomic_rlx(&record.start_time);
}


struct track {
	struct le le;
	uint16_t id;
	char file[512];
	uint64_t last;
	struct flac *flac;
};


static void track_destruct(void *arg)
{
	struct track *track = arg;

	list_unlink(&track->le);
	mem_deref(track->flac);
}


static int record_track(struct auframe *af)
{
	struct le *le;
	struct track *track = NULL;
	uint64_t offset;
	int err;

	LIST_FOREACH(&record.tracks, le)
	{
		struct track *t = le->data;

		if (t->id == af->id) {
			track = t;
			break;
		}
	}

	if (!track) {
		track = mem_zalloc(sizeof(struct track), track_destruct);
		if (!track)
			return ENOMEM;

		track->id   = af->id;
		track->last = re_atomic_rlx(&record.start_time);

		/* TODO: use track name */
		re_snprintf(track->file, sizeof(track->file),
			    "%s/audio_id%u.flac", record.folder, track->id);

		err = sl_flac_init(&track->flac, af, track->file);
		if (err) {
			warning("sl_record: error flac_init (%m)\n", err);
			mem_deref(track);
			return err;
		}

		info("sl_record: add track (%s)\n", track->file);

		list_append(&record.tracks, &track->le, track);
	}

	offset	    = af->timestamp - track->last;
	track->last = af->timestamp;

	sl_flac_record(track->flac, af, offset);

	return 0;
}


static int record_thread(void *arg)
{
	struct auframe af;
	int err;
	(void)arg;

	int16_t *sampv;
	size_t sampc = SRATE * CH * PTIME / 1000;

	sampv = mem_zalloc(sampc * sizeof(int16_t), NULL);
	if (!sampv)
		return ENOMEM;

	auframe_init(&af, AUFMT_S16LE, sampv, sampc, SRATE, CH);

	re_atomic_rlx_set(&record.start_time, tmr_jiffies());

	while (re_atomic_rlx(&record.run)) {
		sys_msleep(4);
		while (aubuf_cur_size(record.ab) > sampc) {
			aubuf_read_auframe(record.ab, &af);
			err = record_track(&af);
			if (err)
				goto out;
		}
	}

out:
	re_atomic_rlx_set(&record.start_time, 0);
	mem_deref(sampv);

	return 0;
}


void sl_record(struct auframe *af)
{
	if (!re_atomic_rlx(&record.run) || !af->id)
		return;

	af->timestamp = tmr_jiffies();
	aubuf_write_auframe(record.ab, af);
}


void sl_record_toggle(const char *folder)
{
	if (!re_atomic_rlx(&record.run))
		sl_record_start(folder);
	else
		sl_record_close();
}


int sl_record_start(const char *folder)
{
	int err;

	if (!folder)
		return EINVAL;

	if (re_atomic_rlx(&record.run))
		return EALREADY;

	re_atomic_rlx_set(&record.start_time, 0);
	str_dup(&record.folder, folder);

	err = aubuf_alloc(&record.ab, 0, 0);
	if (err) {
		return err;
	}

	re_atomic_rlx_set(&record.run, true);
	info("sl_record: started\n");

	thread_create_name(&record.thread, "sl_record", record_thread, NULL);

	return 0;
}


int sl_record_close(void)
{
	if (!re_atomic_rlx(&record.run))
		return EINVAL;

	re_atomic_rlx_set(&record.run, false);
	info("sl_record: close\n");
	thrd_join(record.thread, NULL);

	mem_deref(record.ab);

	record.folder = mem_deref(record.folder);
	list_flush(&record.tracks);

	return 0;
}
