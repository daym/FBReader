#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <ZLStringUtil.h>
#include "PdfParser.h"
#include "PdfObject.h"
#include "Pdf.h"

#define GETC(stream, ch) { if(stream.read(&ch, 1) != 1) ch = 0; }
#define EGETC(stream, ch) { \
		if (stream.read(&ch, 1) != 1) { \
			std::cerr << "warning: unexpected EOF"  << std::endl; \
			abort(); \
			break; \
		} \
}

enum PdfCharacterType {
	PDF_CHAR_REGULAR,
	PDF_CHAR_WHITESPACE,
	PDF_CHAR_DELIMITER
};

static PdfCharacterType *PdfCharacterTypeTable = 0;

shared_ptr<PdfObject> PdfParser::readCompressedObjectFromLocation(const std::pair<int,int> &address) {
	std::map<std::pair<int,int>,std::pair<int,int> >::const_iterator jtCompressed = myCompressedObjectLocationMap.find(address);
	if(jtCompressed == myCompressedObjectLocationMap.end() || address.second != 0) // not found || invalid argument.
		return 0;
		
	return getObjStmEntry(jtCompressed->second.first, 0, jtCompressed->second.second, address.first);
}

shared_ptr<PdfObject> PdfParser::readObjectFromLocation(const std::pair<int,int> &address) {
	ZLInputStream& inputStream = *myInputStream;
	std::map<std::pair<int,int>,int>::const_iterator jt = myObjectLocationMap.find(address);
	if (jt == myObjectLocationMap.end()) {
		char old_ch = ch;
		shared_ptr<PdfObject> result = readCompressedObjectFromLocation(address);
		ch = old_ch;
		return result;
	}
	inputStream.seek(jt->second, true);
	ch = 0;
	readToken(inputStream, myBuffer);
	if (address.first != atoi(myBuffer.c_str())) {
		return 0;
	}
	readToken(inputStream, myBuffer);
	if (address.second != atoi(myBuffer.c_str())) {
		return 0;
	}
	readToken(inputStream, myBuffer);
	if (myBuffer != "obj") {
		return 0;
	}
	//std::cerr << "note: obj addr " << address.first << std::endl;
	if(address.first == 15745) {
		std::cerr << "!" << std::endl;
	}
	return readObject(myInputStream);
}

shared_ptr<PdfObject> PdfParser::resolveReference(shared_ptr<PdfObject> ref) {
	// since even the /XRef itself contains references, ONLY lazy-resolving can work (sanely).
	if (ref.isNull() || (ref->type() != PdfObject::REFERENCE)) {
		return ref;
	}
	const PdfObjectReference &reference = (const PdfObjectReference&)*ref;
	//std::cerr << "resolving reference " << reference.number() << std::endl;
	const std::pair<int,int> address(reference.number(), reference.generation());
	shared_ptr<PdfObject> object = myPDF.findObject(address);
	if(!object.isNull())
		return object;
	//std::map<std::pair<int,int>,int>::const_iterator jt = myObjectLocationMap.find(address);
	off_t position = myInputStream->offset();
	object = readObjectFromLocation(address);
	myInputStream->seek(position, true);
	myPDF.addObject(address, object);
	return object;
}

static void stripBuffer(std::string &buffer) {
	int index = buffer.find('%');
	if (index >= 0) {
		buffer.erase(index);
	}
	ZLStringUtil::stripWhiteSpaces(buffer);
}

std::string PdfParser::readLine(void) {
	std::string buffer;
	ZLInputStream& stream = *myInputStream;
	//buffer.clear();
	//char ch;
	while (1) {
		if ((ch == 10) || (ch == 13)) {
			//if (!buffer.empty()) {
			//}
			GETC(stream, ch);
			if(ch == 10 || ch == 13)
				GETC(stream, ch);
			break;
		} else {
			buffer += ch;
		}
		EGETC(stream, ch)
	}
	return buffer;
}

shared_ptr<PdfObject> PdfParser::readTraditionalXREF() {
		ZLInputStream& inputStream = *myInputStream;
		while (true) {
			myBuffer = readLine();
			stripBuffer(myBuffer);
			if (myBuffer == "trailer") {
				break;
			}
			const int index = myBuffer.find(' ');
			const int start = atoi(myBuffer.c_str());
			const int len = atoi(myBuffer.c_str() + index + 1);
			for (int i = 0; i < len; ++i) {
				myBuffer = readLine();
				stripBuffer(myBuffer);
				if (myBuffer.length() != 18) {
					return shared_ptr<PdfObject>();
				}
				const int objectOffset = atoi(myBuffer.c_str());
				const int objectGeneration = atoi(myBuffer.c_str() + 11);
				const bool objectInUse = myBuffer[17] == 'n';
				if (objectInUse) {
					myObjectLocationMap[std::pair<int,int>(start + i, objectGeneration)] = objectOffset;
				}
			}
		}
		//ch = 0;
		shared_ptr<PdfObject> trailer = readObject(myInputStream);
		if (trailer.isNull() || (trailer->type() != PdfObject::DICTIONARY)) {
			return shared_ptr<PdfObject>();
		}
		myPDF.ensureTrailer(trailer);
		PdfDictionaryObject &trailerDictionary = (PdfDictionaryObject&)*trailer;
		shared_ptr<PdfObject> previous = trailerDictionary["Prev"];
		return previous;
}

static unsigned int readBinaryInteger(ZLInputStream& stream, size_t sizeInBytes, bool& didAnything) {
	unsigned char buffer[4] = {0};
	assert(sizeInBytes <= 4);
	assert(sizeof(int) >= 4);
	if(stream.read((char*) buffer, sizeInBytes) != sizeInBytes) { // ERROR
		return 0;
	}
	if(sizeInBytes > 0)
		didAnything = true;
	unsigned int result = 0;
	for(int i = 0; i < sizeInBytes; ++i) {
		result = (result << 8) | buffer[i];
	}
	return result;
}

shared_ptr<PdfObject> PdfParser::readV5XREF() {
	ZLInputStream& inputStream = *myInputStream;
	shared_ptr<PdfObject> previous;
	shared_ptr<PdfObject> xrefO = readObject(myInputStream); // hello chicken-and-egg problem.
	// after readToken() == "obj";
	if(xrefO.isNull() || xrefO->type() != PdfObject::STREAM) {
		return previous;
	}
	PdfStreamObject& xref = (PdfStreamObject&) *xrefO;
	shared_ptr<PdfObject> dictionaryO = xref.dictionary();
	if(dictionaryO.isNull())
		return previous;
	myPDF.ensureTrailer(dictionaryO);
	PdfDictionaryObject& dictionary = (PdfDictionaryObject&) *dictionaryO;
	//dictionary.dump();
	previous = dictionary["Prev"]; // optional.
	int Size = 0;
	shared_ptr<PdfObject> WO = dictionary["W"];
	if(WO.isNull() || WO->type() != PdfObject::ARRAY)
		return previous;
	PdfArrayObject& WAO = (PdfArrayObject&) *WO;
	int Ws[10];
	int W_count = WAO.size();
	if(W_count > 10)
		W_count = 10;
	for(int i = 0; i < W_count; ++i) {
		shared_ptr<PdfObject> itemO = WAO[i];
		if(itemO.isNull() || !integerFromPdfObject(itemO, Ws[i])) // ???
			Ws[i] = 0;
	}
	
	if(!integerFromPdfObject(dictionary["Size"], Size)) // same meaning as in traditional trailer.
		return previous;
	shared_ptr<PdfObject> indexO = dictionary["Index"];
	std::vector<std::pair<int, int> > indices;
	if(!indexO.isNull() && indexO->type() == PdfObject::ARRAY) {
		PdfArrayObject& indexA = (PdfArrayObject&)*indexO;
		int indexSize = indexA.size();
		for(int i = 0; i < indexSize - 1; i += 2) {
			int beginning = 0;
			int length = 0;
			if(!integerFromPdfObject(indexA[i], beginning) || !integerFromPdfObject(indexA[i+1], length)) { // ERROR
				return previous;
			}
			indices.push_back(std::make_pair(beginning, length));
		}
	} else
		indices.push_back(std::make_pair(0, Size));
	shared_ptr<ZLInputStream> streamP = xref.stream();
	if(streamP.isNull())
		return previous;
	ZLInputStream& stream = *streamP;
	bool didAnything = false;
	if(indices.size() < 1) // ERROR
		return previous;
	didAnything = true;
	for(int ID = indices[0].first; didAnything; ) {
		didAnything = false;
		int V[10] = {4711, 0};
		for(int i = 0; i < W_count; ++i) {
			int W = Ws[i];
			if(W > 0) {
				V[i] = readBinaryInteger(stream, W, didAnything);
			} else { // TODO actually there's only one default 0, which is the generation number for type=1. Maybe not do too much?
				V[i] = 0;
			}
			//std::cerr << "XREFV5: " << V[i] << ' ';
		}
		//std::cerr <<  std::endl;
		if(!didAnything) // safety
			break;
		if(V[0] == 0) { // fields: [type, object_next_free, next_generation_number] (like 'f).
		} else if(V[0] == 1) { // fields: [type, offset, generation_number] (like 'n).
			myObjectLocationMap[std::pair<int,int>(ID, V[2]/*generation*/)] = V[1]/*offset*/;
			//std::cout << "object " << ID << " known to be uncompressed." << std::endl;
		} else if(V[0] == 2) { // fields: [type, object, index_in_stream] (generation implicitly 0).
			myCompressedObjectLocationMap[std::pair<int,int>(ID, 0)] = std::make_pair(V[1] /* referenced object */, V[2] /* index in referenced object */);
			//std::cout << "object " << ID << " known to be compressed within object " << V[1] << "." << std::endl;
		}
		++ID;
		indices[0] = std::make_pair(indices.front().first, indices.front().second - 1);
		if(indices.front().second == 0) { // count==0.
			indices.erase(indices.begin()); // pop front
			if(indices.size() < 1)
				break;
			ID = indices[0].first;
		}
	}
	return previous;
}

bool PdfParser::readReferenceTable(int xrefOffset) {
	ZLInputStream& inputStream = *myInputStream;
	while (true) {
		inputStream.seek(xrefOffset, true);
		ch = 0;
		readToken(inputStream, myBuffer);
		shared_ptr<PdfObject> previous;
		// TODO what about leading whitespace?
		if (myBuffer == "xref") {
			myBuffer = readLine(); // just the newline, usually.
			//stripBuffer(myBuffer);
			previous = readTraditionalXREF();
		} else {
			skipWhiteSpaces(inputStream);
			readToken(inputStream, myBuffer); // number
			skipWhiteSpaces(inputStream);
			readToken(inputStream, myBuffer); // obj
			skipWhiteSpaces(inputStream);
			if(myBuffer == "obj")
				previous = readV5XREF();
			else
				return false;
		}

		if(!integerFromPdfObject(previous, xrefOffset))
			return true;
	}
}


int PdfParser::readOctalNumber(ZLInputStream& inputStream) { /* used for \ escapes in strings, so we know that after it is not EOF */
	int result = 0;
	while(ch >= '0' && ch <= '9') {
		result *= 8;
		result += ch - '0';
		EGETC(inputStream, ch);
	}
	return(result);
}
shared_ptr<PdfObject> PdfParser::readObject(shared_ptr<ZLInputStream> inputStreamO) {
	ZLInputStream& inputStream = *inputStreamO;
	skipWhiteSpaces(inputStream);
	int nestingLevel = 0;

	PdfObject::Type type = PdfObject::NIL;
	bool hexString = false;
	switch (ch) {
		case '(':
			GETC(inputStream, ch);
			hexString = false;
			type = PdfObject::STRING;
			break;
		case '<':
			GETC(inputStream, ch);
			hexString = true;
			type = (ch == '<') ? PdfObject::DICTIONARY : PdfObject::STRING;
			break;
		case '>': // end of dictionary
			GETC(inputStream, ch);
			if (ch == '>') {
				GETC(inputStream, ch);
			}
			return 0;
		case '/':
			type = PdfObject::NAME;
			break;
		case '[':
			type = PdfObject::ARRAY;
			break;
		case ']': // end of array
			GETC(inputStream, ch);
			return 0;
		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			type = PdfObject::INTEGER_NUMBER;
			break;
		case 't':
		case 'f':
			type = PdfObject::BOOLEAN;
			break;
	}

	switch (type) {
		case PdfObject::DICTIONARY:
		{
			ch = 0;
			shared_ptr<PdfObject> name;
			shared_ptr<PdfObject> value;
			shared_ptr<PdfObject> next;
			PdfDictionaryObject *dictionary = new PdfDictionaryObject();
			while (true) {
				next = readObject(inputStreamO);
				if (next.isNull()) {
					break;
				}
				PdfObject::Type oType = next->type();
				if (oType == PdfObject::NAME) {
					name = next;
					value = readObject(inputStreamO);
					if (value.isNull()) {
						break;
					}
					dictionary->setObject(name, value);
					skipWhiteSpaces(inputStream);
				} else if (oType == PdfObject::INTEGER_NUMBER) {
					if (value.isNull() || (value->type() != PdfObject::INTEGER_NUMBER)) {
						break;
					}
					skipWhiteSpaces(inputStream);
					if (ch != 'R') {
						break;
					}
					const int number = ((PdfIntegerObject&)*value).value();
					const int generation = ((PdfIntegerObject&)*next).value();
					dictionary->setObject(name, new PdfObjectReference(number, generation));
					value = 0;
					ch = 0;
				} else {
					break;
				}
			}
			std::string token;
			readToken(inputStream, token);
			if (token == "stream") {
				shared_ptr<PdfObject> d = dictionary;
				shared_ptr<PdfObject> lengthRef = (*dictionary)["Length"];
				shared_ptr<PdfObject> length = resolveReference(lengthRef);
				(*dictionary)["Length"] = length;
				int lengthV = 0;
				if(!length.isNull() && length->type() == PdfObject::INTEGER_NUMBER) {
					lengthV = ((PdfIntegerObject&) *length).value();
					/*if(lengthV == 0) {
						std::cerr << "warning: stream length is zero";
						if(!lengthRef.isNull() && lengthRef->type() == PdfObject::INTEGER_NUMBER) {
							std::cerr << " (without reference)";
						}
						std::cerr << "." << std::endl;
						shared_ptr<PdfObject> length = resolveReference(lengthRef);
					}*/
				} else {
					std::cerr << "warning: unknown length of stream with dict ";
					dictionary->dump();
					std::cerr  << std::endl;
				}
				skipWhiteSpaces(inputStream);
				return new PdfStreamObject(d, inputStream, ch, lengthV);
			} else {
				return dictionary;
			}
		}
		case PdfObject::NAME:
		{
			std::string xname;
			std::string name;
			GETC(inputStream, ch);
			do {
				readToken(inputStream, xname);
				name += xname;
				if(ch == '#') { // escaped code
					GETC(inputStream, ch);
					char num[3];
					num[0] = ch;
					GETC(inputStream, ch);
					num[1] = ch;
					GETC(inputStream, ch);
					num[2] = '\0';
					name += (char)strtol(num, 0, 16);
				} else
					break;
			} while(1);
			return PdfNameObject::nameObject(name);
		}
		case PdfObject::BOOLEAN:
		{
			std::string name;
			readToken(inputStream, name);
			return (name == "true") ? PdfBooleanObject::TRUE() : PdfBooleanObject::FALSE();
		}
		case PdfObject::INTEGER_NUMBER:
		{
			std::string str;
			if ((ch == '+') || (ch == '-')) {
				str += ch;
				GETC(inputStream, ch);
			}
			while ((ch >= '0') && (ch <= '9')) {
				str += ch;
				GETC(inputStream, ch);
			}
			if(ch == '.') { /* double-precision floating point value */
				str += ch;
				GETC(inputStream, ch);
				while((ch >= '0' && ch <= '9') || ch == 'e' || ch == 'E' || ch == '-' || ch == '+') { /* FIXME is that OK? */
					str += ch;
					GETC(inputStream, ch);
				}
				
				return PdfRealObject::realObject(atof(str.c_str()));
			} else
				return PdfIntegerObject::integerObject(atoi(str.c_str()));
		}
		case PdfObject::STRING:
		{
			std::stringstream valueStream;
			if (hexString) {
				char num[3];
				num[2] = '\0';
				while (ch && ch != '>') {
					num[0] = ch;
					EGETC(inputStream, num[1]);
					valueStream << (char)strtol(num, 0, 16);
					EGETC(inputStream, ch);
				}
				if(ch != '>') {
					std::cerr << "warning: unterminated hex string." << std::endl;
				}
				ch = 0;
			} else {
				nestingLevel = 0;
				//std::cout << "brace bli" << std::endl;
				while(ch != ')' || nestingLevel > 0) {
					//std::cout << (char) ch << ',';
					if(/* unlikely */ ch == '\\') {
						EGETC(inputStream, ch);
						switch(ch) {
						case 'n':
							valueStream << (char) 0x0A;
							break;
						case 'r':
							valueStream << (char) 0x0D;
							break;
						case 't':
							valueStream << (char) 0x09;
							break;
						case 'b':
							valueStream << (char) 0x08;
							break;
						case 'f':
							valueStream << (char) 0x0C; 
							break;
						case '(':
							valueStream << (char) '(';
							break;
						case ')':
							valueStream << (char) ')';
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7': /* octal */
							valueStream << (char) readOctalNumber(inputStream);
							continue;
						default:
							valueStream << (char) ch;
						}
					} else {
						if(ch == ')')
							--nestingLevel;
						else if(ch == '(')
							++nestingLevel;
						valueStream << (char) ch;
					}
					EGETC(inputStream, ch);
				}
				ch = 0;
			}
			return new PdfStringObject(valueStream.str());
		}
		case PdfObject::ARRAY:
		{
			PdfArrayObject *array = new PdfArrayObject();
			ch = 0;
			while (true) {
				skipWhiteSpaces(inputStream);
				if (ch == 'R') {
					const int size = array->size();
					if ((size >= 2) &&
							((*array)[size - 1]->type() == PdfObject::INTEGER_NUMBER) &&
							((*array)[size - 2]->type() == PdfObject::INTEGER_NUMBER)) {
						const int generation = ((PdfIntegerObject&)*array->popLast()).value();
						const int number = ((PdfIntegerObject&)*array->popLast()).value();
						array->addObject(new PdfObjectReference(number, generation));
						ch = 0;
					}
				}
				shared_ptr<PdfObject> object = readObject(inputStreamO);
				if (object.isNull()) {
					break;
				}
				array->addObject(object);
			}
			/*std::cerr << "PdfArrayObject " << array->size() << "\n";*/
			return array;
		}
		default:
			break;
	}

	std::string buffer;
	GETC(inputStream, ch);
	while (ch && PdfCharacterTypeTable[(unsigned char)ch] == PDF_CHAR_REGULAR) {
		buffer += ch;
		GETC(inputStream, ch);
	}
	/*std::cerr << "buffer = " << buffer << "\n";*/

	return 0;
}

void PdfParser::parse() {
	ZLInputStream& myStream = *myInputStream;
	size_t eofOffset = myStream.sizeOfOpened();
	if (eofOffset < 100) {
		return;
	}

	myStream.seek(eofOffset - 100, true);
	bool readXrefOffset = false;
	size_t xrefOffset = (size_t)-1;
	while (true) {
		myBuffer = readLine();
		if (myBuffer.empty()) {
			break;
		}
		stripBuffer(myBuffer);
		if (readXrefOffset) {
			if (!myBuffer.empty()) {
				xrefOffset = atoi(myBuffer.c_str());
				break;
			}
		} else if (myBuffer == "startxref") {
			readXrefOffset = true;
		}
	}

	if (!readReferenceTable(xrefOffset)) {
		return;
	}
}

PdfParser::PdfParser(shared_ptr<ZLInputStream> source, Pdf& destination) :
	myInputStream(source),
	myPDF(destination)
{
}


void PdfParser::skipWhiteSpaces(ZLInputStream& inputStream) {
	if (PdfCharacterTypeTable == 0) {
		PdfCharacterTypeTable = new PdfCharacterType[256];
		for (int i = 0; i < 256; ++i) {
			PdfCharacterTypeTable[i] = PDF_CHAR_REGULAR;
		}
		PdfCharacterTypeTable[0] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable[9] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable[10] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable[12] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable[13] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable[32] = PDF_CHAR_WHITESPACE;
		PdfCharacterTypeTable['('] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable[')'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['<'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['>'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['['] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable[']'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['{'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['}'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['/'] = PDF_CHAR_DELIMITER;
		PdfCharacterTypeTable['%'] = PDF_CHAR_DELIMITER;
	}

	while ((PdfCharacterTypeTable[(unsigned char)ch] == PDF_CHAR_WHITESPACE) &&
				 (inputStream.read(&ch, 1) == 1)) {
	}
}

void PdfParser::readToken(ZLInputStream& inputStream, std::string &buffer) {
	buffer.clear();
	skipWhiteSpaces(inputStream);
	while (PdfCharacterTypeTable[(unsigned char)ch] == PDF_CHAR_REGULAR && ch != '#') {
		buffer += ch;
		EGETC(inputStream, ch);
	}
}

std::vector<std::pair<int, shared_ptr<PdfObject> > > PdfParser::parseObjStm(PdfStreamObject& objStm) {
	ch = 0;
	std::vector<std::pair<int, shared_ptr<PdfObject> > > result;
	shared_ptr<PdfObject> dictionaryO = objStm.dictionary();
	if(dictionaryO.isNull() || dictionaryO->type() != PdfObject::DICTIONARY) // ERROR
		return result; 
	PdfDictionaryObject& dictionary = (PdfDictionaryObject&) *dictionaryO;
	int N = 0;
	int First = 0;
	if(!integerFromPdfObject(dictionary["N"], N) || !integerFromPdfObject(dictionary["First"], First))
		return result;
	// FIXME Extends
	shared_ptr<ZLInputStream> streamO = objStm.stream();
	if(streamO.isNull())
		return result;
	ZLInputStream& stream = *streamO;
	std::vector<std::pair<int, int> > offsets;
	for(int i = 0; i < N; ++i) {
		std::string objectIDStr;
		std::string offsetStr;
		readToken(stream, objectIDStr);
		readToken(stream, offsetStr);
		int objectID = atoi(objectIDStr.c_str()); // FIXME error check.
		int offset = atoi(offsetStr.c_str()); // FIXME error check.
		// was: stream >> objectID >> offset;
		offsets.push_back(std::make_pair(objectID, offset)); // Note: offsets are in increasing order!
	}
	for(std::vector<std::pair<int, int> >::const_iterator iter_offsets = offsets.begin(); iter_offsets != offsets.end(); ++iter_offsets) {
		// TODO or just skip junk until we are there.
		stream.seek(First + iter_offsets->second, true);
		ch = 0;
		result.push_back(std::make_pair(iter_offsets->first, readObject(streamO)));
	}
	return result;
}

shared_ptr<PdfObject> PdfParser::getObjStmEntry(int objStmID, int objStmGeneration, int referencedIndex, int referencedID) {
	assert(objStmGeneration == 0); // FIXME support other generations?
	std::pair<int, int> key = std::make_pair(objStmID, objStmGeneration);
	if(myCompressedObjects.find(key) == myCompressedObjects.end()) {
		shared_ptr<PdfObject> obj = resolveReference(new PdfObjectReference(objStmID, objStmGeneration));
		if(!obj.isNull() && obj->type() == PdfObject::STREAM)
			myCompressedObjects[key] = new PdfCompressedObjects(parseObjStm((PdfStreamObject&) *obj));
		else { // ERROR
			return 0;
		}
	}
	assert((*myCompressedObjects[key])[referencedIndex].first == referencedID);
	return (*myCompressedObjects[key])[referencedIndex].second; // FIXME bounds check.
}
