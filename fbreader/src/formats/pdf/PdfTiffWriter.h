#ifndef __PDF_TIFF_WRITER_H
#define __PDF_TIFF_WRITER_H
/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <stdint.h>
 
#define MAX_TIFFHEADER 158

class PdfTiffWriter {
private:
	unsigned char tiffheader[MAX_TIFFHEADER];
	size_t additional_offset;
	size_t max_additional_data;
	size_t ifd_size;
public:
	PdfTiffWriter(int k, int cols = 1728, int rows = 0, bool blackis1 = 0, size_t size = 0);
	void addTag(uint16_t tag, uint32_t value, size_t size);
	void addStringTag(uint16_t tag, const std::string& value);
	std::string str(void) const;
};

#endif /* ndef __PDF_TIFF_WRITER_H */
