#ifndef __PDF_CONTENT_PARSER_H
#define __PDF_CONTENT_PARSER_H
#include <list>
#include "PdfObject.h"
#include "PdfScanner.h"
#include "PdfContent.h"

class PdfContentParser : public PdfScanner {
private:
	shared_ptr<PdfContent> myContent;
	std::list<shared_ptr<PdfObject> > mySources;
	std::list<shared_ptr<PdfObject> >::const_iterator mySourcesIter;
protected /* parser */:
	shared_ptr<PdfInstruction> parseInstruction();
	PdfOperator parseOperator();
	shared_ptr<PdfObject> parseTextBlockBodyE(); /* actually returns PdfContent */
	shared_ptr<PdfObject> parseInlineImageBody();
	shared_ptr<PdfObject> parseValueOrBracedValueList();
	shared_ptr<PdfObject> parseValue();
	shared_ptr<PdfObject> parseSquareBracedValueList(); /* actually returns PdfArrayObject */
	virtual shared_ptr<ZLInputStream> goToNextStream();
	PdfContentParser(std::list<shared_ptr<PdfObject> >& sources, shared_ptr<PdfContent> output);
	shared_ptr<PdfObject> parseAttributesBody(bool BAllowObjectReferences = true, int delimiter = '>');
	shared_ptr<PdfObject> parseAttribute(bool BAllowObjectReferences, std::string& key);
public:
	static shared_ptr<PdfContent> parse(std::list<shared_ptr<PdfObject> >& sources);
};

#endif /* ndef __PDF_CONTENT_PARSER_H */
