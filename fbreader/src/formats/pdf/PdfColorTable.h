#ifndef __PDF_COLOR_TABLE_H
#define __PDF_COLOR_TABLE_H
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include "PdfScanner.h"
#include "PdfToUnicodeMap.h"
#include "PdfDefaultCharMap.h"

struct PdfColorTableEntry {
	unsigned char components[4];
	PdfColorTableEntry() {
		components[0] = 0;
		components[1] = 0;
		components[2] = 0;
		components[3] = 0;
	}
};
class PdfColorTable {
private:
	std::vector<PdfColorTableEntry> fEntries;
public:
	PdfColorTable();
	size_t size() const;
	const PdfColorTableEntry operator[](int index) const;
	void set(int index, PdfColorTableEntry entry);
};

#endif /* ndef __PDF_COLOR_TABLE_H */
