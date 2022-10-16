#include <studiolink.h>

#if defined(WIN32)
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

#if defined(PATH_MAX)
#define FS_PATH_MAX PATH_MAX
#elif defined(_POSIX_PATH_MAX)
#define FS_PATH_MAX _POSIX_PATH_MAX
#else
#define FS_PATH_MAX 512
#endif

enum { UUID_LEN = 37 };
static char conf_path[FS_PATH_MAX] = {0};
static char uuid[UUID_LEN];


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

out:
	return conf_path;
}


const char *sl_conf_uuid(void)
{
	char path[FS_PATH_MAX];
	FILE *f = NULL;
	int err = 0;

	re_snprintf(path, sizeof(path), "%s/uuid", sl_conf_path());

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