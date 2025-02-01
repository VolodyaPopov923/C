#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#ifndef LAB_2_FFMPEG_REALIZATION_H
#define LAB_2_FFMPEG_REALIZATION_H

struct ffmpeg_context
{
	AVFormatContext *pFormatContext;
	AVCodecParameters *pCodecParameters;
	AVCodecContext *pCodecContext;
	SwrContext *swr_ctx;
	AVPacket *pPacket;
	AVFrame *pFrame;
	double *arr;
	const AVCodec *pCodec;
	uint16_t number_stream;
	uint32_t arr_size;
	uint32_t max_arr_len;
} typedef ffmpeg_context;

uint8_t buildStructContext(ffmpeg_context *context, uint16_t argc, char *argv[], uint16_t number_file);

uint8_t readCheck(ffmpeg_context *context1, ffmpeg_context *context2, uint16_t cnt_file, uint8_t peredisk);

uint8_t add_element(double **arr, uint32_t *arr_size, uint32_t *max_arr_len, double element);
uint8_t resampling(ffmpeg_context *context1, ffmpeg_context *context2);
uint8_t reallocArray(uint8_t **array, const uint32_t *array_size);
uint8_t reallocArrayD(double **array, const int32_t *array_size);
void freeArrFmmpeg(double *array1, double *array2, double *array3, double *array4);

#endif