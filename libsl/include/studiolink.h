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

#ifdef __cplusplus
extern "C" {
#endif

/* main.c */
int sl_getopt(int argc, char *const argv[]);
int sl_init(const uint8_t *conf);
int sl_open_webui(void);
int sl_main(void);
void sl_close(void);


/* http.c */
enum SL_HTTP_MET {
	SL_HTTP_GET,
	SL_HTTP_POST,
	SL_HTTP_PUT,
	SL_HTTP_PATCH,
	SL_HTTP_DELETE,
};
struct sl_http;
int sl_http_alloc(struct sl_http **http, http_resp_h *resph);
int sl_http_req(struct sl_http *http, enum SL_HTTP_MET sl_met, char *url);
int sl_http_listen(struct http_sock **sock);

/* ws.c */
enum ws_type { WS_TRACKS };
int sl_ws_init(void);
int sl_ws_close(void);
int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh);

/* ws_tracks.c */
void sl_ws_tracks(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);


#ifdef __cplusplus
}
#endif

#endif /* STUDIOLINK_H__ */
