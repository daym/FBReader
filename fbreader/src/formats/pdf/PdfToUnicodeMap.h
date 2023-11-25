#ifndef __PDF_TOUNICODE_MAP_H
#define __PDF_TOUNICODE_MAP_H
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include "PdfScanner.h"
#include "PdfToUnicodeMap.h"
#include "PdfDefaultCharMap.h"

#define MAX_TARGET 14
struct PdfToUnicodeMapChars {
	int count;
	unsigned int items[MAX_TARGET];
};
struct PdfToUnicodeMapRangeEntry {
	char nativePrefix[3];
	unsigned char nativePrefixSize;
	unsigned char nativeBeginning;
	unsigned char nativeEnd;
	struct PdfToUnicodeMapChars unicodeBeginnings;
};

class PdfToUnicodeMap {
private:
	std::vector<struct PdfToUnicodeMapRangeEntry> fRanges;
	//PdfToUnicodeMapAddress fAddress;
	unsigned int fPrefixlessCodes[256];
	size_t fPrefixlessCodesUsed;
public:
	PdfToUnicodeMap();
	struct PdfToUnicodeMapChars nextUnicodeCodepoint(unsigned char*& aNativeString, size_t& aNativeStringSize) const;
	void addRange(const std::string nativeBeginning, const std::string nativeEnd, std::vector<unsigned int> unicodeBeginning);
	size_t size() const;
	std::string convertStringToUnicode(const std::string& aValue) const;
};

#endif /* ndef __PDF_TOUNICODE_MAP_H */
