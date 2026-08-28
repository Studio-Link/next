/*    _____ __            ___         __    _       __
 *   / ___// /___  ______/ (_)___    / /   (_)___  / /__
 *   \__ \/ __/ / / / __  / / __ \  / /   / / __ \/ //_/
 *  ___/ / /_/ /_/ / /_/ / / /_/ / / /___/ / / / / ,<
 * /____/\__/\__,_/\__,_/_/\____(_)_____/_/_/ /_/_/|_|
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#ifndef STUDIOLINK_H__
#define STUDIOLINK_H__

#include <re.h>
#include <rem.h>
#include <baresip.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

#define FS_PATH_MAX 512
#define SL_MAX_JSON (512 * 1024)

/******************************************************************************
 * conf.c
 */

struct sl_config {
	struct config *baresip; /**< baresip config object            */

	struct {
		char src[128];
		char mod[16];
	} play, src;
};

const char *sl_conf_path(void);
const char *sl_conf_uuid(void);
int sl_conf_cacert(void);
struct sl_config *sl_conf(void);
int sl_conf_init(void);


/******************************************************************************
 * main.c
 */

/**
 * StudioLink parse CLI args
 *
 * @param argc Argument count
 * @param argv Argument array
 *
 * @return int
 */
int sl_getopt(int argc, char *const argv[]);


/**
 * Init StudioLink dependencies
 *
 * Initializes Libre and Baresip
 *
 * @param conf Baresip config
 *
 * @return int
 */
int sl_baresip_init(const uint8_t *conf);

/**
 * Init StudioLink
 *
 * @return int
 */
int sl_init(void);

/**
 * StudioLink Open web user interface
 *
 * @return int
 */
int sl_open_webui(void);

/**
 * StudioLink in headless mode?
 *
 * @return true for headless, otherwise false
 */
bool sl_headless(void);

/**
 * StudioLink Main function
 *
 * @return int
 */
int sl_main(void);

/**
 * Close/Exit StudioLink
 */
void sl_close(void);


/******************************************************************************
 * http/client.c
 */
enum sl_httpc_met {
	SL_HTTP_GET,
	SL_HTTP_POST,
	SL_HTTP_PUT,
	SL_HTTP_PATCH,
	SL_HTTP_DELETE
};
struct sl_httpc;
int sl_httpc_alloc(struct sl_httpc **http, http_resp_h *resph);
int sl_httpc_req(struct sl_httpc *http, enum sl_httpc_met sl_met, char *url);


/******************************************************************************
 * http/server.c
 */

int sl_http_listen(uint16_t *port);
void sl_http_close(void);


/******************************************************************************
 * ws.c
 */
enum ws_type { WS_TRACKS, WS_METERS, WS_DEBUG };
int sl_ws_init(void);
int sl_ws_close(void);
int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh);
void sl_ws_send_str(enum ws_type ws_type, char *str);
void sl_ws_send_mb(enum ws_type type, const struct mbuf *mb);
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);


/******************************************************************************
 * tracks.c
 */
/* Local audio device track */
struct sl_local {
	struct slaudio *slaudio;
};

/* Remote audio call track */
struct sl_remote {
	struct call *call;
};
enum sl_track_type { SL_TRACK_REMOTE, SL_TRACK_LOCAL };
enum sl_track_status {
	SL_TRACK_INVALID	     = -1,
	SL_TRACK_IDLE		     = 0,
	SL_TRACK_LOCAL_REGISTERING   = 1,
	SL_TRACK_LOCAL_REGISTER_OK   = 2,
	SL_TRACK_LOCAL_REGISTER_FAIL = 3,
	SL_TRACK_LOCAL_AUDIO_READY   = 4,
	SL_TRACK_REMOTE_CONNECTED    = 5,
	SL_TRACK_REMOTE_CALLING	     = 6,
	SL_TRACK_REMOTE_INCOMING     = 7,
};
struct sl_track {
	struct le le;
	uint16_t id;
	enum sl_track_type type;
	char name[64];
	char error[128];
	enum sl_track_status status;
	bool muted;
	union
	{
		struct sl_local local;
		struct sl_remote remote;
	} u;
};
int sl_tracks_init(void);
int sl_tracks_close(void);
const struct list *sl_tracks(void);
int sl_track_next_id(void);
int sl_track_add(struct sl_track **trackp, enum sl_track_type type);
int sl_track_del(int id);
enum sl_track_status sl_track_status(int id);
int sl_tracks_json(struct re_printf *pf, void *arg);
struct sl_track *sl_track_by_id(int id);
struct slaudio *sl_track_audio(struct sl_track *track);
int sl_track_dial(struct sl_track *track, struct pl *peer);
void sl_track_accept(struct sl_track *track);
void sl_track_hangup(struct sl_track *track);
void sl_track_toggle_mute(struct sl_track *track);
void sl_track_ws_send(void);


/******************************************************************************
 * audio.c
 */
struct slaudio;
int sl_audio_init(void);
int sl_audio_close(void);
int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track);
int slaudio_odict(struct odict **o, struct slaudio *a);
int sl_audio_set_device(struct slaudio *audio, int play_idx, int src_idx);
void sl_audio_mute(struct slaudio *audio, bool mute);


/******************************************************************************
 * meter.c
 */

void sl_meter_init(void);
void sl_meter_close(void);
void sl_meter_process(unsigned int ch, float *in, unsigned long nframes);


/******************************************************************************
 * db.c
 */
struct sldb {
	size_t sz;
	void *data;
};

int sl_db_init(void);
void sl_db_close(void);
int sl_db_get(struct sldb *key, struct sldb *val);
int sl_db_set(struct sldb *key, struct sldb *val);


/******************************************************************************
 * account.c
 */
int sl_account_init(void);
int sl_account_close(void);
struct ua *sl_account_ua(void);


/******************************************************************************
 * record.c
 */
uint64_t sl_record_msecs(void);
void sl_record_toggle(void);
int sl_record_start(void);
void sl_record(struct auframe *af);
int sl_record_close(void);


/******************************************************************************
 * resample.c
 */
struct sl_rsmp;
int sl_rsmp_alloc(struct sl_rsmp **rsmpp, unsigned ch, uint32_t irate,
		  uint32_t orate, size_t in_max);
size_t sl_rsmp_process(struct sl_rsmp *rsmp, const float *in, size_t in_frm,
		       float *out, size_t out_max);
void sl_rsmp_set_ratio(struct sl_rsmp *rsmp, double ratio);
double sl_rsmp_get_ratio(const struct sl_rsmp *rsmp);
size_t sl_rsmp_staged(const struct sl_rsmp *rsmp);
void sl_rsmp_reset(struct sl_rsmp *rsmp);


/******************************************************************************
 * drift.c
 */
struct sl_drift;

/**
 * Allocate a clock drift controller for one audio buffer
 *
 * @param driftp    Allocated controller
 * @param srate     Samplerate of the buffer content
 * @param ch        Channels of the buffer content
 * @param target_sz Wanted fill level in bytes
 *
 * @return 0 if success, otherwise errorcode
 */
int sl_drift_alloc(struct sl_drift **driftp, uint32_t srate, uint8_t ch,
		   size_t target_sz);

/**
 * Feed the current fill level of the buffer to the controller
 *
 * @param drift  Controller
 * @param cur_sz Current fill level in bytes
 * @param now    Current time in ms (tmr_jiffies)
 */
void sl_drift_update(struct sl_drift *drift, size_t cur_sz, uint64_t now);

/**
 * @return Resample ratio that cancels the drift, 1.0 means no correction
 */
double sl_drift_ratio(const struct sl_drift *drift);

/**
 * @return Applied correction in ppm. This is the negative of the measured
 *         clock offset between producer and consumer.
 */
double sl_drift_ppm(const struct sl_drift *drift);

/**
 * @return true once the controller has settled and is steering
 */
bool sl_drift_locked(const struct sl_drift *drift);

/**
 * Move the wanted fill level
 *
 * The setpoint has to clear one hardware block plus the mixer frame size,
 * otherwise a large block can empty the buffer between refills no matter
 * how well the drift itself is tracked.
 *
 * @param drift     Controller
 * @param target_sz Wanted fill level in bytes
 */
void sl_drift_set_target(struct sl_drift *drift, size_t target_sz);

/** Drop the estimate, e.g. after a device change or a buffer flush */
void sl_drift_reset(struct sl_drift *drift);


/******************************************************************************
 * flac.c
 */
struct flac;
int sl_flac_init(struct flac **flacp, struct auframe *af, char *file);
int sl_flac_record(struct flac *flac, struct auframe *af, uint64_t offset);


#ifdef __cplusplus
}
#endif

#endif /* STUDIOLINK_H__ */
