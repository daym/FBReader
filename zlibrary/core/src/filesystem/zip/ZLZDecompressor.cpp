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

#include <string.h>
#include <iostream>
#include <algorithm>

#include "../ZLInputStream.h"
#include "ZLZDecompressor.h"

const size_t IN_BUFFER_SIZE = 2048;
const size_t OUT_BUFFER_SIZE = 32768;

ZLZDecompressor::ZLZDecompressor(size_t size, bool useHeader) : myAvailableSize(size) {
	myZStream = new z_stream;
	memset(myZStream, 0, sizeof(z_stream));
	if(useHeader)
		inflateInit2(myZStream, MAX_WBITS);
	else
		inflateInit2(myZStream, -MAX_WBITS);

	myInBuffer = new char[IN_BUFFER_SIZE];
	myOutBuffer = new char[OUT_BUFFER_SIZE];
}

ZLZDecompressor::~ZLZDecompressor() {
	delete[] myInBuffer;
	delete[] myOutBuffer;

	inflateEnd(myZStream);
	delete myZStream;
}

size_t ZLZDecompressor::decompress(ZLInputStream &stream, char *buffer, size_t maxSize) {
	while ((myBuffer.length() < maxSize) && (myAvailableSize > 0)) {
		size_t size = std::min(myAvailableSize, (size_t)IN_BUFFER_SIZE);

		myZStream->next_in = (Bytef*)myInBuffer;
		myZStream->avail_in = stream.read(myInBuffer, size);
		if (myZStream->avail_in == size) {
			myAvailableSize -= size;
		} else {
			myAvailableSize = 0;
		}
		while (myZStream->avail_in == 0) {
			break;
		}
		while (myZStream->avail_in > 0) {
			myZStream->avail_out = OUT_BUFFER_SIZE;
			myZStream->next_out = (Bytef*)myOutBuffer;
			int code = ::inflate(myZStream, Z_SYNC_FLUSH);
			if ((code != Z_OK) && (code != Z_STREAM_END)) {
				break;
			}
			if (OUT_BUFFER_SIZE == myZStream->avail_out) {
				break;
			}
			myBuffer.append(myOutBuffer, OUT_BUFFER_SIZE - myZStream->avail_out);
			if (code == Z_STREAM_END) {
				myAvailableSize = 0;
				stream.seek(0 - myZStream->avail_in, false);
				break;
			}
		}
	}

	size_t realSize = std::min(maxSize, myBuffer.length());
	if (buffer != 0) {
		memcpy(buffer, myBuffer.data(), realSize);
	}
	myBuffer.erase(0, realSize);
	return realSize;
}
