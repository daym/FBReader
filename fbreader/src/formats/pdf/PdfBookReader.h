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

#ifndef __PdfBOOKREADER_H__
#define __PdfBOOKREADER_H__

#include <map>
#include <set>
#include <stack>
#include "../../bookmodel/BookReader.h"
#include "PdfContent.h"
#include "PdfToUnicodeMap.h"
#include "PdfToUnicodeParser.h"
#include "PdfFont.h"
#include "PdfGraphicsState.h"
#include "Pdf.h"

class PdfObject;
class PdfObjectReference;

class PdfBookReader : public BookReader {
public:
	PdfBookReader(BookModel &model);
	~PdfBookReader();
	bool readBook(shared_ptr<ZLInputStream> stream);
private:
	void processPage(shared_ptr<PdfObject> pageObject, ZLInputStream &stream);
	bool processPages(shared_ptr<PdfObject> pageRootNode, ZLInputStream &stream);
	void processContents(shared_ptr<PdfObject> contentsObject, ZLInputStream &stream);
	void processInstruction(const PdfInstruction& instruction_);
	PdfXObjects prepareXObjects(shared_ptr<PdfObject> resourcesObject, ZLInputStream& stream);
	shared_ptr<PdfObject> findXObject(const shared_ptr<PdfObject>& name) const;
	void processXObject(shared_ptr<PdfObject> nameObject);
	void processInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions);
	void loadFonts(shared_ptr<PdfObject> resourcesObject, ZLInputStream &stream);
	shared_ptr<PdfFont> loadFont(shared_ptr<PdfObject> metadata, ZLInputStream &stream);
	void setTextRise(double value); // "Ts" command.
	bool increaseEffectiveTextRise(double value); // real rise in FBReader, either by positioning or by "Ts" command.
	void goToNextLine();
	void setPosition(double horizontalPosition, double verticalPosition);
	void setFont(const shared_ptr<PdfObject>& name, unsigned fontSize);
	void addData(const std::string& text);
	void addWordSpace();
	void setBoldness(bool value);
	std::string mangleTextPart(const std::string& part);
	void goToNextParagraph();
	void updateTextHeight();
	void setTextScale(double h, double v);
	void cancelSubscriptSuperscript();
	void processInlineMovement(PdfStringObject* text);
	void processInlineMovement1(int amount);
	void setPositionRelativeToBeginning(double dX, double dY);
	void setPositionRelativeToHere(double dX, double dY);
protected:
	shared_ptr<PdfObject> resolveReferences(shared_ptr<PdfObject> ref, ZLInputStream &stream);
		
private:
	/*BookReader myModelReader;*/
	double myBBoldness;
	const PdfToUnicodeMap* fToUnicodeTable; /* current one, depending on font. */
	const PdfFont* myFont;
	std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> > fFontsByName;
	std::map<PdfToUnicodeMapAddress, shared_ptr<PdfFont> > fFontsByAddress;
	std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> > fFontsByNativeAddress;
	shared_ptr<PdfObject> resourcesObject;
	PdfXObjects myXObjects2; /* name -> image-data */
	std::set<shared_ptr<PdfObject> > myLines; // names
	double myLeading; // text state, not per text object.
	double myPositionV; // text object stuff.
	double myPositionH; // this isn't really updated correctly (yet?).
	double myTextRise; // text state, not per text object.
	double myEffectiveTextRise; // calculated.
	double myDescent; // negative.
	double myAscent;
	double myTextSpaceFromGlyphSpaceFactor;
	double myTextHeight; // this tries to find the maximum text height as a measure of what "big" is.
	bool myIsFirstOnLine;
	unsigned char myPreviousCharacter[5];
	unsigned myImageIndex;
	double myParagraphIndentation;
	bool myBForceNewLine; // used in tables etc where it's important to have newlines.
	double myPositionBeginningH;
	unsigned myFontSize;
	int myCharSpacing;
	int myWordSpacing;
	int myHorizontalScaling; /* 100 = none */
	std::stack<PdfGraphicsState> myGraphicsStack;
	PdfGraphicsState myGraphicsState;
	shared_ptr<PdfParser> myParser;
	shared_ptr<Pdf> myPDF;
	double myTextScaleH;
	double myTextScaleV;
	bool myBPendingTextMatrixReset;
	double myPendingTextMatrixBeginningH;
	double myPendingTextMatrixH;
	double myPendingTextMatrixV;
	std::set<shared_ptr<PdfObject> > myResolvedTraces;
};

#endif /* __PdfBOOKREADER_H__ */
