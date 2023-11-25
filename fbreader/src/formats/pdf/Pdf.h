#ifndef __PDF_H
#define __PDF_H
#include <map>
#include <shared_ptr.h>
#include "PdfObject.h"

// main data class for the PDF file.
class Pdf {
private:
	shared_ptr<PdfObject> myTrailer;
	std::map<std::pair<int,int>,shared_ptr<PdfObject> > myObjectMap;
public:
	shared_ptr<PdfObject> trailer() const {
		return myTrailer;
	}
	shared_ptr<PdfObject> findObject(const std::pair<int,int> address);
	void addObject(const std::pair<int,int> address, shared_ptr<PdfObject> object);
	void ensureTrailer(shared_ptr<PdfObject> trailer);
};
#endif /* ndef __PDF_H */
