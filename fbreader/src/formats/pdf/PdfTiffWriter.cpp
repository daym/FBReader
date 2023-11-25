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
#include <iostream>
#include <string.h>
#include "PdfTiffWriter.h"

static unsigned char basic_header[] = "II\x2A\x00\x08\x00\x00\x00\x00\x00";
#define BASIC_HEADER_SIZE 10

static unsigned int tag_count = 10;
//class PdfTiffWriter {
PdfTiffWriter::PdfTiffWriter(int k, int cols, int rows, bool blackis1, size_t size) {
	max_additional_data = 24;
	ifd_size = 10 + (12 * tag_count) + 4;
	size_t tiffheader_size = ifd_size + max_additional_data;
	if(tiffheader_size > MAX_TIFFHEADER) {
		//throw std::runtime_error("TIFF header too large for buffer");
	}
	memset(tiffheader, 0, MAX_TIFFHEADER);
	for(int i = 0; i < BASIC_HEADER_SIZE; ++i)
		this->tiffheader[i] = basic_header[i];
	this->additional_offset = ifd_size;
	int comptype = 3; // T4
	int t4options = 0; // 1D or 2D T4?
	if(k < 0)
		comptype = 4;
	else if(k > 0) {
		comptype = 3;
		t4options = 1;
	} // else leave T4 1D
	// TODO maybe fall back to Height for rows.
	// keep sorted by tag.
	addTag(256, cols, 4);
	addTag(257, rows, 4);
	addTag(259, comptype, 2);
	addTag(262, blackis1, 2); // Photometric Interpretation
	addTag(273, tiffheader_size, 4); // Offset to start of image data - updated below
	addTag(279, size, 4); // Length of image data
	//self.addTag(282, 300, 1) # X Resolution 300 (default unit Inches) # TODO rational
	//self.addTag(283, 300, 1) # Y Resolution 300 (default unit Inches) # TODO rational
	if (comptype == 3)
		addTag(292, t4options, 4); // 32 flag bits.
	addStringTag(305, "FBReader"); // generated-by-software
}
void PdfTiffWriter::addTag(uint16_t tag, uint32_t x_value, size_t size) {
	// size is in bytes
	tiffheader[8] += 1;
	size_t count = tiffheader[8];
	if(count >= tag_count) { // too many
		return;
	}
	size_t offset = (count - 1)*12 + 10;
	tiffheader[offset] = tag & 0xFF;
	tiffheader[offset + 1] = (tag >> 8) & 0xFF;
	tiffheader[offset + 2] = (size == 2) ? 3 : (size == 4) ? 4 : 0;
	tiffheader[offset + 3] = 0; // TODO what's this?
	tiffheader[offset + 4] = 1; // array item count
	// Tiff types: 1=byte, 2=ascii, 3=short, 4=ulong 5=rational
	tiffheader[offset + 8] = x_value & 0xFF;
	tiffheader[offset + 9] = (x_value >> 8) & 0xFF;
	tiffheader[offset + 10] = (x_value >> 16) & 0xFF;
	tiffheader[offset + 11] = (x_value >> 24) & 0xFF;
}
void PdfTiffWriter::addStringTag(uint16_t tag, const std::string& value) {
	// size is in bytes
	tiffheader[8] += 1;
	size_t count = tiffheader[8];
	if(count >= tag_count) { // too many
		return;
	}
	size_t offset = (count - 1)*12 + 10;
	tiffheader[offset] = tag & 0xFF;
	tiffheader[offset + 1] = (tag >> 8) & 0xFF;
	tiffheader[offset + 2] = 2;
	tiffheader[offset + 3] = 0; // TODO what's this?
	tiffheader[offset + 4] = value.length(); // array item count
	uint32_t x_value = additional_offset;
	// Tiff types: 1=byte, 2=ascii, 3=short, 4=ulong 5=rational
	tiffheader[offset + 8] = x_value & 0xFF;
	tiffheader[offset + 9] = (x_value >> 8) & 0xFF;
	tiffheader[offset + 10] = (x_value >> 16) & 0xFF;
	tiffheader[offset + 11] = (x_value >> 24) & 0xFF;
	// string
	if(additional_offset + value.length() <= ifd_size + max_additional_data) {
		for(int i = additional_offset, j = 0, e = additional_offset + value.length(); i < e; ++i, ++j)
			tiffheader[i] = value[i];
		additional_offset += value.length();
	} else { // ooops
		std::cerr << "PdfTiffWriter: I don't have enough space in my little buffer" << std::endl;
	}
}
std::string PdfTiffWriter::str(void) const {
	return(std::string((const char*) tiffheader, ifd_size + max_additional_data));
}
