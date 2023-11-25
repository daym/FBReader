/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <iostream>
#include <sstream>
#include <fstream>

#include <ZLInputStream.h>
#include <ZLZDecompressor.h>
#include <ZLStringInputStream.h>
#include "PdfObject.h"
#include "PdfDepredictor.h"

PdfObject::~PdfObject() {
}

shared_ptr<PdfObject> PdfIntegerObject::integerObject(int value) {
	if ((value < 0) || (value >= 256)) {
		return new PdfIntegerObject(value);
	} else {
		static shared_ptr<PdfObject>* table = new shared_ptr<PdfObject>[256];
		if (table[value].isNull()) {
			table[value] = new PdfIntegerObject(value);
		}
		return table[value];
	}
}

PdfIntegerObject::PdfIntegerObject(int value) : myValue(value) {
	/*std::cerr << "PdfIntegerObject " << value << "\n"; yeah yeah */
}

int PdfIntegerObject::value() const {
	return myValue;
}

PdfObject::Type PdfIntegerObject::type() const {
	return INTEGER_NUMBER;
}

shared_ptr<PdfObject> PdfBooleanObject::TRUE() {
	static shared_ptr<PdfObject> value = new PdfBooleanObject(true);
	return value;
}

shared_ptr<PdfObject> PdfBooleanObject::FALSE() {
	static shared_ptr<PdfObject> value = new PdfBooleanObject(false);
	return value;
}

PdfBooleanObject::PdfBooleanObject(bool value) : myValue(value) {
	//std::cerr << "PdfBooleanObject " << value << "\n";
}

bool PdfBooleanObject::value() const {
	return myValue;
}

PdfObject::Type PdfBooleanObject::type() const {
	return BOOLEAN;
}

PdfStringObject::PdfStringObject(const std::string &value) : myValue(value) {
	/*std::cerr << "PdfStringObject " << value << "\n";*/
}

PdfObject::Type PdfStringObject::type() const {
	return STRING;
}

std::map<std::string,shared_ptr<PdfObject> > PdfNameObject::ourObjectMap;

shared_ptr<PdfObject> PdfNameObject::nameObject(const std::string &id) {
	// TODO: process escaped characters
	std::map<std::string,shared_ptr<PdfObject> >::const_iterator it = ourObjectMap.find(id);
	if (it != ourObjectMap.end()) {
		return it->second;
	}
	shared_ptr<PdfObject> object = new PdfNameObject(id);
	ourObjectMap.insert(std::make_pair(id, object));
	return object;
}

PdfNameObject::PdfNameObject(const std::string& id) : fID(id) {
}

PdfObject::Type PdfNameObject::type() const {
	return NAME;
}

PdfDictionaryObject::PdfDictionaryObject() {
}

void PdfDictionaryObject::setObject(shared_ptr<PdfObject> id, shared_ptr<PdfObject> object) {
	myMap[id] = object;
}

shared_ptr<PdfObject> PdfDictionaryObject::operator[](shared_ptr<PdfObject> id) const {
	std::map<shared_ptr<PdfObject>,shared_ptr<PdfObject> >::const_iterator it = myMap.find(id);
	if(it == myMap.end()) {
		//std::cerr << "warning: entry not found. Key was: " << ((PdfNameObject&) *id).id() << std::endl;
	}
	return (it != myMap.end()) ? it->second : 0;
}
shared_ptr<PdfObject>& PdfDictionaryObject::operator[](shared_ptr<PdfObject> id) {
	std::map<shared_ptr<PdfObject>,shared_ptr<PdfObject> >::const_iterator it = myMap.find(id);
	if(it == myMap.end()) {
		myMap[id] = shared_ptr<PdfObject>();
	}
	return(myMap[id]);
}

std::list<shared_ptr<PdfObject> > PdfDictionaryObject::keys() const {
	std::list<shared_ptr<PdfObject> > result;
	
	for(std::map<shared_ptr<PdfObject>,shared_ptr<PdfObject> >::const_iterator it = myMap.begin(); it != myMap.end(); ++it) {
		result.push_back(it->first);
	}
	return result;
}

shared_ptr<PdfObject> PdfDictionaryObject::operator[](const std::string &id) const {
	return operator[](PdfNameObject::nameObject(id));
}

PdfObject::Type PdfDictionaryObject::type() const {
	return DICTIONARY;
}

void PdfDictionaryObject::dump() const {
	std::cerr << "PdfDictionaryObject::dump() => [" << std::endl;
	std::list<shared_ptr<PdfObject> > fKeys = keys();
	for(std::list<shared_ptr<PdfObject> >::const_iterator it = fKeys.begin(); it != fKeys.end(); ++it) {
		shared_ptr<PdfObject> name = *it;
		std::cerr << ((PdfNameObject&)*name).id() << std::endl;
	}
	std::cerr << "]" << std::endl;
}

PdfArrayObject::PdfArrayObject() {
}

void PdfArrayObject::addObject(shared_ptr<PdfObject> object) {
	myVector.push_back(object);
}

shared_ptr<PdfObject> PdfArrayObject::popLast() {
	if (!myVector.empty()) {
		shared_ptr<PdfObject> last = myVector.back();
		myVector.pop_back();
		return last;
	}
	return 0;
}

int PdfArrayObject::size() const {
	return myVector.size();
}

shared_ptr<PdfObject> PdfArrayObject::operator[](int index) const {
	return myVector[index];
}
shared_ptr<PdfObject>& PdfArrayObject::operator[](int index) {
	return myVector[index];
}

PdfObject::Type PdfArrayObject::type() const {
	return ARRAY;
}

PdfObjectReference::PdfObjectReference(int number, int generation) : myNumber(number), myGeneration(generation) {
}

int PdfObjectReference::number() const {
	return myNumber;
}

int PdfObjectReference::generation() const {
	return myGeneration;
}

PdfObject::Type PdfObjectReference::type() const {
	return REFERENCE;
}

shared_ptr<PdfObject> PdfStreamObject::dictionary() const {
	return myDictionary;
}
void PdfStreamObject::setDictionary(shared_ptr<PdfObject> value) {
	myDictionary = value;
}

shared_ptr<ZLInputStream> PdfStreamObject::stream() const {
	/*std::cerr << "requested stream for data:" << std::endl;
	std::cerr << myData << std::endl;
	std::cerr << "END: requested stream for data." << std::endl;*/
	//return new std::istringstream(myData);
	return new ZLStringInputStream(myData);
}
void PdfStreamObject::processDecompressor(shared_ptr<PdfObject> filter, ZLInputStream& originalStream) {
	if (filter == PdfNameObject::nameObject("FlateDecode")) {
		ZLZDecompressor decompressor(myData.length(), true);
		ZLStringInputStream dataStream(myData);
		myData = "";
		char buffer[2048];
		while (true) {
			size_t size = decompressor.decompress(dataStream, buffer, 2048);
			if (size == 0)
				break;
			myData.append(buffer, size);
		}
	}
}
void PdfStreamObject::processDecompressorChain(shared_ptr<PdfObject>& filters, ZLInputStream& originalStream) {
	if(!filters.isNull() && filters->type() == PdfObject::ARRAY) {
		PdfArrayObject& items = (PdfArrayObject&) *filters;
		int size = items.size();
		for(int i = 0; i < size; ++i) {
			shared_ptr<PdfObject> item = items[i];
			processDecompressor(item, originalStream);
		}
	} else
		processDecompressor(filters, originalStream);
}
void PdfStreamObject::dump() {
	std::cout << "dict" << std::endl;
	if(!myDictionary.isNull()) {
		PdfDictionaryObject& dict = (PdfDictionaryObject&) *myDictionary;
		dict.dump();
	}
	std::ofstream ofs("Q", std::ios::out | std::ios::app | std::ios::binary);
	ofs << myData;
}
PdfStreamObject::PdfStreamObject(shared_ptr<PdfObject> dictionaryO, ZLInputStream &dataStream, char& ch, size_t size) {
	mySize = size;
	myDictionary = dictionaryO;
	dataStream.seek(-1, false);
	ch = 0;
	myData.append(mySize, '\0'); /* help debugging */
	/*myData[0] = ch;
	dataStream.read((char*)myData.data() + 1, mySize - 1);*/
	dataStream.read((char*) myData.data(), mySize);

	int value = size;
	if (value > 0) {
		PdfDictionaryObject& dictionary = (PdfDictionaryObject&)(*dictionaryO);
		shared_ptr<PdfObject> filters = dictionary["Filter"];
		processDecompressorChain(filters, dataStream);
		shared_ptr<PdfObject> DecodeParmsO = dictionary["DecodeParms"];
		if(!DecodeParmsO.isNull() && DecodeParmsO->type() == PdfObject::DICTIONARY) {
			int columns = 0;
			int predictor = 0;
			PdfDictionaryObject& DecodeParms = (PdfDictionaryObject&) *DecodeParmsO;
			integerFromPdfObject(DecodeParms["Predictor"], predictor);
			integerFromPdfObject(DecodeParms["Columns"], columns);
			if(predictor) {
				unsigned char* data = (unsigned char*) myData.data();
				depredict(columns, predictor, data, myData.length()); /* will depredict in-place */
			}
		}
	} else {
		//std::cerr << "error: could not determine substream size." << std::endl;
	}
}

PdfObject::Type PdfStreamObject::type() const {
	return STREAM;
}




shared_ptr<PdfObject> PdfRealObject::realObject(double value) {
	/*if (value != 0.0) {*/
		return new PdfRealObject(value);
	/*} else {
		static shared_ptr<PdfObject>* table = new shared_ptr<PdfObject>[256];
		if (table[(int) value].isNull()) {
			table[(int) value] = new PdfRealObject(value);
		}
		return table[(int) value];
	}*/
}

PdfRealObject::PdfRealObject(double value) : myValue(value) {
	/*std::cerr << "PdfRealObject " << value << "\n"; yeah yeah */
}

double PdfRealObject::value() const {
	return myValue;
}

PdfObject::Type PdfRealObject::type() const {
	return REAL_NUMBER;
}

