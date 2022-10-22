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

int sl_http_listen(struct http_sock **sock);


/******************************************************************************
 * ws.c
 */
enum ws_type { WS_TRACKS, WS_METERS };
int sl_ws_init(void);
int sl_ws_close(void);
int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh);
void sl_ws_send_str(enum ws_type ws_type, char *str);
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);


/******************************************************************************
 * tracks.c
 */
struct sl_track;
enum sl_track_type { SL_TRACK_REMOTE, SL_TRACK_LOCAL };
enum sl_track_status {
	SL_TRACK_INVALID = -1,
	SL_TRACK_IDLE,
	SL_TRACK_AUDIO_READY,
	SL_TRACK_REMOTE_CONNECTED,
	SL_TRACK_REMOTE_CALLING,
	SL_TRACK_REMOTE_INCOMING,
};
int sl_tracks_init(void);
int sl_tracks_close(void);
const struct list *sl_tracks(void);
int sl_track_next_id(void);
int sl_track_add(struct sl_track **trackp, enum sl_track_type type);
int sl_track_del(int id);
enum sl_track_status sl_track_status(int id);
int sl_tracks_json(struct re_printf *pf);
struct sl_track *sl_track_by_id(int id);
struct slaudio *sl_track_audio(struct sl_track *track);
int sl_track_dial(struct sl_track *track, struct pl *peer);
void sl_track_accept(struct sl_track *track);
void sl_track_hangup(struct sl_track *track);
void sl_track_ws_send(void);


/******************************************************************************
 * audio.c
 */
struct slaudio;
int sl_audio_init(void);
int sl_audio_close(void);
int sl_audio_add_remote_track(struct slaudio *audio, struct sl_track *track);
int sl_audio_del_remote_track(struct sl_track *track);
int sl_audio_alloc(struct slaudio **audiop, struct sl_track *track);
int slaudio_odict(struct odict **o, struct slaudio *a);
int sl_audio_set_device(struct slaudio *audio, int play_idx, int src_idx);


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


#ifdef __cplusplus
}
#endif

#endif /* STUDIOLINK_H__ */
