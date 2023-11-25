#ifndef __PDF_TYPE1FONTFILE_PARSER_H
#define __PDF_TYPE1FONTFILE_PARSER_H
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include "PdfScanner.h"
#include "PdfToUnicodeMap.h"

#define cNilCodepoint 0xFFFD

class PdfType1FontFileParser : public PdfScanner {
private:
	PdfToUnicodeMap& fMap;
protected:
	void parse_();
	void parseCharset();
	std::string parseValue();
public:
	PdfType1FontFileParser(shared_ptr<ZLInputStream> stream, PdfToUnicodeMap& map); // , const PdfToUnicodeMapAddress& address);
	bool parse();
	PdfToUnicodeMap& map() const { return fMap; }
};

#endif /* ndef __PDF_TYPE1FONTFILE_PARSER_H */
