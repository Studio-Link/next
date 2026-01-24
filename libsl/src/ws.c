#include <re.h>
#include <baresip.h>
#include <studiolink.h>

static struct websock *ws  = NULL;
static struct list wsl	   = LIST_INIT;
static mtx_t *wsl_lock	   = NULL;
static struct tmr tmr_exit = TMR_INIT;
struct ws_conn {
	struct le le;
	struct websock_conn *c;
	enum ws_type type;
};

void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg)
{
	(void)hdr;
	(void)mb;
	(void)arg;
}


static void exit_baresip(void *arg)
{
	(void)arg;

	re_cancel();
}


static void conn_destroy(void *arg)
{
	struct ws_conn *ws_conn = arg;

	mtx_lock(wsl_lock);
	list_unlink(&ws_conn->le);
	uint32_t count = list_count(&wsl);
	mtx_unlock(wsl_lock);

	if (count == 0 && !sl_headless()) {
		ua_stop_all(false);
		tmr_start(&tmr_exit, 800, exit_baresip, NULL);
	}

	mem_deref(ws_conn->c);
}


static void close_handler(int err, void *arg)
{
	struct ws_conn *ws_conn = arg;
	(void)err;

	mem_deref(ws_conn);
}


int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh)
{
	struct ws_conn *ws_conn;
	int err;

	ws_conn = mem_zalloc(sizeof(*ws_conn), conn_destroy);
	if (!ws_conn)
		return ENOMEM;

	err = websock_accept(&ws_conn->c, ws, conn, msg, 0, recvh,
			     close_handler, ws_conn);
	if (err)
		goto out;

	ws_conn->type = type;

	mtx_lock(wsl_lock);
	list_append(&wsl, &ws_conn->le, ws_conn);
	mtx_unlock(wsl_lock);

out:
	if (err)
		mem_deref(ws_conn);

	return err;
}


void sl_ws_send_str(enum ws_type type, char *str)
{
	struct le *le;

	if (!str)
		return;

	mtx_lock(wsl_lock);
	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *ws_conn = le->data;

		if (ws_conn->type != type)
			continue;

		websock_send(ws_conn->c, WEBSOCK_TEXT, "%s", str);
	}
	mtx_unlock(wsl_lock);
}


void sl_ws_send_mb(enum ws_type type, const struct mbuf *mb)
{
	struct le *le;

	if (!mb)
		return;

	mtx_lock(wsl_lock);
	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *ws_conn = le->data;

		if (ws_conn->type != type)
			continue;

		websock_send(ws_conn->c, WEBSOCK_TEXT, "%b", mbuf_buf(mb),
			     mbuf_get_left(mb));
	}
	mtx_unlock(wsl_lock);
}


int sl_ws_init(void)
{
	int err;

	list_init(&wsl);
	err = websock_alloc(&ws, NULL, NULL);
	if (err) {
		warning("sl_ws_init: websock_alloc failed\n");
		return err;
	}

	err = mutex_alloc_tp(&wsl_lock, mtx_recursive);
	if (err) {
		warning("sl_ws_init: mutex_alloc failed\n");
		mem_deref(ws);
		return err;
	}

	return err;
}


int sl_ws_close(void)
{
	mtx_lock(wsl_lock);
	list_flush(&wsl);
	mtx_unlock(wsl_lock);

	wsl_lock = mem_deref(wsl_lock);
	ws	 = mem_deref(ws);

	return 0;
}
