#include "ffmpeg_realization.h"

#include <fftw3.h>
#include <stdint.h>

#ifndef LAB_2_FFTW_REALIZATION_H
#define LAB_2_FFTW_REALIZATION_H

uint8_t fftw_realization(ffmpeg_context* context1, ffmpeg_context* context2);
void freeArr(fftw_complex* out1, fftw_complex* out2, fftw_complex* result, double* out, double* arr1, double* arr2);

#endif