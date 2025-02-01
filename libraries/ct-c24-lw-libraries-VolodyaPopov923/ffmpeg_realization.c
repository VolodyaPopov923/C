#include "ffmpeg_realization.h"

#include "return_codes.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

uint8_t buildStructContext(ffmpeg_context *context, uint16_t argc, char *argv[], uint16_t number_file)
{
	context->pFormatContext = avformat_alloc_context();
	if (context->pFormatContext == NULL)
	{
		fprintf(stderr, "error with format context");
		return ERROR_FORMAT_INVALID;
	}

	if (avformat_open_input(&context->pFormatContext, ((number_file == 2 && argc == 3) ? argv[2] : argv[1]), NULL, NULL) != 0)
	{
		fprintf(stderr, "error with open file");
		return ERROR_CANNOT_OPEN_FILE;
	}

	if (avformat_find_stream_info(context->pFormatContext, NULL) < 0)
	{
		fprintf(stderr, "error with find stream info");
		return ERROR_FORMAT_INVALID;
	}

	context->number_stream = av_find_best_stream(context->pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (context->number_stream < 0)
	{
		fprintf(stderr, "error with find best stream");
		return ERROR_FORMAT_INVALID;
	}
	context->pCodecParameters = context->pFormatContext->streams[context->number_stream]->codecpar;
	if (context->pCodecParameters == NULL)
	{
		fprintf(stderr, "error with codec parameters");
		return ERROR_FORMAT_INVALID;
	}
	context->pCodec = avcodec_find_decoder(context->pCodecParameters->codec_id);
	enum AVCodecID ci = context->pCodecParameters->codec_id;
	if (context->pCodec == NULL ||
		(ci != AV_CODEC_ID_AAC && ci != AV_CODEC_ID_MP3 && ci != AV_CODEC_ID_MP2 && ci != AV_CODEC_ID_OPUS && ci != AV_CODEC_ID_FLAC))
	{
		fprintf(stderr, "illegal codec");
		return ERROR_FORMAT_INVALID;
	}
	if (context->pCodecParameters->codec_type != AVMEDIA_TYPE_AUDIO)
	{
		fprintf(stderr, "error with codec type");
		return ERROR_FORMAT_INVALID;
	}

	context->pCodecContext = avcodec_alloc_context3(context->pCodec);
	if (context->pCodecContext == NULL)
	{
		fprintf(stderr, "error with codec context");
		return ERROR_FORMAT_INVALID;
	}
	avcodec_parameters_to_context(context->pCodecContext, context->pCodecParameters);
	if (avcodec_open2(context->pCodecContext, context->pCodec, NULL) < 0)
	{
		fprintf(stderr, "error with codec open");
		return ERROR_FORMAT_INVALID;
	}
	context->pPacket = av_packet_alloc();
	if (context->pPacket == NULL)
	{
		fprintf(stderr, "error with packet");
		return ERROR_FORMAT_INVALID;
	}
	context->pFrame = av_frame_alloc();
	if (context->pFrame == NULL)
	{
		fprintf(stderr, "error with frame");
		return ERROR_FORMAT_INVALID;
	}

	context->max_arr_len = 256;
	context->arr_size = 0;
	context->arr = malloc(256 * sizeof(double));
	if (context->arr == NULL)
	{
		fprintf(stderr, "error with arr");
		return ERROR_NOTENOUGH_MEMORY;
	}

	context->swr_ctx = swr_alloc();
	if (context->swr_ctx == NULL)
	{
		fprintf(stderr, "error with swr ctx");
		return ERROR_ARGUMENTS_INVALID;
	}
	uint8_t set_opts_result = swr_alloc_set_opts2(
		&context->swr_ctx,
		&context->pCodecContext->ch_layout,
		AV_SAMPLE_FMT_DBLP,
		context->pCodecContext->sample_rate,
		&context->pCodecContext->ch_layout,
		context->pCodecContext->sample_fmt,
		context->pCodecContext->sample_rate,
		0,
		NULL);
	if (set_opts_result < 0)
	{
		fprintf(stderr, "error with swr set opts");
		return ERROR_FORMAT_INVALID;
	}
	uint8_t init_result = swr_init(context->swr_ctx);
	if (init_result < 0)
	{
		fprintf(stderr, "error with swr init");
		return ERROR_FORMAT_INVALID;
	}
	if (argc == 2 && context->pCodecContext->ch_layout.nb_channels != 2)
	{
		fprintf(stderr, "error with number of channels");
		return ERROR_FORMAT_INVALID;
	}
	if (argc == 3 && context->pCodecContext->ch_layout.nb_channels < 1)
	{
		fprintf(stderr, "error with number of channels");
		return ERROR_FORMAT_INVALID;
	}
	return SUCCESS;
}

uint8_t readCheck(ffmpeg_context *context1, ffmpeg_context *context2, uint16_t cnt_file, uint8_t peredisk)
{
	uint8_t *converted_values[8];
	uint8_t res;
	converted_values[0] = malloc(256 * sizeof(double));
	converted_values[1] = malloc(256 * sizeof(double));
	if (converted_values[0] == NULL || converted_values[1] == NULL)
	{
		fprintf(stderr, "error with malloc");
		return ERROR_NOTENOUGH_MEMORY;
	}
	uint32_t converted_values_size = 256;
	int32_t out_samples;

	while (av_read_frame(context1->pFormatContext, context1->pPacket) >= 0)
	{
		if (context1->number_stream != context1->pPacket->stream_index)
			continue;
		int32_t response = avcodec_send_packet(context1->pCodecContext, context1->pPacket);
		while (response >= 0)
		{
			response = avcodec_receive_frame(context1->pCodecContext, context1->pFrame);
			if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
				break;
			AVFrame *frame = context1->pFrame;
			int32_t samples = frame->nb_samples;

			while (samples > converted_values_size)
			{
				converted_values_size *= 2;
				res = reallocArray(&converted_values[0], &converted_values_size);
				if (res != 0)
					return res;
				res = reallocArray(&converted_values[1], &converted_values_size);
				if (res != 0)
					return res;
			}
			if (peredisk)
			{
				out_samples = (int32_t)av_rescale_rnd(swr_get_delay(context1->swr_ctx, 44100) + samples, 44100, 44100, AV_ROUND_UP);

				av_samples_alloc(converted_values, NULL, context1->pCodecContext->ch_layout.nb_channels, (int32_t)out_samples, AV_SAMPLE_FMT_DBLP, 0);
			}
			swr_convert(
				context1->swr_ctx,
				(uint8_t **)converted_values,
				peredisk ? out_samples : samples,
				(const uint8_t **)frame->data,
				samples);
			samples = peredisk ? out_samples : samples;

			for (uint64_t i = 0; i < samples; i++)
			{
				if (add_element(&context1->arr, &context1->arr_size, &context1->max_arr_len, ((double *)converted_values[0])[i]) != 0)
				{
					fprintf(stderr, "error with add element");
					freeArrFmmpeg(converted_values[0], converted_values[1], context1->arr, context2->arr);
					return ERROR_NOTENOUGH_MEMORY;
				}
				if (cnt_file == 1)
					if (add_element(&context2->arr, &context2->arr_size, &context2->max_arr_len, ((double *)converted_values[1])[i]) != 0)
					{
						fprintf(stderr, "error with add element");
						freeArrFmmpeg(converted_values[0], converted_values[1], context1->arr, context2->arr);
						return ERROR_NOTENOUGH_MEMORY;
					}
			}
			av_frame_unref(context1->pFrame);
		}
		av_packet_unref(context1->pPacket);
	}
	free(converted_values[0]);
	free(converted_values[1]);
	return SUCCESS;
}

uint8_t add_element(double **arr, uint32_t *arr_size, uint32_t *max_arr_len, double element)
{
	if (*arr_size == *max_arr_len)
	{
		double *temp = realloc(*arr, (*max_arr_len << 1) * sizeof(double));
		if (temp == NULL)
		{
			fprintf(stderr, "error with realloc");
			return ERROR_NOTENOUGH_MEMORY;
		}
		*arr = temp;
		*max_arr_len <<= 1;
	}
	(*arr)[*arr_size] = element;
	*arr_size += 1;
	return SUCCESS;
}

uint8_t resampling(ffmpeg_context *context1, ffmpeg_context *context2)
{
	ffmpeg_context *context_low_rate = context1->pCodecContext->sample_rate < context2->pCodecContext->sample_rate ? context1 : context2;
	ffmpeg_context *context_high_rate = context1->pCodecContext->sample_rate < context2->pCodecContext->sample_rate ? context2 : context1;

	context_low_rate->swr_ctx = swr_alloc();
	if (context_low_rate->swr_ctx == NULL)
	{
		fprintf(stderr, "Error allocating swr context");
		return ERROR_FORMAT_INVALID;
	}

	uint8_t set_opts_result = swr_alloc_set_opts2(
		&context_low_rate->swr_ctx,
		&context_low_rate->pCodecContext->ch_layout,
		AV_SAMPLE_FMT_DBLP,
		context_high_rate->pCodecContext->sample_rate,
		&context_low_rate->pCodecContext->ch_layout,
		context_low_rate->pCodecContext->sample_fmt,
		context_low_rate->pCodecContext->sample_rate,
		0,
		NULL);

	if (set_opts_result < 0)
	{
		fprintf(stderr, "Error setting swr options");
		return ERROR_FORMAT_INVALID;
	}

	uint8_t init_result = swr_init(context_low_rate->swr_ctx);
	if (init_result < 0)
	{
		fprintf(stderr, "Error initializing swr context");
		return ERROR_FORMAT_INVALID;
	}
	uint8_t nb_samples = swr_get_out_samples(context_low_rate->swr_ctx, context_low_rate->pFrame->nb_samples);
	if (nb_samples < 0)
	{
		fprintf(stderr, "Error getting output samples");
		return ERROR_FORMAT_INVALID;
	}
	return SUCCESS;
}

uint8_t reallocArray(uint8_t **array, const uint32_t *array_size)
{
	uint8_t *temp = realloc(*array, *array_size * sizeof(double));
	if (temp == NULL)
	{
		fprintf(stderr, "error with realloc");
		return ERROR_NOTENOUGH_MEMORY;
	}
	*array = temp;
	return SUCCESS;
}

uint8_t reallocArrayD(double **array, const int32_t *array_size)
{
	double *temp = realloc(*array, *array_size * sizeof(double));
	if (temp == NULL)
	{
		fprintf(stderr, "error with realloc");
		return ERROR_NOTENOUGH_MEMORY;
	}
	*array = temp;
	return SUCCESS;
}

void freeArrFmmpeg(double *array1, double *array2, double *array3, double *array4)
{
	free(array1);
	free(array2);
	free(array3);
	free(array4);
}
