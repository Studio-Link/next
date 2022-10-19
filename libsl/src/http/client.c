#include <studiolink.h>

struct sl_httpc {
	struct http_cli *client;
	struct http_reqconn *conn;
};


static void destroy(void *arg)
{
	struct sl_httpc *p = arg;
	mem_deref(p->client);
	mem_deref(p->conn);
}


int sl_httpc_alloc(struct sl_httpc **http, http_resp_h *resph)
{
	int err;
	struct sl_httpc *p;
	char file[FS_PATH_MAX];

	p = mem_zalloc(sizeof(struct sl_httpc), destroy);
	if (!p)
		return ENOMEM;

	err = http_client_alloc(&p->client, net_dnsc(baresip_network()));
	if (err)
		goto out;

	err = http_reqconn_alloc(&p->conn, p->client, resph, NULL, NULL);
	if (err)
		goto out;

	re_snprintf(file, sizeof(file), "%s" DIR_SEP "cacert.pem",
		    sl_conf_path());

	err = http_client_add_ca(p->client, file);
	if (err)
		goto out;

out:
	if (err)
		mem_deref(p);
	else
		*http = p;

	return err;
}


int sl_httpc_req(struct sl_httpc *http, enum sl_httpc_met sl_met, char *url)
{
	struct pl met, uri;
	int err;

	if (!http || !url)
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

	return err;
}
