#include <studiolink.h>

static struct ua *ua;
struct sl_httpc *httpc;


static void http_resph(int err, const struct http_msg *msg, void *arg)
{
	char aor[1024] = {0};
	struct pl json;
	struct odict *o = NULL;
	const char *user;
	const char *domain;
	const char *password;
	const char *regint;
	const char *stunserver;
	const char *stunuser;
	const char *stunpass;
	(void)arg;


	if (err) {
		warning("sl_account_init/http_resph: err arg: %m\n", err);
		return;
	}

	if (!msg)
		return;

	pl_set_mbuf(&json, msg->mb);

	err = json_decode_odict(&o, 32, json.p, json.l, 8);
	if (err) {
		warning("sl_account_init/json_decode_odict: err %m\n", err);
		return;
	}

	err = ENODATA;

	user = odict_string(o, "user");
	if (!user)
		goto out;

	domain = odict_string(o, "domain");
	if (!domain)
		goto out;

	password = odict_string(o, "password");
	if (!password)
		goto out;

	regint = odict_string(o, "regint");
	if (!regint)
		goto out;

	stunserver = odict_string(o, "stunserver");
	if (!stunserver)
		goto out;

	stunuser = odict_string(o, "stunuser");
	if (!stunuser)
		goto out;

	stunpass = odict_string(o, "stunpass");
	if (!stunpass)
		goto out;

	err = 0;

	re_snprintf(
		aor, sizeof(aor),
		"<sip:%s@%s;transport=tls>;auth_pass=%s;regint=%s;stunserver="
		"\"%s\";medianat=turn;mediaenc=dtls_srtp;stunuser=%s;stunpass="
		"%s;",
		user, domain, password, regint, stunserver, stunuser,
		stunpass);

	err = ua_alloc(&ua, aor);
	if (err)
		goto out;

	err = ua_register(ua);

out:
	if (err)
		warning("sl_account_init/http_resph: err %m\n", err);

	mem_deref(o);
}


int sl_account_init(void)
{
	struct sl_config *conf = sl_conf();
	char url[256];
	int err;

	err = sl_httpc_alloc(&httpc, http_resph);
	if (err)
		goto out;

	re_snprintf(url, sizeof(url),
		    "https://my.studio.link/api/v1/provisioning/%s",
		    conf->baresip->sip.uuid);

	err = sl_httpc_req(httpc, SL_HTTP_GET, url);

out:
	return err;
}


int sl_account_close(void)
{
	httpc = mem_deref(httpc);

	return 0;
}
