/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <assert.h>
#include <sstream>
#include "PdfColorTableParser.h"
#include "PdfColorTable.h"

bool PdfColorTableParser::parse() {
	consume();
	if(input == EOF) {
		return false;
	}
	return parse_();
}
bool PdfColorTableParser::parse_() {
	int index = 0;
	while(input != EOF) {
		PdfColorTableEntry entry;
		for(int j = 0; j < fComponentCount; ++j)
			entry.components[j] = (unsigned char) consume(); /* TODO what if EOF happens in the middle? */
		fTable.set(index, entry);
		++index;
	}
	return(true);
}
PdfColorTableParser::PdfColorTableParser(shared_ptr<ZLInputStream> stream, int componentCount, PdfColorTable& table) : PdfScanner(stream), fComponentCount(componentCount), fTable(table) {
	PdfColorTableEntry entry;
	if(fComponentCount > sizeof(entry.components)/sizeof(entry.components[0]))
		abort();
}
PdfColorTableParser::PdfColorTableParser(const std::string& str, int componentCount, PdfColorTable& table) : PdfScanner(new ZLStringInputStream(str)), fComponentCount(componentCount), fTable(table) {
	PdfColorTableEntry entry;
	if(fComponentCount > sizeof(entry.components)/sizeof(entry.components[0]))
		abort();
}
