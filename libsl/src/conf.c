#include <cacert.h>
#include <studiolink.h>

enum { UUID_LEN = 37 };
static char conf_path[FS_PATH_MAX] = {0};
static char uuid[UUID_LEN];

static struct sl_config slconf = {
	.baresip = NULL, .play.mod = "portaudio", .src.mod = "portaudio"};


struct sl_config *sl_conf(void)
{
	return &slconf;
}

int sl_conf_path_set(const char *path)
{
	int err;

	if (!path)
		return EINVAL;

	if (re_snprintf(conf_path, sizeof(conf_path),
			"%s" DIR_SEP ".studio-link", path) < 0) {
		warning("sl_conf_path: path too long\n");
		return ENAMETOOLONG;
	}

	err = fs_mkdir(conf_path, 0700);
	if (err && err != EEXIST) {
		warning("sl_conf_path: fs_mkdir err %m\n", err);
		return err;
	}

	return 0;
}

/**
 * Get the path to configuration files
 *
 * @return Pointer to path or NULL on error
 */
const char *sl_conf_path(void)
{
	char buf[FS_PATH_MAX];
	int err;

	if (str_isset(conf_path))
		goto out;

	err = fs_gethome(buf, sizeof(buf));
	if (err) {
		warning("sl_conf_path: fs_gethome err %m\n", err);
		return NULL;
	}

	if (re_snprintf(conf_path, sizeof(conf_path),
			"%s" DIR_SEP ".studio-link", buf) < 0) {
		warning("sl_conf_path: path too long\n");
		return NULL;
	}

	err = fs_mkdir(conf_path, 0700);
	if (err && err != EEXIST) {
		warning("sl_conf_path: fs_mkdir err %m\n", err);
	}

out:
	return conf_path;
}


const char *sl_conf_uuid(void)
{
	char path[FS_PATH_MAX];
	FILE *f = NULL;
	int err = 0;

	re_snprintf(path, sizeof(path), "%s" DIR_SEP "uuid", sl_conf_path());

	f = fopen(path, "r");
	if (f) {
		if (!fgets(uuid, sizeof(uuid), f))
			err = errno;
		goto out;
	}

	err = fs_fopen(&f, path, "w");
	if (err)
		goto out;

	info("sl_conf_uuid: generate new uuid\n");

	if (re_snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%08x%04x",
			rand_u32(), rand_u16(), rand_u16(), rand_u16(),
			rand_u32(), rand_u16()) < 0) {
		err = ENOMEM;
		goto out;
	}


	if (re_fprintf(f, "%s", uuid) < 0) {
		err = EFBIG;
		goto out;
	}

out:
	if (f)
		fclose(f);


	if (err) {
		warning("sl_conf_uuid: failed %s (%m)\n", path, err);
		return NULL;
	}

	info("sl_conf_uuid: '%s'\n", uuid);

	return uuid;
}


int sl_conf_cacert(void)
{
	char file[FS_PATH_MAX];
	FILE *f = NULL;
	int err = 0;

	re_snprintf(file, sizeof(file), "%s" DIR_SEP "cacert.pem",
		    sl_conf_path());
	err = fs_fopen(&f, file, "w");
	if (err)
		return err;

	if (fwrite(cacert_pem, cacert_pem_len, 1, f) < 1)
		err = ENFILE;

	if (f)
		fclose(f);

	return err;
}


int sl_conf_init(void)
{
	slconf.baresip = conf_config();
	if (!slconf.baresip)
		return EINVAL;

	return 0;
}
