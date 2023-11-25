#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include "PdfDefaultCharMap.h"
#include "parseDefaultCharName.h"

/** \return cNilCodepoint if not found. */
unsigned int getUnicodeFromDefaultCharMap(const std::string& key) {
	const char* x_name = key.c_str();
	unsigned int unicode = parseDefaultCharName(x_name);
	if(unicode)
		return unicode;
	else {
		/* note: it seems that with Type 3 symbol "fonts" (which uses Pdf XObjects as glyphs), the names are dummy names that are to map directly to their own code. 
		   Since this is not unicode, we don't do it anyway. */
		std::string readableKey = key;
		if(readableKey.length() == 1 && readableKey[0] < 32) {
			std::stringstream x;
			x << "(code " << (unsigned int) (unsigned char) readableKey[0] << ")";
			readableKey = x.str();
		}
		std::cerr << "PdfDefaultCharMap: entry not found: " << readableKey << std::endl;
		return cNilCodepoint;
	}
}

