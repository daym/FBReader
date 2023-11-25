#include <iostream>
#include "Pdf.h"

shared_ptr<PdfObject> Pdf::findObject(const std::pair<int,int> address) {
	std::map<std::pair<int,int>,shared_ptr<PdfObject> >::iterator iter = myObjectMap.find(address);
	if(iter != myObjectMap.end())
		return iter->second;
	else
		return shared_ptr<PdfObject>();
}

void Pdf::addObject(const std::pair<int,int> address, shared_ptr<PdfObject> object) {
	if(!findObject(address).isNull()) {
		std::cerr << "error: duplicate object data for object (" << address.first << "," << address.second << ")!" << std::endl;
	}
	myObjectMap.insert(std::pair<std::pair<int,int>,shared_ptr<PdfObject> >(address, object));
}

void Pdf::ensureTrailer(shared_ptr<PdfObject> trailer) {
	if (myTrailer.isNull()) {
		myTrailer = trailer;
	}
}
