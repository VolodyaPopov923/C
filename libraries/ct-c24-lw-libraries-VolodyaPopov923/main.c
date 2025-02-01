#include "ffmpeg_realization.h"
#include "fftw_realization.h"
#include "return_codes.h"
#include <libavutil/log.h>

#include <stdio.h>

int main(int argc, char *argv[])
{
	av_log_set_level(AV_LOG_QUIET);
	if (argc > 3 || argc < 2)
	{
		fprintf(stderr, "illegal count arguments");
		return ERROR_ARGUMENTS_INVALID;
	}
	ffmpeg_context context1;
	ffmpeg_context context2;
	uint8_t res, peredisk = 0;
	res = buildStructContext(&context1, argc, argv, 1);
	if (res != SUCCESS)
		return res;

	res = buildStructContext(&context2, argc, argv, 2);
	if (res != SUCCESS)
		return res;

	if (context1.pCodecParameters->sample_rate != context2.pCodecParameters->sample_rate)
	{
		peredisk = 1;
		res = resampling(&context1, &context2);
		if (res != SUCCESS)
			return res;
	}
	uint16_t cnt_file = (argc == 2);
	res = readCheck(&context1, &context2, cnt_file, peredisk);
	if (res != SUCCESS)
		return res;
	if (argc == 3)
		res = readCheck(&context2, &context1, cnt_file, peredisk);
	if (res != SUCCESS)
		return res;
	fftw_realization(&context1, &context2);
	return SUCCESS;
}
