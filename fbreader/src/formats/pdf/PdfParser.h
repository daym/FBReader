#ifndef __PDF_PARSER_H
#define __PDF_PARSER_H
#include <map>
#include <vector>
#include <shared_ptr.h>
#include <ZLInputStream.h>

class Pdf;
//#include "Pdf.h"

class PdfObject;
class PdfStreamObject;
typedef std::vector<std::pair<int, shared_ptr<PdfObject> > > PdfCompressedObjects;

class PdfParser {
protected:
	bool readReferenceTable(int offset);
	shared_ptr<PdfObject> readObjectFromLocation(const std::pair<int,int> &address);
protected:
	void readToken(ZLInputStream& inputStream, std::string &buffer);
	void skipWhiteSpaces(ZLInputStream& inputStream);

private:
	std::string myBuffer;
	std::map<std::pair<int,int>,int> myObjectLocationMap; // either it's in there.
	std::map<std::pair<int,int>,std::pair<int,int> > myCompressedObjectLocationMap; // or it's in there, not both. // (ID, generation) -> (referenced_ID, index_within)
	shared_ptr<ZLInputStream> myInputStream;
	Pdf& myPDF;
	char ch;
	std::map<std::pair<int,int>, shared_ptr<PdfCompressedObjects> > myCompressedObjects;
public /* almost private */:
	shared_ptr<PdfObject> readObject(shared_ptr<ZLInputStream> inputStream);
public:
	shared_ptr<PdfObject> resolveReference(shared_ptr<PdfObject> reference);
public:
	PdfParser(shared_ptr<ZLInputStream> source, Pdf& destination);
	void parse();
protected:
	PdfCompressedObjects parseObjStm(PdfStreamObject& objStm);
	shared_ptr<PdfObject> getObjStmEntry(int objStmID, int objStmGeneration, int referencedIndex, int referencedID);
	shared_ptr<PdfObject> readTraditionalXREF();
	shared_ptr<PdfObject> readV5XREF();
	shared_ptr<PdfObject> readCompressedObjectFromLocation(const std::pair<int,int> &address);
private:
	std::string readLine(void);
	int readOctalNumber(ZLInputStream& inputStream);
};

#endif /* ndef __PDF_PARSER_H */
