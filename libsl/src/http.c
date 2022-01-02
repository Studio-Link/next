#include <re.h>
#include <baresip.h>
#include <studiolink.h>


struct sl_http {
	struct http_cli *client;
	struct http_reqconn *conn;
};


static void destroy(void *arg)
{
	struct sl_http *p = arg;
	mem_deref(p->client);
	mem_deref(p->conn);
}


int sl_http_alloc(struct sl_http **http, http_resp_h *resph)
{
	int err;
	struct sl_http *p;

	p = mem_zalloc(sizeof(struct sl_http), destroy);
	if (!p)
		return ENOMEM;

	err = http_client_alloc(&p->client, net_dnsc(baresip_network()));
	if (err)
		return err;

	err = http_reqconn_alloc(&p->conn, p->client, resph, NULL, NULL);
	if (err)
		return err;

	*http = p;

	return err;
}


int sl_http_req(struct sl_http *http, enum SL_HTTP_MET sl_met, char *url)
{
	struct pl met, uri;
	int err;

	if (!http | !url)
		return EINVAL;

	switch (sl_met) {
	case SL_HTTP_GET:
		pl_set_str(&met, "GET");
		break;
	case SL_HTTP_POST:
		pl_set_str(&met, "POST");
		break;
	case SL_HTTP_PUT:
		pl_set_str(&met, "PUT");
		break;
	case SL_HTTP_PATCH:
		pl_set_str(&met, "PATCH");
		break;
	case SL_HTTP_DELETE:
		pl_set_str(&met, "DELETE");
		break;
	}

	pl_set_str(&uri, url);


	err = http_reqconn_set_method(http->conn, &met);
	if (err)
		return err;

	err = http_reqconn_send(http->conn, &uri);

	return 0;
}


static void http_sreply(struct http_conn *conn, uint16_t scode,
		       const char *reason, const char *ctype, const char *fmt,
		       size_t size)
{
	struct mbuf *mb;
	int err = 0;
	(void)scode;
	(void)reason;

	mb = mbuf_alloc(size);
	if (!mb) {
		warning("http_sreply: ENOMEM\n");
		return;
	}

	err = mbuf_write_mem(mb, (const uint8_t *)fmt, size);
	if (err) {
		warning("http_sreply: mbuf_write_mem err %m\n", err);
		goto out;
	}

	err = http_reply(conn, 200, "OK",
		   "Content-Type: %s\r\n"
		   "Content-Length: %zu\r\n"
		   "Cache-Control: no-cache, no-store, must-revalidate\r\n"
		   "\r\n"
		   "%b",
		   ctype, mb->end, mb->buf, mb->end);
	if (err)
		warning("http_sreply: http_reply err %m\n", err);
out:
	mem_deref(mb);
}


static void http_req_handler(struct http_conn *conn,
			     const struct http_msg *msg, void *arg)
{
	(void)arg;


	if (!conn || !msg)
		return;

	/*
	 * Static Requests
	 */
	if (0 == pl_strcasecmp(&msg->path, "/")) {
		http_sreply(conn, 200, "OK", "text/html", "", 0);
		return;
	}
}


int sl_http_listen(struct http_sock **sock)
{
	int err;
	struct sa srv;

	if (!sock)
		return EINVAL;

	err = sa_set_str(&srv, "127.0.0.1", 9999);

	info("listen: %J\n", &srv);
	err = http_listen(sock, &srv, http_req_handler, NULL);
	if (err)
		return err;

	return 0;
}
