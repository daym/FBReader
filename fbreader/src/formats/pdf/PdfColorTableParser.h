#ifndef __PDF_COLOR_TABLE_PARSER_H
#define __PDF_COLOR_TABLE_PARSER_H
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include "PdfScanner.h"
#include "PdfColorTable.h"

class PdfColorTableParser : public PdfScanner {
private:
	int fComponentCount;
	PdfColorTable& fTable;
protected:
	bool parse_();
public:
	PdfColorTableParser(shared_ptr<ZLInputStream> stream, int componentCount, PdfColorTable& table);
	PdfColorTableParser(const std::string& str, int componentCount, PdfColorTable& table);
	bool parse();
	PdfColorTable& table() const { return fTable; }
};

#endif /* ndef __PDF_COLOR_TABLE_PARSER_H */
