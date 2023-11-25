#ifndef __PDF_CFF_PARSER_H
#define __PDF_CFF_PARSER_H
#include "PdfCffMap.h"
#include "PdfScanner.h"

/* CFF font (FontFile3) parser */

class PdfCffParser : public PdfScanner {
public:
	PdfCffParser(shared_ptr<ZLInputStream> stream, PdfCffMap& map);
	void parse();
};

#endif /* ndef __PDF_CFF_PARSER_H */
