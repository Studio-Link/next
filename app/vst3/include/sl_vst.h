/*    _____ __            ___         __    _       __
 *   / ___// /___  ______/ (_)___    / /   (_)___  / /__
 *   \__ \/ __/ / / / __  / / __ \  / /   / / __ \/ //_/
 *  ___/ / /_/ /_/ / /_/ / / /_/ / / /___/ / / / / ,<
 * /____/\__/\__,_/\__,_/_/\____(_)_____/_/_/ /_/_/|_|
 *
 * VST3 plugin bridge - C interface used by the C++ plugin sources.
 *
 * Copyright Sebastian Reimers
 * License: MIT (see LICENSE File)
 */

#ifndef SL_VST_H__
#define SL_VST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * StudioLink runs its audio engine at a fixed rate/layout. The DAW may use
 * anything, so the bridge resamples and converts on the audio thread.
 */
enum {
	SL_VST_SRATE = 48000, /**< StudioLink internal samplerate  */
	SL_VST_CH    = 2,     /**< StudioLink internal channels    */
	SL_VST_PTIME = 20,    /**< StudioLink internal frame in ms */
};


/******************************************************************************
 * engine.c - process wide StudioLink instance
 *
 * libsl uses global state (one libre main loop, one HTTP server, one track
 * list), so all plugin instances in a host process share a single engine.
 * The engine is reference counted and runs re_main() on its own thread.
 */

/**
 * Reference the shared StudioLink engine, starting it if needed.
 *
 * Blocks until the engine is up (or failed). Must not be called from the
 * realtime audio thread.
 *
 * @return 0 if success, otherwise errorcode
 */
int sl_vst_engine_ref(void);

/**
 * Release a reference. Stops the engine when the last one is dropped.
 */
void sl_vst_engine_deref(void);

/**
 * @return true if the engine is initialized and running
 */
bool sl_vst_engine_running(void);

/** Control commands, marshalled to the StudioLink thread */
enum sl_vst_cmd {
	SL_VST_CMD_MUTE,   /**< value: 1 mute, 0 unmute local track    */
	SL_VST_CMD_RECORD, /**< value: 1 start, 0 stop recording       */
	SL_VST_CMD_WEBUI,  /**< value: ignored, opens the web frontend */
};

/**
 * Send a control command to the StudioLink thread.
 *
 * Thread safe and non blocking (a single write() on the mqueue pipe), so it
 * may be called from the audio thread - but only on actual value changes.
 *
 * @param cmd   Command
 * @param value Command value
 *
 * @return 0 if success, otherwise errorcode
 */
int sl_vst_engine_cmd(enum sl_vst_cmd cmd, int32_t value);


/******************************************************************************
 * driver.c - baresip ausrc/auplay driver named "vst"
 *
 * Registered before sl_init() so that the local StudioLink track picks the
 * DAW as its audio device instead of portaudio.
 */

int sl_vst_driver_register(void);
void sl_vst_driver_unregister(void);

/**
 * Claim the audio bridge for one plugin instance.
 *
 * Only one instance can carry audio, because libsl currently supports a
 * single local track. Every other instance stays in bypass.
 *
 * @return true if this caller is now the owner
 */
bool sl_vst_driver_claim(void);
void sl_vst_driver_release(void);

/**
 * Push one block of DAW input into StudioLink (48kHz S16LE stereo).
 *
 * @param sampv Interleaved samples
 * @param sampc Number of samples (frames * SL_VST_CH)
 */
void sl_vst_driver_write(const int16_t *sampv, size_t sampc);

/**
 * Pull one block of the StudioLink mix (48kHz S16LE stereo). Missing samples
 * are zero filled.
 *
 * @param sampv Interleaved samples
 * @param sampc Number of samples (frames * SL_VST_CH)
 */
void sl_vst_driver_read(int16_t *sampv, size_t sampc);

/**
 * Run the 20ms frame exchange with baresip. Called from the audio thread
 * after write() and before read().
 *
 * @param need_sampc Number of samples the caller is about to read
 */
void sl_vst_driver_pump(size_t need_sampc);

void sl_vst_driver_flush(void);


/******************************************************************************
 * io.c - per plugin instance audio bridge
 */

struct sl_vst_io;

/**
 * Allocate the audio bridge for one plugin instance.
 *
 * Not realtime safe, call from setupProcessing()/setActive().
 *
 * @param iop       Allocated bridge
 * @param srate     DAW samplerate
 * @param maxframes DAW max. block size
 *
 * @return 0 if success, otherwise errorcode
 */
int sl_vst_io_alloc(struct sl_vst_io **iop, uint32_t srate, size_t maxframes);

void sl_vst_io_free(struct sl_vst_io *io);

/**
 * @return true if this instance owns the audio bridge
 */
bool sl_vst_io_owner(const struct sl_vst_io *io);

/**
 * Process one DAW block. Realtime safe.
 *
 * Non owner instances and blocks larger than maxframes are passed through
 * unchanged.
 *
 * @param io      Audio bridge
 * @param in      DAW input channel buffers
 * @param out     DAW output channel buffers
 * @param ch_in   Number of input channels
 * @param ch_out  Number of output channels
 * @param nframes Number of frames
 */
void sl_vst_io_process(struct sl_vst_io *io, const float *const *in,
		       float *const *out, unsigned ch_in, unsigned ch_out,
		       size_t nframes);

#ifdef __cplusplus
}
#endif

#endif /* SL_VST_H__ */
