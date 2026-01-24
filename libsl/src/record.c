/**
 * @file record.c generic recording
 *
 * Copyright (C) 2026 Sebastian Reimers
 */
#include <stdlib.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <studiolink.h>


static struct {
	struct list tracks;
	RE_ATOMIC bool run;
	thrd_t thread;
	struct aubuf *ab;
	char folder[256];
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

		re_snprintf(track->file, sizeof(track->file),
			    "%s/track_%u.flac", record.folder, track->id);

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


static void record_open_folder(void)
{
	char command[256] = {0};

#if defined(DARWIN)
	re_snprintf(command, sizeof(command), "open %s", record.folder);
#elif defined(WIN32)
	re_snprintf(command, sizeof(command), "explorer.exe %s",
		    record.folder);
#else
	re_snprintf(command, sizeof(command), "xdg-open %s", record.folder);
#endif
	system(command);
}


void sl_record_toggle(void)
{
	if (!re_atomic_rlx(&record.run))
		sl_record_start();
	else
		sl_record_close();
}


static int timestamp_print(struct re_printf *pf, const struct tm *tm)
{
	if (!tm)
		return 0;

	return re_hprintf(pf, "%d-%02d-%02d-%02d-%02d-%02d",
			  1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			  tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static int folder_init(void)
{
	char buf[256];
	char basefolder[256];

#ifdef WIN32
	char win32_path[MAX_PATH];

	if (S_OK !=
	    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, win32_path)) {
		return ENOENT;
	}
	str_ncpy(buf, win32_path, sizeof(buf));
#else
	int err = fs_gethome(buf, sizeof(buf));
	if (err)
		return err;
#endif

	(void)re_snprintf(basefolder, sizeof(basefolder),
			  "%s" DIR_SEP "studio-link", buf);
	(void)fs_mkdir(basefolder, 0700);

	(void)re_snprintf(basefolder, sizeof(basefolder),
			  "%s" DIR_SEP "studio-link", buf);

	struct timespec tspec;
	struct tm tm;

	(void)clock_gettime(CLOCK_REALTIME, &tspec);
	if (!localtime_r(&tspec.tv_sec, &tm))
		return EINVAL;

	(void)re_snprintf(record.folder, sizeof(record.folder),
			  "%s" DIR_SEP "%H", basefolder, timestamp_print, &tm);

	return fs_mkdir(record.folder, 0700);
}


int sl_record_start(void)
{
	int err;

	if (re_atomic_rlx(&record.run))
		return EALREADY;

	re_atomic_rlx_set(&record.start_time, 0);

	err = folder_init();
	if (err) {
		warning("sl_record_start: folder_init err %m\n", err);
		return err;
	}

	err = aubuf_alloc(&record.ab, 0, 0);
	if (err)
		return err;

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

	list_flush(&record.tracks);

	record_open_folder();

	return 0;
}
