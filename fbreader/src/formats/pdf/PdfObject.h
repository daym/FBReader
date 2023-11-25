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

#ifndef __PDFOBJECT_H__
#define __PDFOBJECT_H__

#include <string>
#include <vector>
#include <map>
#include <list>

#include <shared_ptr.h>
#include "PdfParser.h"

class ZLInputStream;

class PdfObject {

public:
	//static shared_ptr<PdfObject> readObject(ZLInputStream &stream, char &ch);

public:
	enum Type {
		BOOLEAN,
		INTEGER_NUMBER,
		REAL_NUMBER,
		STRING,
		NAME,
		ARRAY,
		DICTIONARY,
		STREAM,
		NIL,
		REFERENCE,
		INSTRUCTION, /* FIXME do we want an ADT node type or just an array with the instruction data instead? */
		CONTENT,
	};

	virtual ~PdfObject();

	virtual Type type() const = 0;
};

class PdfBooleanObject : public PdfObject {

public:
	static shared_ptr<PdfObject> TRUE();
	static shared_ptr<PdfObject> FALSE();

private:
	PdfBooleanObject(bool value);

public:
	bool value() const;

private:
	Type type() const;

private:
	const bool myValue;
};

class PdfIntegerObject : public PdfObject {

public:
	static shared_ptr<PdfObject> integerObject(int value);

private:
	PdfIntegerObject(int value);

public:
	int value() const;

private:
	Type type() const;

private:
	const int myValue;
};

class PdfRealObject : public PdfObject {

public:
	static shared_ptr<PdfObject> realObject(double value);

private:
	PdfRealObject(double value);

public:
	double value() const;

private:
	Type type() const;

private:
	const double myValue;
};

class PdfStringObject : public PdfObject {

public:
	PdfStringObject(const std::string &value);

private:
	Type type() const;

private:
	std::string myValue;
public:
	std::string value() { return myValue; }

friend shared_ptr<PdfObject> PdfParser::readObject(shared_ptr<ZLInputStream>);
};

class PdfNameObject : public PdfObject {
private:
	std::string fID;
public:
	static shared_ptr<PdfObject> nameObject(const std::string &id);
	std::string id() const { return fID; }
	
private:
	static std::map<std::string,shared_ptr<PdfObject> > ourObjectMap;

private:
	PdfNameObject(const std::string& id);
	PdfNameObject(const PdfNameObject& source); /* not defined */
	PdfNameObject& operator=(const PdfNameObject& source); /* not defined */
private:
	Type type() const;
};

class PdfDictionaryObject : public PdfObject {

private:
	PdfDictionaryObject();
	void setObject(shared_ptr<PdfObject> id, shared_ptr<PdfObject> object);

public:
	shared_ptr<PdfObject> operator [] (shared_ptr<PdfObject> id) const;
	shared_ptr<PdfObject>& operator [] (shared_ptr<PdfObject> id);
	shared_ptr<PdfObject> operator [] (const std::string &id) const;
	std::list<shared_ptr<PdfObject> > keys() const;
	void dump() const;

private:
	Type type() const;

private:
	std::map<shared_ptr<PdfObject>,shared_ptr<PdfObject> > myMap;

friend shared_ptr<PdfObject> PdfParser::readObject(shared_ptr<ZLInputStream>);
};

class PdfStreamObject : public PdfObject {
private:
	void processDecompressor(shared_ptr<PdfObject> filter, ZLInputStream& originalStream);
	void processDecompressorChain(shared_ptr<PdfObject>& filters, ZLInputStream& originalStream);
private:
	PdfStreamObject(shared_ptr<PdfObject> dictionary, ZLInputStream& dataStream, char& ch, size_t size);

private:
	Type type() const;
public:
	shared_ptr<ZLInputStream> stream() const;
	shared_ptr<PdfObject> dictionary() const;
	void setDictionary(shared_ptr<PdfObject> value);
	void dump();
private:
	std::string myData;
	size_t mySize;
	shared_ptr<PdfObject> myDictionary;

friend shared_ptr<PdfObject> PdfParser::readObject(shared_ptr<ZLInputStream>);
};

class PdfArrayObject : public PdfObject {

private:
	shared_ptr<PdfObject> popLast();

public:
	void addObject(shared_ptr<PdfObject> object);
	PdfArrayObject();
	int size() const;
	shared_ptr<PdfObject> operator [] (int index) const;
	shared_ptr<PdfObject>& operator [] (int index);

private:
	Type type() const;

private:
	std::vector<shared_ptr<PdfObject> > myVector;

friend shared_ptr<PdfObject> PdfParser::readObject(shared_ptr<ZLInputStream>);
};

class PdfObjectReference : public PdfObject {

public:
	PdfObjectReference(int number, int generation);

	int number() const;
	int generation() const;

private:
	Type type() const;

private:
	const int myNumber;
	const int myGeneration;
};

/* helpers */

static inline bool integerFromPdfObject(const shared_ptr<PdfObject>& item, int& a) {
	if(!item.isNull() && item->type() == PdfObject::INTEGER_NUMBER) {
		a = ((PdfIntegerObject&)*item).value();
		return true;
	} else
		return false;
}

static inline bool doubleFromPdfObject(const shared_ptr<PdfObject>& item, double& a) {
	if(item.isNull())
		return false;
	if(item->type() == PdfObject::REAL_NUMBER) {
		a = ((PdfRealObject&)*item).value();
		return true;
	} else {
		int b;
		if(integerFromPdfObject(item, b)) {
			a = b;
			return true;
		} else
			return false;
	}
}

#endif /* __PDFOBJECT_H__ */
