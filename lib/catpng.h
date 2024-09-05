#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "lab_png.h"
#include "crc.h"
#include "zutil.h"

void create_IHDR(U8 *arr, U32 height, U32 width);
void create_HEADER(U8 *arr);
void create_IDAT(U8 *arr, U32 length_data, U8 *idat_data);
void create_IEND(U8 *arr, U32 IDAT_length_data);
int test1(int argc, char **argv, void *arg);