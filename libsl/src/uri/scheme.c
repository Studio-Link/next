#ifdef WIN32

#include <winreg.h>

int sl_uri_register_scheme(const char *scheme, const char *cmd_path)
{
	LONG ret;
	HKEY hkey     = NULL;
	HKEY hkey_cmd = NULL;
	char subkey[128];

	re_snprintf(subkey, sizeof(subkey), "Software\\Classes\\%s", scheme);

	ret = RegCreateKeyExA(HKEY_CURRENT_USER, class, 0, NULL,
			      REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey,
			      NULL););
	if (ret != ERROR_SUCCESS) {
		warning("sl_uri_register_scheme/URI err: %d", ret);
		return EPERM;
	}

	re_snprintf(subkey, sizeof(subkey), "URL:%s Protocol", scheme);

	ret = RegSetKeyValueA(hkey, NULL, NULL, REG_SZ, subkey,
			      strlen(subkey) + 1);
	if (ret != ERROR_SUCCESS) {
		warning("sl_uri_register_scheme/base proto err: %d", ret);
		return EPERM;
	}

	ret = RegSetKeyValueA(hkey, NULL, "URL Protocol", REG_SZ, "", 1);
	if (ret != ERROR_SUCCESS) {
		warning("sl_uri_register_scheme/proto val err: %d", ret);
		return EPERM;
	}

	ret = RegCreateKeyExA(hkey, "Shell\\Open\\Command", 0, NULL,
			      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
			      &hkey_cmd, NULL);
	if (ret != ERROR_SUCCESS) {
		warning("sl_uri_register_scheme/command key err: %d", ret);
		return EPERM;
	}

	ret = RegSetKeyValueA(hkey_cmd, NULL, NULL, REG_SZ, cmd_path,
			      strlen(cmd_path) + 1);
	if (ret != ERROR_SUCCESS) {
		warning("sl_uri_register_scheme/command val err: %d", ret);
		return EPERM;
	}

	return 0;
}


#endif
