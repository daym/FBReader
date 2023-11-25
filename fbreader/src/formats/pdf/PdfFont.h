#ifndef __PDF_FONT_H
#define __PDF_FONT_H
/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <danny_milo@yahoo.com>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if !, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "PdfObject.h"
#include "PdfToUnicodeMap.h"
#include <ZLInputStream.h>

typedef std::map<shared_ptr<PdfObject>, shared_ptr<PdfObject> > PdfXObjects; /* name -> image-data */

class PdfFont {
private:
	shared_ptr<PdfObject> myMetadata;
	shared_ptr<PdfObject> myFontDescriptor;
	PdfXObjects myResources; // for symbol font.
	shared_ptr<PdfObject> myWidths;
	shared_ptr<PdfObject> myWs;
	int myWidthsMissingWidth;
	int myWidthsFirstChar;
	bool myBBoldness;
	PdfToUnicodeMap myToUnicodeMap;
	double myScale;
	double myDescent;
	double myAscent;
	int myCachedWidths[256]; /* -1 = unknown */
public:
	PdfFont(shared_ptr<PdfObject> metadata, shared_ptr<PdfObject> fontDescriptor, shared_ptr<PdfObject> encoding, shared_ptr<PdfObject> fontFile, shared_ptr<PdfObject> toUnicode, PdfXObjects resources, shared_ptr<PdfObject> widths, shared_ptr<PdfObject> ws, int widthsMissingWidth, int widthsFirstChar);
	double textSpaceFromGlyphSpaceFactor() const { return myScale; }
	double descent() const { return myDescent; }
	double ascent() const { return myAscent; }
	bool boldness() const { return myBBoldness; }
	const PdfToUnicodeMap* toUnicodeTable() const { return &myToUnicodeMap; }
	bool containsUnicode() const { return myResources.size() == 0; }
	int width(int code) const;
protected:
	void prepare();
	void parseEncoding(shared_ptr<PdfObject> encoding);
	void parseDefaultEncoding(shared_ptr<PdfObject> encoding);
	void parseFontFile(shared_ptr<PdfObject> fontFile);
	bool parseToUnicodeTable(shared_ptr<PdfObject> toUnicode);
};
	
#endif /* ndef __PDF_FONT_H */
