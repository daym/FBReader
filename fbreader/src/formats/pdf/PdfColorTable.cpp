/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <assert.h>
#include <string.h>
#include "PdfColorTable.h"

PdfColorTable::PdfColorTable() {
}
size_t PdfColorTable::size() const {
	return(fEntries.size());
}
const PdfColorTableEntry PdfColorTable::operator[](int index) const {
	return(fEntries[index]);
}
void PdfColorTable::set(int index, PdfColorTableEntry entry) {
	if(index > 255) {
		std::cerr << "warning: the PDF standard specifies a max size of 256 entries." << std::endl;
	}
	if(index < 0) {
		std::cerr << "error: index into palette out of range." << std::endl;
		return;
	}
	while(size() <= index)
		fEntries.push_back(PdfColorTableEntry());
	fEntries[index] = entry;
}
