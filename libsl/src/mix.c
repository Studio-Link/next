#include <string.h>
#include <studiolink.h>

#define BUFFER_LEN 19200 /* max buffer_len = 192kHz*2ch*25ms*2frames */

struct mix {
	struct list srcl;
};

struct mix_source {
	struct le le;
	struct mix *mix;
	struct auframe af;
	mix_frame_h *wh;
	mix_frame_h *rh;
	int16_t src_buf[BUFFER_LEN];
	int16_t play_buf[BUFFER_LEN];
	void *arg;
};


static void mix_destructor(void *data)
{
	struct mix *mix = data;

	list_flush(&mix->srcl);
}


int mix_alloc(struct mix **mixp)
{
	struct mix *mix;

	if (!mixp)
		return EINVAL;

	mix = mem_zalloc(sizeof(struct mix), mix_destructor);
	if (!mix)
		return ENOMEM;

	*mixp = mix;

	return 0;
}


static void source_destructor(void *data)
{
	struct mix_source *src = data;

	list_unlink(&src->le);
}


int mix_source_alloc(struct mix_source **srcp, struct mix *mix,
		     mix_frame_h *wh, mix_frame_h *rh, void *arg)
{
	struct mix_source *src;

	if (!srcp || !mix || !wh)
		return EINVAL;

	src = mem_zalloc(sizeof(struct mix_source), source_destructor);
	if (!src)
		return ENOMEM;

	src->mix = mix;
	src->wh	 = wh;
	src->rh	 = rh;
	src->arg = arg;

	list_append(&mix->srcl, &src->le, src);

	auframe_init(&src->af, AUFMT_S16LE, src->src_buf, 0, 48000, 2);

	*srcp = src;

	return 0;
}


int mix_sources(struct mix *mix, struct mix_source *msrc, struct auframe *maf)
{
	struct le *le;

	if (!mix || !msrc || !maf)
		return EINVAL;

	msrc->af = *maf;

	LIST_FOREACH(&mix->srcl, le)
	{
		struct mix_source *src = le->data;

		src->af.sampc = maf->sampc;

		if (src->rh)
			src->rh(&src->af, src->arg);

		memset(src->play_buf, 0, auframe_size(&src->af));
	}

	LIST_FOREACH(&mix->srcl, le)
	{
		struct mix_source *src = le->data;
		struct le *cle;
		struct auframe af;

		/* Initialize from source (assuming equal settings) */
		af = src->af;

		LIST_FOREACH(&mix->srcl, cle)
		{
			struct mix_source *csrc = cle->data;
			int32_t sample;

			/* N-1 */
			if (csrc == src)
				continue;

			for (size_t i = 0; i < csrc->af.sampc; i++) {
				sample = src->play_buf[i] +
					 ((int16_t *)csrc->af.sampv)[i];

				/* soft clipping */
				if (sample >= 32767)
					sample = 32767;
				if (sample <= -32767)
					sample = -32767;

				src->play_buf[i] = sample;
			}
		}

		af.sampv = src->play_buf;
		src->wh(&af, src->arg);
	}

	return 0;
}
