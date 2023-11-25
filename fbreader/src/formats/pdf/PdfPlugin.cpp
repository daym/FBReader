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
#include <ZLFile.h>
#include <ZLInputStream.h>

#include "PdfPlugin.h"
#include "PdfDescriptionReader.h"
#include "PdfBookReader.h"
#include "../../library/Book.h"
#include "../../bookmodel/BookModel.h"

bool PdfPlugin::acceptsFile(const ZLFile &file) const {
	if(file.extension() == "pdf")
		return true;
	shared_ptr<ZLInputStream> inputStream = file.inputStream();
	if(inputStream.isNull() || !inputStream->open())
		return false;
	char buffer[5];
	if(inputStream->read(buffer, 5) < 5)
		return false;
	return (memcmp(buffer, "%PDF-", 5) == 0);
}

bool PdfPlugin::readMetaInfo(Book &book) const {
	return PdfDescriptionReader(book).readMetaInfo(book.file().inputStream());
}

bool PdfPlugin::readModel(BookModel &model) const {
	const Book &book = *model.book();
	return PdfBookReader(model).readBook(book.file().inputStream());
}
