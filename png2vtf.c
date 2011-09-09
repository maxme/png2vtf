/*
 * Copyright 2002-2010 Based on a libpng example writtent by Guillaume
 * Cottenceau. Modified by Maxime Biais to transform it to a
 * non-premultiplied alpha TO premultiplied alpha converter.
 * http://twitter.com/maximeb
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>
#include "vtf_format.h"

void abort_(const char * s, ...) {
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep *row_pointers;
char *vtf_data;
VTFHEADER vtf_header;

void read_png_file(char* file_name) {
  char header[8];    // 8 is the maximum size that can be checked

  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp)
	abort_("[read_png_file] File %s could not be opened for reading", file_name);
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8))
	abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
	abort_("[read_png_file] png_create_read_struct failed");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
	abort_("[read_png_file] png_create_info_struct failed");

  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[read_png_file] Error during init_io");

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);


  /* read file */
  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[read_png_file] Error during read_image");

  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
  for (y=0; y<height; y++)
	row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

  png_read_image(png_ptr, row_pointers);

  fclose(fp);
}


void write_png_file(char* file_name) {
  /* create file */
  FILE *fp = fopen(file_name, "wb");
  if (!fp)
	abort_("[write_png_file] File %s could not be opened for writing", file_name);


  /* initialize stuff */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
	abort_("[write_png_file] png_create_write_struct failed");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
	abort_("[write_png_file] png_create_info_struct failed");

  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[write_png_file] Error during init_io");

  png_init_io(png_ptr, fp);


  /* write header */
  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[write_png_file] Error during writing header");

  png_set_IHDR(png_ptr, info_ptr, width, height,
			   bit_depth, color_type, PNG_INTERLACE_NONE,
			   PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);


  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[write_png_file] Error during writing bytes");

  png_write_image(png_ptr, row_pointers);


  /* end write */
  if (setjmp(png_jmpbuf(png_ptr)))
	abort_("[write_png_file] Error during end of write");

  png_write_end(png_ptr, NULL);

  /* cleanup heap allocation */
  for (y=0; y<height; y++)
	free(row_pointers[y]);
  free(row_pointers);

  fclose(fp);
}

int is_not_premultiplied() {
  if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
	abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be"
		   "PNG_COLOR_TYPE_RGBA (lacks the alpha channel)");
  if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
	abort_("[process_file] color_type of input file must be "
		   "PNG_COLOR_TYPE_RGBA (%d) (is %d)",
		   PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
  for (y = 0; y < height; y++) {
	png_byte* row = row_pointers[y];
	for (x = 0; x < width; x++) {
	  png_byte* ptr = &(row[x*4]);
	  if ((ptr[3] != 255) && (ptr[0] > ptr[3] || ptr[1] > ptr[3] \
							  || ptr[2] > ptr[3]))
		  return 0;
	  }
	}
  return 1;
}

void convert_file() {
    printf("height=%d, width=%d\n", height, width);
    vtf_data = malloc(sizeof(char) * height * width);
    
    for (y = 0; y < height; y++) {
        png_byte* row = row_pointers[y];
        for (x = 0; x < width; x++) {
            png_byte* ptr = &(row[x*4]);
            // printf("x=%d y=%d\n", x, y);
            vtf_data[y*width +x] = (char) ptr[3]; // 0:r, 1:g, 2:b, 3:alpha
        }
    }
}

void init_vtf_header() {
    vtf_header.signature[0] = 'V';
    vtf_header.signature[1] = 'T';
    vtf_header.signature[2] = 'F';
    vtf_header.signature[3] = 0;
    vtf_header.version[0] = 7;
    vtf_header.version[1] = 2;
    vtf_header.width = log(width) / log(2);
    vtf_header.height = log(height) / log(2);
    vtf_header.highResImageFormat = IMAGE_FORMAT_A8;
}

void write_vtf_header(FILE *fp, const VTFHEADER *h) {
    fwrite(&vtf_header, sizeof(VTFHEADER), 1, fp);
}

void write_vtf_file(char* file_name) {
    init_vtf_header();
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
        abort_("File %s could not be opened for writing", file_name);

    // write header
    write_vtf_header(fp, &vtf_header);
    // write data
    fwrite(vtf_data, 1, height * width, fp);
    free(vtf_data);
    fclose(fp);
}


int main(int argc, char **argv) {
  if (argc != 3)
	abort_("Usage: program_name <file_in> <file_out>");

  read_png_file(argv[1]);
  convert_file();
  write_vtf_file(argv[2]);

  return 0;
}
