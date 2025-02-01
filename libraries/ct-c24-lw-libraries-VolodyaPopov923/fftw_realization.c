#include "fftw_realization.h"

#include "ffmpeg_realization.h"
#include "return_codes.h"

#include <fftw3.h>
#include <stdio.h>

uint8_t fftw_realization(ffmpeg_context *context1, ffmpeg_context *context2)
{
	int32_t max_len = context1->max_arr_len > context2->max_arr_len ? (int32_t)context1->max_arr_len : (int32_t)context2->max_arr_len;
	fftw_complex *out1 = fftw_alloc_complex(sizeof(fftw_complex) * max_len);
	fftw_complex *out2 = fftw_alloc_complex(sizeof(fftw_complex) * max_len);
	fftw_complex *result = fftw_alloc_complex(sizeof(fftw_complex) * max_len);
	double *out = fftw_alloc_real(sizeof(double) * max_len);

	if (!out1 || !out2 || !result || !out)
	{
		fprintf(stderr, "Not enough memory\n");
		freeArr(out1, out2, result, out, context1->arr, context2->arr);
		return ERROR_NOTENOUGH_MEMORY;
	}

	if (context1->max_arr_len > context2->max_arr_len)
	{
		context2->max_arr_len = max_len;
		reallocArrayD(&context2->arr, &max_len);
	}
	else if (context1->max_arr_len < context2->max_arr_len)
	{
		context1->max_arr_len = max_len;
		reallocArrayD(&context1->arr, &max_len);
	}
	for (uint32_t i = context1->arr_size; i < max_len; i++)
		context1->arr[i] = 0;
	for (uint32_t i = context2->arr_size; i < max_len; i++)
		context2->arr[i] = 0;

	fftw_execute(fftw_plan_dft_r2c_1d(max_len, context1->arr, out1, FFTW_ESTIMATE));
	fftw_execute(fftw_plan_dft_r2c_1d(max_len, context2->arr, out2, FFTW_ESTIMATE));
	for (uint32_t i = 0; i < max_len; i++)
	{
		result[i][0] = out1[i][0] * out2[i][0] + out1[i][1] * out2[i][1];
		result[i][1] = -out1[i][0] * out2[i][1] + out1[i][1] * out2[i][0];
	}

	fftw_execute(fftw_plan_dft_c2r_1d(max_len, result, out, FFTW_ESTIMATE));
	int32_t max_index = 0;
	for (int32_t i = 0; i < max_len; i++)
	{
		if (out[i] > out[max_index])
		{
			max_index = i;
		}
	}

	if (max_index > context1->max_arr_len / 2)
		max_index -= max_len;
	int32_t sample_rate = context1->pCodecParameters->sample_rate;
	printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", max_index, sample_rate, max_index * 1000 / sample_rate);
	freeArr(out1, out2, result, out, context1->arr, context2->arr);
	return SUCCESS;
}

void freeArr(fftw_complex *out1, fftw_complex *out2, fftw_complex *result, double *out, double *arr1, double *arr2)
{
	fftw_free(out1);
	fftw_free(out2);
	fftw_free(result);
	fftw_free(out);
	free(arr1);
	free(arr2);
}
