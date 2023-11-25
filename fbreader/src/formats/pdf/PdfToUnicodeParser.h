#ifndef __PDF_TOUNICODE_PARSER_H
#define __PDF_TOUNICODE_PARSER_H
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include "PdfScanner.h"

typedef std::pair<int,int> PdfToUnicodeMapAddress;
/*address(reference.number(), reference.generation());*/

class PdfToUnicodeParser : public PdfScanner {
private:
	PdfToUnicodeMap& fMap;
protected:
	void parseRangeBody();
	void parseCharBody();
	void parse_();
	std::string parseValue();
	unsigned int parseCharsetEntry();
public:
	PdfToUnicodeParser(shared_ptr<ZLInputStream> stream, PdfToUnicodeMap& map);
	void parse();
};

#endif /* ndef __PDF_TOUNICODE_PARSER_H */
