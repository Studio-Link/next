//
// Created by sreimers on 7/2/24.
//

#ifndef STUDIO_LINK_LOGGER_H
#define STUDIO_LINK_LOGGER_H
#define LOG_TAG "SL Lib"

#define LOGD(...)                     \
        ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#define LOGI(...)                     \
        if (log_level_get() < LEVEL_WARN) \
        ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#define LOGW(...)                      \
        if (log_level_get() < LEVEL_ERROR) \
        ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#define LOGE(...)                       \
        if (log_level_get() <= LEVEL_ERROR) \
        ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif //STUDIO_LINK_LOGGER_H
