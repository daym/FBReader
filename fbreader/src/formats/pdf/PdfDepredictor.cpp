#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include "PdfDepredictor.h"

/* this is used for the funniest things in PDF in the wild, even for the Trailer (which isn't a picture :P) */

	unsigned int abs42(int a) {
		return(a > 0) ? a : -a;
	}
	
/* this could actually be converted into a fully-fledged InputStream if it just buffered two "lines". */
	/* in-place depredictor */
	int PNG_depredict(int columns, unsigned char* data, int data_len) {
		unsigned char* row_data;
		unsigned char* original_row_data;
		unsigned char* original_data;
		int original_data_len;
		int scanline_length = columns + 1;
		int paeth = 0;
		int pa = 0;
		int index;
		int pb = 0;
		int pc = 0;
		original_data = data;
		original_data_len = data_len;
		for(int row = 0; row_data = data, data_len >= scanline_length; ++row, data += scanline_length, data_len -= scanline_length) {
			int filter = *row_data;
			++row_data;
			original_row_data = row_data;
			if(filter == 0) {
				/* none */
			} else if(filter == 1) { /* Sub */
				for(index = 0; index < scanline_length - 1; ++index, ++row_data) {
					unsigned char left = (index < 1) ? 0 : *(row_data - 1);
					*row_data += left;
					/* prediction [pixel, left, row_data[index]] */
				}
			} else if(filter == 2) { /* Up */
				for(index = 0; index < scanline_length - 1; ++index, ++row_data) {
					unsigned char upper = (row == 0) ? 0 : *(row_data - scanline_length);
					*row_data += upper;
				}
			} else if(filter == 3) { /* Average */
				unsigned char pixel;
				for(index = 0; pixel = row_data[index], index < scanline_length - 1; ++index, ++row_data) {
					unsigned char upper = (row == 0) ? 0 : *(row_data - scanline_length);
					unsigned char left = (index < 1) ? 0 : *(row_data - 1);
					*row_data = (unsigned int) (pixel + (unsigned int)((left + upper)/2));
				}
			} else if(filter == 4) { /* Paeth */
				/* TODO test */
				unsigned char pixel;
				unsigned char left = 0;
				unsigned char upper = 0;
				unsigned char upper_left = 0;
				for(index = 0; pixel = *row_data, index < scanline_length - 1; ++index) {
					left = (index < 1) ? 0 : *(row_data - 1);
					if(row == 0) {
						upper = 0;
						upper_left = 0;
					} else {
						upper = *(row_data - scanline_length);
						upper_left = (index == 0) ? 0 : *(row_data - scanline_length - 1);
					}
					int p = (int) left + (int) upper - (int) upper_left;
					pa = abs42(p - left);
					pb = abs42(p - upper);
					pc = abs42(p - upper_left);
					paeth = (pa <= pb && pa <= pc) ? left :
					        (pb <= pc) ? upper : 
					        upper_left;
					*row_data = (pixel + paeth);
				}
			} else {
				std::cerr << "invalid filter " << filter << std::endl;
				/*raise DepredictionError("invalid filter %r" % filter)*/
			}
		}
		{ /* remove the filters.
		     Memory layout is:
		        <filter> <data> <data> <data> [...]
		        <filter> <data> <data> <data> [...]
		        ...
		     remove the <filter> by scrolling over it. */
			int shortened_count = 0;
			int i;
			for(i = 0; i < original_data_len; i += scanline_length, ++shortened_count) {
				memcpy(&original_data[i - shortened_count], &original_data[i + 1], scanline_length - 1);
			}
			return shortened_count;
		}
	}
	int depredict(int columns, int predictor, unsigned char* data, int data_len) {
		if(predictor == 0 || predictor == 1)
			return 0;
		else if(predictor == 2) /* TIFF */ {
			std::cerr << "error: TIFF predictor not supported yet." << std::endl;
			return 0;
		} else if(predictor == 10 || predictor == 11 || predictor == 12 || predictor == 13 || predictor == 14 || predictor == 15)
			return PNG_depredict(columns, data, data_len);
		else {
			std::cerr << "error: ignored unknown predictor " << predictor << std::endl;
			return 0;
		}
	}
