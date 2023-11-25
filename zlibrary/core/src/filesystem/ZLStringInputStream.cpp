/*
 * Copyright (C) 2008-2010 Geometer Plus <contact@geometerplus.com>
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
#include <algorithm>

#include "ZLStringInputStream.h"

ZLStringInputStream::ZLStringInputStream(const std::string &data) : myData(data), myOffset(0) {
}

bool ZLStringInputStream::open() {
	myOffset = 0;
	return true;
}

size_t ZLStringInputStream::read(char *buffer, size_t maxSize) {
	size_t size = std::min(maxSize, myData.length() - myOffset);
	memcpy(buffer, myData.data() + myOffset, size);
	myOffset += size;
	return size;
}

void ZLStringInputStream::close() {
}

void ZLStringInputStream::seek(int offset, bool absoluteOffset) {
	if (!absoluteOffset) {
		offset += myOffset;
	}
	myOffset = std::min((size_t)std::max(0, offset), myData.length());
}

size_t ZLStringInputStream::offset() const {
	return myOffset;
}

size_t ZLStringInputStream::sizeOfOpened() {
	return myData.length();
}
