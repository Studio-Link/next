#include <jni.h>
#include <android/log.h>
#include <re_dbg.h>
#include <studiolink.h>
#include "logger.h"

static thrd_t t;

static void log_handler(uint32_t level, const char *msg)
{
    LOGD("%s", msg);
}

static struct log lg = {
        .h = log_handler,
};

static void dbg_print(int level, const char *p, size_t len, void *arg)
{
    LOGD("%s", p);
}

static int sl_thread(void *arg)
{
    int err;
    err = sl_baresip_init(NULL);
    LOGD("Startup init: %d", err);
    if (err)
        return err;

    err = sl_init();
    if (err)
        return err;

    err = sl_main();

    sl_close();

    LOGD("Exit");
    return err;
}


JNIEXPORT jint JNICALL Java_link_studio_app_Api_slStart(JNIEnv *env, jobject obj, jstring jPath)
{
    const char *path = (*env)->GetStringUTFChars(env, jPath, 0);

    dbg_handler_set(dbg_print, NULL);
    log_register_handler(&lg);

    sl_conf_path_set(path);

    int ret = thrd_create(&t, sl_thread, NULL);
    if (ret != thrd_success)
        return ENOMEM;

    return 0;
}