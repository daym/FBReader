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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <cmath>
#include <cassert>

#include <ZLStringUtil.h>
#include <ZLInputStream.h>
#include <ZLUnicodeUtil.h>

#include "PdfBookReader.h"
#include "PdfObject.h"
#include "PdfContent.h"
#include "PdfContentParser.h"
#include "PdfToUnicodeMap.h"
#include "../../bookmodel/BookModel.h"
#include "PdfImage.h"
#include "PdfGraphicsState.h"

/* empirically determined, yay */
#define EPSILON 0.0002

PdfBookReader::PdfBookReader(BookModel &model) : BookReader(model), /*myModelReader(model), */fToUnicodeTable(0), myGraphicsState(1.0, 1.0) {
	myFont = NULL;
	myFontSize = 1;
	myLeading = 0.0;
	myPositionV = 0.0;
	myPositionH = 0.0;
	myPositionBeginningH = 0.0;
	myTextRise = 0.0;
	myEffectiveTextRise = 0.0;
	myDescent = 0.0;
	myAscent = 0.0;
	myTextHeight = 0.0;
	myIsFirstOnLine = true;
	myPreviousCharacter[0] = 0;
	myPreviousCharacter[1] = 0;
	myPreviousCharacter[2] = 0;
	myPreviousCharacter[3] = 0;
	myPreviousCharacter[4] = 0;
	myImageIndex = 0;
	myBBoldness = false;
	myParagraphIndentation = 0.0;
	myTextScaleH = 1.0;
	myTextScaleV = 0.9; // 1.0;
	myCharSpacing = 0;
	myWordSpacing = 0;
	myHorizontalScaling = 100;
	myBPendingTextMatrixReset = true;
	myPendingTextMatrixBeginningH = 0.0;
	myPendingTextMatrixH = 0.0;
	myPendingTextMatrixV = 0.0;
}

PdfBookReader::~PdfBookReader() {
}

bool PdfBookReader::readBook(shared_ptr<ZLInputStream> stream) {
	bool status;
	if (stream.isNull() || !stream->open()) {
		return false;
	}

	char buffer[50];
	if ((*stream).read(buffer, 50) < strlen("%PDF-")) {
		return false;
	}

	if (memcmp(buffer, "%PDF-", strlen("%PDF-")) != 0) {
		return false;
	}

	myPDF = new Pdf();
	myParser = new PdfParser(stream, *myPDF);
	myParser->parse();

	shared_ptr<PdfObject> trailerX = myPDF->trailer();
	if(trailerX.isNull()) {
		return false;
	}

	PdfDictionaryObject &trailerDictionary = (PdfDictionaryObject&)*trailerX;
	//trailerDictionary.dump();
	shared_ptr<PdfObject> root = resolveReferences(trailerDictionary["Root"], *stream);
	if (root.isNull() || (root->type() != PdfObject::DICTIONARY)) {
		return false;
	}

	PdfDictionaryObject &rootDictionary = (PdfDictionaryObject&)*root;
	if (rootDictionary["Type"] != PdfNameObject::nameObject("Catalog")) {
		return false;
	}
	shared_ptr<PdfObject> pageRootNode = rootDictionary["Pages"];

	setMainTextModel();
	if (isKindStackEmpty())
		pushKind(REGULAR);
	assert(pageRootNode->type() != PdfObject::REFERENCE);
	status = processPages(pageRootNode, *stream);
	std::cout << "end of book, status " << status << std::endl;

	return status;
}

bool PdfBookReader::processPages(shared_ptr<PdfObject> pageRootNode, ZLInputStream &stream) {
	//pageRootNode = resolveReference(pageRootNode, stream);
	if(pageRootNode.isNull() || (pageRootNode->type() != PdfObject::DICTIONARY)) {
		return false;
	}
	PdfDictionaryObject &pageRootNodeDictionary = (PdfDictionaryObject&)*pageRootNode;	
	//pageRootNodeDictionary.dump();
	if (pageRootNodeDictionary["Type"].isNull() || pageRootNodeDictionary["Type"] != PdfNameObject::nameObject("Pages")) {
		return false;
	}
	
	/*
	shared_ptr<PdfObject> count = pageRootNodeDictionary["Count"];
	if (!count.isNull() && (count->type() == PdfObject::INTEGER_NUMBER)) {
		std::cerr << "count = " << ((PdfIntegerObject&)*count).value() << "\n";
	}
	*/
	shared_ptr<PdfObject> pages = pageRootNodeDictionary["Kids"];
	if (pages.isNull() || (pages->type() != PdfObject::ARRAY)) {
		return false;
	}
	const PdfArrayObject& pagesArray = (const PdfArrayObject&)*pages;
	const size_t pageNumber = pagesArray.size();
	for (size_t i = 0; i < pageNumber; ++i) {
		if(!processPages(pagesArray[i], stream)) {
			processPage(pagesArray[i], stream);
		}
	}
	return true;
}


void PdfBookReader::processXObject(shared_ptr<PdfObject> nameObject) {
	if(myLines.find(nameObject) != myLines.end()) {
		/* probably a separator */
		// TODO increaseEffectiveTextRise(-myEffectiveTextRise); or something.
		if(myBBoldness) {
			setBoldness(false);
			addData("/");
			setBoldness(true);
		} else
			addData("/");

		myIsFirstOnLine = true;
		return;
	}
	
	goToNextLine();
	/*const PdfNameObject& name = (const PdfNameObject&)*name;*/
	shared_ptr<PdfObject> imageX = findXObject(nameObject);
	std::string ID;
	ZLStringUtil::appendNumber(ID, myImageIndex++);
	//ID << reference.number() << "-" << reference.generation();
	/* TODO check filter = DCTDecode */
	// TODO get attribute "Subtype": "Form" or "Image".
	// TODO scale
	if(imageX.isNull() || imageX->type() != PdfObject::STREAM)
		return;
	addImageReference(ID);
	addImage(ID, new PdfImage(imageX));
	//std::cout << "should print XObject " << ((PdfNameObject&)*nameObject).id() << " here..." << std::endl;
}

/*findLeading(instructions, index) {
opMoveCaretToStartOfNextLine                           
(opMoveCaret)
opNextLineShowString
opNextLineSpacedShowString
--     
opSetTextLeading
opMoveCaretToStartOfNextLineAndOffsetAndSetLeading 
}
*/


void PdfBookReader::setFont(const shared_ptr<PdfObject>& name, unsigned fontSize) {
	/*addData("<set font");
	addData(((PdfNameObject&)(*name)).id());
	addData(">");*/
	/*std::cerr << "setting font " << ((PdfNameObject&)(*name)).id() << std::endl;*/
	myDescent = 0.0;
	myAscent = 0.0;
	std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> >::const_iterator iter_fonts = fFontsByName.find(name);
	if(iter_fonts != fFontsByName.end()) { // found font.
		PdfFont& font = *iter_fonts->second;
		fToUnicodeTable = font.toUnicodeTable();
		myFont = &font;
		myTextSpaceFromGlyphSpaceFactor = font.textSpaceFromGlyphSpaceFactor();
		myDescent = font.descent();
		myAscent = font.ascent();
		if(fabs(myAscent) < EPSILON && fabs(myDescent) < EPSILON) {
			std::cerr << "setting font " << ((PdfNameObject&)(*name)).id() << std::endl;
			std::cerr << "setFont: warning: ascent=" << myAscent << std::endl;
			std::cerr << "setFont: descent=" << myDescent << std::endl;
		}
		myFontSize = fontSize;
		//std::cerr << "factor " << myTextSpaceFromGlyphSpaceFactor << std::endl;
		updateTextHeight();
		setBoldness(font.boldness());
	} else {
		addData("<unknown font>");
		fToUnicodeTable = 0;
		myFont = 0;
	}
}

void PdfBookReader::updateTextHeight() {
	double currentTextHeight = (myAscent - myDescent) * myTextSpaceFromGlyphSpaceFactor * myFontSize * /* fabs(myGraphicsState.CTMScaleV()) * */ fabs(myTextScaleV); 
	if(currentTextHeight < EPSILON) {
		std::cerr << "warning: current text height is 0." << std::endl;
		std::cerr << "info: ^-- myAscent-myDescent is " << (myAscent - myDescent) << std::endl;
		std::cerr << "info: ^-- myTextSpaceFromGlyphSpaceFactor is " << myTextSpaceFromGlyphSpaceFactor << std::endl;
		std::cerr << "info: ^-- myFontSize is " << myFontSize << std::endl;
		//std::cerr << "info: ^-- CTM scale is " << fabs(myGraphicsState.CTMScaleV()) << std::endl;
		std::cerr << "info: ^-- myTextScaleV is " << myTextScaleV << std::endl;
	}
	//std::cout << "setting text height " << currentTextHeight << std::endl;
	//if(currentTextHeight > myTextHeight)
		myTextHeight = currentTextHeight;
}

std::string PdfBookReader::mangleTextPart(const std::string& part) {
	if(ZLStringUtil::stringStartsWith(part, "....")) { // silly detection of table of contents "1.1 Stuff...............4711".
		myBForceNewLine = true;
		// shorten the run of points to maybe fit on a small screen in portrait mode.
		const char* result_c = part.c_str();
		while(result_c && *result_c == '.')
			++result_c;
		std::string result = result_c;
		result = "...." + result;
		return result;
	}
	return part;
}
void PdfBookReader::processInlineMovement1(int amount) {
	myPositionH += (amount * myFontSize / 1000 + myCharSpacing/* FIXME + [T_W]*/) * myHorizontalScaling / 100;
}
void PdfBookReader::processInlineMovement(PdfStringObject* text) {
	if(myFont == NULL) /* ?!?! */
		return;
	std::string value = text->value();
	int size = value.length();
	for(int i = 0; i < size; ++i) {
		int code = (unsigned char) value.at(i);
		if(code == 0) /* FIXME */
			continue;
		int W0 = myFont->width(code);
		/* FIXME check whether the code is a space code and if so, use word spacing. */
		myPositionH += (W0 * myFontSize / 1000 + myCharSpacing/* FIXME + [T_W]*/) * myHorizontalScaling / 100;
	}
}
void PdfBookReader::processInstruction(const PdfInstruction& instruction) {
	int i;
	std::string result;
	bool actual_newline = true;
	PdfOperator operator_ = instruction.operator_();
	/*instruction.dump();*/
	if(operator_ == opShowString /*Tj non-array*/ ||
	   operator_ == opNextLineShowString /*'*/ || 
	   operator_ == opNextLineSpacedShowString /*"*/ ||
	   operator_ == opShowStringWithVariableSpacing /*TJ*/) {
		if(myFont && !myFont->containsUnicode()) {
			// TODO draw symbol font.
			return;
		}

		shared_ptr<PdfObject> item = instruction.extractFirstArgument();
		if(item->type() == PdfObject::ARRAY) {
			PdfArrayObject* xarray = (PdfArrayObject*)&*item;
			int argument_count = xarray->size();
			for(i = 0; i < argument_count; ++i) {
				shared_ptr<PdfObject> xitem = (*xarray)[i];
				if(xitem->type() == PdfObject::STRING) { // esp. ! number
					PdfStringObject* stringObject = (PdfStringObject*)&*xitem;
					result = result + mangleTextPart(fToUnicodeTable->convertStringToUnicode(stringObject->value()));
					//result = result + ' ';
					processInlineMovement(stringObject);
				} else if(xitem->type() == PdfObject::INTEGER_NUMBER) {
					PdfIntegerObject* integerObject = (PdfIntegerObject*)&*xitem;
					if(integerObject->value() < -57) // FIXME these are textspace/1000.
						result = result + " ";
					processInlineMovement1(-integerObject->value());
				} else if(xitem->type() == PdfObject::REAL_NUMBER) {
					PdfRealObject* realObject = (PdfRealObject*)&*xitem;
					if(realObject->value() < -57)
						result = result + " ";
					processInlineMovement1(-realObject->value());
					/*debugging: else
						result = result + "@";*/
				}
			};
			//result = Copy(result, 1, Length(result) - 1);
		} else if(item->type() == PdfObject::STRING) {
			PdfStringObject* stringObject = (PdfStringObject*)&*item;
			std::string v = fToUnicodeTable->convertStringToUnicode(stringObject->value());
			result = result + v;
			processInlineMovement(stringObject);
		}

		if(operator_ == opNextLineShowString ||
		   operator_ == opNextLineSpacedShowString) {
			result = result + ' '; // (char) 10;
		}
	} else if(operator_ == opPaintExternalObject) {
		shared_ptr<PdfObject> item = instruction.extractFirstArgument();
		if(item->type() == PdfObject::NAME) {
			processXObject(item);
		}
	} else if(operator_ == opSetFontAndSize) {
		const shared_ptr<PdfArrayObject>& arguments = instruction.arguments();
		int argument_count = arguments->size();
		if(argument_count == 0) /* !?!?! */
			return;
		shared_ptr<PdfObject> name = (*arguments)[0];
		unsigned fontSize = 0;
		if(argument_count > 1) {
			shared_ptr<PdfObject> size = (*arguments)[1];
			if(!size.isNull()) {
				if(size->type() == PdfObject::INTEGER_NUMBER) {
					fontSize = ((const PdfIntegerObject&)(*size)).value();
				} else if(size->type() == PdfObject::REAL_NUMBER) {
					fontSize = ((const PdfRealObject&)(*size)).value(); // TODO what about fractions?
				} else { /* ignore junk */
				}
			}
		}
		setFont(name, fontSize);
	} else if(operator_ == opMoveCaretToStartOfNextLineAndOffsetAndSetLeading) {
		double horizontal_offset, vertical_offset;
		if(instruction.extractTwoFloatArguments(horizontal_offset, vertical_offset)) {
			//addWordSpace();
			myLeading = -vertical_offset;
			setPositionRelativeToBeginning(horizontal_offset, vertical_offset);
		}
		/*if(actual_newline) {
			goToNextLine();
		}
		myPositionV -= myLeading;*/
	} else if(operator_ == opMoveCaretToStartOfNextLine) {
		//result = result + "\n";
		/*goToNextLine();
		myPositionV -= myLeading;*/
		setPositionRelativeToBeginning(0, - myLeading);
	} else if(operator_ == opMoveCaret) {
		double horizontal_offset, vertical_offset;
		if(instruction.extractTwoFloatArguments(horizontal_offset, vertical_offset)) {
			// FIXME addWordSpace();
			setPositionRelativeToBeginning(horizontal_offset, vertical_offset);
		}
		/*if(actual_newline) {
			goToNextLine();
		}
		myPositionV += vertical_offset;*/
	} else if(operator_ == opSetTextMatrix) {
		myBPendingTextMatrixReset = false; // not cumulative.
		std::vector<double> arguments = instruction.extractFloatArguments();
		// coordinates (x y w) will translate to (a_0⋅x+a_2⋅y+4⋅w, a_1⋅x+a_3⋅y+a_5⋅w, w), where a are the arguments, starting with index 0.
		if(arguments.size() >= 6) {
			//setTextScale(arguments[0], arguments[3]);
			// TODO complete this?
			double verticalPosition = /*arguments[1] * horizontalPosition + */ arguments[5];
			// not yet. setPosition(arguments[4], verticalPosition);
			/* the reason for this indirection is that some inane PDFs start a new BT for each and every line of text. 
			   It is good then to not take the ensuing Tm too seriously. */
			myPendingTextMatrixH = myPendingTextMatrixBeginningH = arguments[4];
			myPendingTextMatrixV = verticalPosition;
			myBPendingTextMatrixReset = true;
		} else {
			//setTextScale(1.0, 1.0);
			// just in case.
			addWordSpace();
		}
	} else if(operator_ == opTransformationMatrixAppend) {
		// FIXME actually concatenate.
		std::vector<double> arguments = instruction.extractFloatArguments();
		if(arguments.size() >= 6) {
			myGraphicsState.setCTMScale(myGraphicsState.CTMScaleH() * arguments[0], myGraphicsState.CTMScaleV() * arguments[3]);
		}
	} else if(operator_ == opSetTextRise) {
		double textRise = 0.0;
		if(instruction.extractFloatArgument(textRise)) {
			// arg1 > 0: superscripted.
			setTextRise(textRise);
		}
	} else if(operator_ == opSetTextLeading) {
		instruction.extractFloatArgument(myLeading);
	} else if(operator_ == opPushGraphicsState) {
		myGraphicsStack.push(myGraphicsState);
	} else if(operator_ == opPopGraphicsState) {
		if(myGraphicsStack.empty())
			std::cerr << "warning: tried to pop element off empty graphics stack." << std::endl;
		else {
			myGraphicsState = myGraphicsStack.top();
			myGraphicsStack.pop();
		}
	} else if(operator_ == opSetCharacterSpacing) {
		if(!integerFromPdfObject(instruction.extractFirstArgument(), myCharSpacing))
			myCharSpacing = 0;
	} else if(operator_ == opSetWordSpacing) {
		if(!integerFromPdfObject(instruction.extractFirstArgument(), myWordSpacing))
			myWordSpacing = 0;
	} else if(operator_ == opSetHorizontalScaling) {
		if(!integerFromPdfObject(instruction.extractFirstArgument(), myHorizontalScaling))
			myHorizontalScaling = 100;
	} else if(operator_ == opEndText) {
		/* some PDFs create a text block for each and every line. So don't do: goToNextParagraph(); */
	}
	if(result.length() > 0) {
		if(myBPendingTextMatrixReset)
			setPositionRelativeToHere(0, 0); // make sure text matrix is taken into account and un-pended.
		/*std::cout << result;*/
		if(result == "\342\200\242") { // U+2022 bullet and nothing else.
			// this is probably an enumeration point so make sure we are on a new line.
			double previousTextHeight = myTextHeight;
			goToNextParagraph();
			myTextHeight = previousTextHeight;

		}
		addData(result);
		/*std::cout << result;
		std::cout.flush();*/
	}
#if 0
	if(operator_ == opShowStringWithVariableSpacing) {
		myPositionV -= myLeading;
		goToNextLine();
		//setPosition(myPositionH, myPositionV - myLeading);
	}
#endif
}

void PdfBookReader::cancelSubscriptSuperscript() {
	increaseEffectiveTextRise(-myEffectiveTextRise);
	myParagraphIndentation = 0.0;
	myTextHeight = 0.0;
}

void PdfBookReader::goToNextParagraph() {
	increaseEffectiveTextRise(-myEffectiveTextRise);
	goToNextLine();
	cancelSubscriptSuperscript();
	updateTextHeight();
	//addData("<newpara>");
}

void PdfBookReader::setPositionRelativeToBeginning(double dX, double dY) {
	if(myBPendingTextMatrixReset) {
		myBPendingTextMatrixReset = false;
		setPosition(myPendingTextMatrixBeginningH + dX, myPendingTextMatrixV + dY);
	} else {
		setPosition(myPositionBeginningH + dX, myPositionV + dY);
	}
}
void PdfBookReader::setPositionRelativeToHere(double dX, double dY) {
	if(myBPendingTextMatrixReset) {
		myBPendingTextMatrixReset = false;
		setPosition(myPendingTextMatrixH + dX, myPendingTextMatrixV + dY);
	} else {
		setPosition(myPositionH + dX, myPositionV + dY);
	}
}
void PdfBookReader::setPosition(double horizontalPosition, double verticalPosition) {
	double offset = verticalPosition - myPositionV;
	double hoffset = horizontalPosition - myPositionH;
	assert(myAscent >= 0);
	assert(myDescent <= 0);
	/*if(myParagraphIndentation != horizontalPosition) {
	}*/
	//if(offset > myAscent || offset < myDescent) { // too far, probably not a superscript/subscript.
	/*char buffer[2000];
	snprintf(buffer, 2000, "(%.2lf %.2lf %.2lf %.2lf)", offset, myTextHeight, myPositionV, fabs(myGraphicsState.CTMScaleV()));
	addData(buffer);*/
	myPositionH = horizontalPosition;
	myPositionBeginningH = horizontalPosition;
	double scaleV = 1.7; // FIXME don't hardcode?
	if(offset > myTextHeight * scaleV || offset < -myTextHeight * scaleV || myIsFirstOnLine) { // too far, probably not a superscript. 
		if(!myIsFirstOnLine)
			addWordSpace(); // ata(" "); // make sure we finished the word, whatever it was. addData("#");
		increaseEffectiveTextRise(-myEffectiveTextRise); /* cancel text rise */
		myPositionV = verticalPosition;
		// the myIsFirstOnLine check is in order to allow fraction lines.
		if((1&&!myIsFirstOnLine) || /*offset > myTextHeight || offset < -myTextHeight || */myBForceNewLine ) { // TODO make this a setting.
			if(myBForceNewLine || (offset <= myTextHeight * 30 && offset >= -myTextHeight * 30) || myTextHeight < EPSILON) { // avoids adding a newline in a multi-column layout which is not what you usually want.
				myBForceNewLine = false;
				goToNextParagraph();
			}
		}
		return;
	} else if(offset >= -myTextHeight && offset <= myTextHeight) { // probably a superscript or subscript.
		myPositionV = verticalPosition;
		if(increaseEffectiveTextRise(offset))
			return;
	} else {
		myPositionV = verticalPosition;
		if(offset > EPSILON || offset < -EPSILON) {
			addWordSpace();
			return;
		}
	}

	if(offset < EPSILON && (hoffset > EPSILON && hoffset > 10 + myCharSpacing * 2 /* TODO allow more leeway */)) // FIXME
		addWordSpace();
}

void PdfBookReader::setTextRise(double value) {
	increaseEffectiveTextRise(myTextRise - value);
	myTextRise = value;
}

bool PdfBookReader::increaseEffectiveTextRise(double offset) {
	bool previousBoldness = myBBoldness;
	/* returns whether we have newly entered either subscript or superscript. */
	double value = myEffectiveTextRise + offset;
	double minimalAD = myTextHeight / 10; // minimal ascent/descent.
	if(minimalAD < EPSILON)
		minimalAD = EPSILON;
	if(fabs(offset) < EPSILON) // rounding error or dude who set the text was drunk.
		return false; // myEffectiveTextRise <= -EPSILON || myEffectiveTextRise >= EPSILON;
	setBoldness(false);
	if(fabs(myEffectiveTextRise) >= EPSILON) { // get rid of previous subscript/superscript.
		//popKind();
		addControl(myEffectiveTextRise > 0 ? SUP : SUB, false);
	}
	if(value >= minimalAD) {
		addControl(SUP, true);
	} else if(value <= -minimalAD) {
		addControl(SUB, true);
	}
	setBoldness(previousBoldness);
	/*char tmp[2000];
	snprintf(tmp, 2000, "(%.4f->%.4f)", myEffectiveTextRise, value);
	addData(tmp);*/
	bool result = (value <= -minimalAD && myEffectiveTextRise > -minimalAD) || (value >= minimalAD && myEffectiveTextRise <= minimalAD);
	myEffectiveTextRise = value;
	return result;
}

void PdfBookReader::goToNextLine() {
	//setTextRise(0.0); 
	//increaseEffectiveTextRise(-myEffectiveTextRise);
	//setEffectiveTextRise(0.0);
	//result = result + "\n";
	addControl(myEffectiveTextRise > 0 ? SUP : SUB, false);
	bool previousBBoldness = myBBoldness;
	if(previousBBoldness)
		setBoldness(false);
	endParagraph();
	beginParagraph();
	myIsFirstOnLine = true;
	if(fabs(myEffectiveTextRise) > EPSILON)
		addControl(myEffectiveTextRise > 0 ? SUP : SUB, true);
	if(previousBBoldness)
		setBoldness(true);
}

shared_ptr<PdfFont> PdfBookReader::loadFont(shared_ptr<PdfObject> metadata, ZLInputStream &stream) {
	assert(!metadata.isNull() && metadata->type() == PdfObject::DICTIONARY);
	const PdfDictionaryObject& fontDictionary = (const PdfDictionaryObject&)*metadata;
	shared_ptr<PdfObject> widths = fontDictionary["Widths"];
	shared_ptr<PdfObject> toUnicode = fontDictionary["ToUnicode"];
	shared_ptr<PdfObject> encoding = fontDictionary["Encoding"];
	shared_ptr<PdfObject> fontDescriptor = fontDictionary["FontDescriptor"];
	if(!fontDescriptor.isNull()) {
		shared_ptr<PdfObject> fontFileRefO = ((const PdfDictionaryObject&)*fontDescriptor)["FontFile"];
		if(!fontFileRefO.isNull() && fontFileRefO->type() == PdfObject::REFERENCE) {
			const PdfObjectReference& fontFileRef = (const PdfObjectReference&) *fontFileRefO;
			//std::cout << "font file ref " << fontFileRef.number() << std::endl;
		}
	}
	shared_ptr<PdfObject> fontFile = fontDescriptor.isNull() ? 0 : ((const PdfDictionaryObject&)*fontDescriptor)["FontFile"];
	shared_ptr<PdfObject> ws;
	shared_ptr<PdfObject> resourcesObject = fontDictionary["Resources"]; // for symbol fonts.
	PdfXObjects xObjects = prepareXObjects(resourcesObject, stream); // for symbol fonts.
	shared_ptr<PdfObject> descendantFontsO = fontDictionary["DescendantFonts"];
	shared_ptr<PdfObject> descendantFontO;
	if(!descendantFontsO.isNull() && descendantFontsO->type() == PdfObject::ARRAY) {
		PdfArrayObject& descendantFonts = ((PdfArrayObject&)*descendantFontsO);
		if(descendantFonts.size() >= 1) { /* note: size should always be 1 according to standard. */
			descendantFontO = descendantFonts[0];
			if(!descendantFontO.isNull() && descendantFontO->type() == PdfObject::DICTIONARY && fontDescriptor.isNull()) { // fall back to descendant.
				const PdfDictionaryObject& descendantFont = (const PdfDictionaryObject&)*descendantFontO;
				/* TODO: note that this can contain "W" for the char's widths. */
				fontDescriptor = descendantFont["FontDescriptor"];
				ws = descendantFont["W"];
			}
		}
	}
	int widthsMissingWidth = 0;
	int widthsFirstChar = 0;
	if(!fontDescriptor.isNull())
		integerFromPdfObject(((const PdfDictionaryObject&)*fontDescriptor)["MissingWidth"], widthsMissingWidth);
	if(!integerFromPdfObject(fontDictionary["FirstChar"], widthsFirstChar))
		widthsFirstChar = 0;
	// TODO CharProcs
	return new PdfFont(metadata, fontDescriptor, encoding, fontFile, toUnicode, xObjects, widths, ws, widthsMissingWidth, widthsFirstChar);
}

void PdfBookReader::loadFonts(shared_ptr<PdfObject> resourcesObject, ZLInputStream &stream) {
	shared_ptr<PdfObject> resourceFontsObject = 0;
	if(!resourcesObject.isNull() && resourcesObject->type() == PdfObject::DICTIONARY) {
		const PdfDictionaryObject &resourcesDictionary = (const PdfDictionaryObject &) *resourcesObject;
		resourceFontsObject = resourcesDictionary["Font"];
		//if(!resourceFontsObject.isNull())
		//	resourceFontsObject = resolveReference(resourceFontsObject, stream);
		if(!resourceFontsObject.isNull() && resourceFontsObject->type() == PdfObject::DICTIONARY) {
			/* /F1 ... */
			/*for each {
				processFont(resourceFontObject);
			}*/
		} else {
			resourceFontsObject = 0;
		}
	}
	if(resourceFontsObject.isNull())
		return;
	const PdfObject& fonts = *resourceFontsObject;
	if(fonts.type() != PdfObject::DICTIONARY) /* WTF */
		return;
		
	//std::map<shared_ptr<PdfObject>, shared_ptr<PdfToUnicodeMap> > fToUnicodeTables;

	std::list<shared_ptr<PdfObject> > keys = ((PdfDictionaryObject&)fonts).keys();
	for(std::list<shared_ptr<PdfObject> >::const_iterator iter_keys = keys.begin(); iter_keys != keys.end(); ++iter_keys) {
		shared_ptr<PdfObject> name = *iter_keys;
		//std::cout << "font " << ((PdfNameObject&)*name).id() << std::endl;
		shared_ptr<PdfObject> referenceX = ((PdfDictionaryObject&)fonts)[name];
		if(referenceX->type() != PdfObject::REFERENCE) { /* probably. */
			// Actually, if we did that right, the same font data could be referred to by multiple names.
			// Since here the font addresses are already resolved, not much can be done.
			// We therefore default to the pointer address.
			std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> >::const_iterator iter_table = fFontsByNativeAddress.find(referenceX);
			if(iter_table == fFontsByNativeAddress.end()) {
				fFontsByNativeAddress[referenceX] = loadFont(referenceX, stream);
			}
			fFontsByName[name] = fFontsByNativeAddress[referenceX];
		} else {
			int reference_number;
			int reference_generation;
			PdfObjectReference& reference = (PdfObjectReference&)*referenceX;
			reference_number = reference.number();
			reference_generation = reference.generation();
			PdfToUnicodeMapAddress address(reference_number, reference_generation);
			std::map<PdfToUnicodeMapAddress, shared_ptr<PdfFont> >::const_iterator iter_table = fFontsByAddress.find(address);
			if(iter_table == fFontsByAddress.end()) {
				//std::cerr << "************* loading font " << reference.number() << std::endl;
				shared_ptr<PdfObject> metadata = resolveReferences(referenceX, stream);
				fFontsByAddress[address] = loadFont(metadata, stream);
				/*PdfToUnicodeMap& map = *fToUnicodeTablesByAddress[address];
				loadFont(name, metadata, map, stream);*/
			}
			fFontsByName[name] = fFontsByAddress[address];
		}
	}
	
	/* /F1 ... */
	/*for each {
		processFont(resourceFontObject);
	}*/
}

void PdfBookReader::processContents(shared_ptr<PdfObject> contentsObject, ZLInputStream &stream) {
	std::list<shared_ptr<PdfObject> > contents;
	if(contentsObject->type() == PdfObject::ARRAY) {
		// this means that one text block can span _multiple_ of the array elements.
		const PdfArrayObject &array = (const PdfArrayObject&)*contentsObject;
		const size_t len = array.size();
		for (size_t i = 0; i < len; ++i) {
			contents.push_back(array[i]);
		}
	} else {
		contents.push_back(contentsObject);
	}
	if(contents.size() < 1)
		return;
	shared_ptr<PdfContent> content = PdfContentParser::parse(contents);
	const std::vector<shared_ptr<PdfInstruction> >& instructions = content->instructions();
	processInstructions(instructions);
}

static bool isLineImage(const shared_ptr<PdfObject>& image) {
	if(image->type() != PdfObject::STREAM)
		return false;
	PdfStreamObject& streamObject = (PdfStreamObject&)*image;
	shared_ptr<PdfObject> dictionary1 = streamObject.dictionary();
	PdfDictionaryObject& dictionary = (PdfDictionaryObject&)*dictionary1;
	shared_ptr<PdfObject> type_ = dictionary["Type"];
	if(type_.isNull() || type_->type() != PdfObject::NAME)
		return false;
	shared_ptr<PdfObject> subtype = dictionary["Subtype"];
	if(subtype.isNull() || subtype->type() != PdfObject::NAME)
		return false;
	if(((PdfNameObject&)*type_).id() != "XObject" ||
	   ((PdfNameObject&)*subtype).id() != "Image")
	   	return false;
	shared_ptr<PdfObject> widthO = dictionary["Width"];
	shared_ptr<PdfObject> heightO = dictionary["Height"];
	if(widthO.isNull() || heightO.isNull() || widthO->type() != PdfObject::INTEGER_NUMBER || heightO->type() != PdfObject::INTEGER_NUMBER)
		return false;
	int width = ((PdfIntegerObject&)*widthO).value();
	int height = ((PdfIntegerObject&)*heightO).value();
	if(width == 1 && height == 1)
		return true;
	shared_ptr<ZLInputStream> inputStreamO = streamObject.stream();
	ZLInputStream& inputStream = *inputStreamO;
	if(inputStream.open()) {
		// TODO check for /Filter/CCITTFaxDecode if that even works.
		char body[13];
		unsigned char* x = (unsigned char*) body;
		// TODO actually implement a CCITT T.4 decoder.
		if(inputStream.read(body, 13) == 13) {
			return *x++ == 0x35 &&
			       *x++ == 0x8d && 
			       *x++ == 0x63 && 
			       *x++ == 0x58 && 
			       *x++ == 0x00 && 
			       *x++ == 0x40 && 
			       *x++ == 0x04 && 
			       *x++ == 0x00 && 
			       *x++ == 0x40 &&
			       *x++ == 0x04 && 
			       *x++ == 0x00 && 
			       *x++ == 0x40 && 
			       *x++ == 0x04;
		}
	}
	return false;
}

void PdfBookReader::processInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions) {
	// this also keeps a state stack for most of the rendering parameters as per PDF standard.
	double prevPositionV = myPositionV;
	
	for(std::vector<shared_ptr<PdfInstruction> >::const_iterator iter_instructions = instructions.begin(); iter_instructions != instructions.end(); ++iter_instructions) {
		shared_ptr<PdfInstruction> instruction = *iter_instructions;
		if(instruction->operator_() == opBeginText) {
			const shared_ptr<PdfArrayObject>& arguments = instruction->arguments();
			myBPendingTextMatrixReset = true;
			myPendingTextMatrixH = myPendingTextMatrixBeginningH = myPendingTextMatrixV = 0.0;
			int argument_count = arguments->size();
			if(argument_count > 0) {
				shared_ptr<PdfObject> item = (*arguments)[0];
				if(item->type() == PdfObject::CONTENT) {
					const std::vector<shared_ptr<PdfInstruction> >& x_instructions = ((PdfContent&)(*item)).instructions();
					processInstructions(x_instructions);
				}
			}
		} else if(instruction->operator_() == opBeginInlineImage) {
		} else
			processInstruction(*instruction);
	}

	//setTextRise(prevTextRise);
	//FIXME re-add setPosition(0/*dummy*/, prevPositionV);
}

PdfXObjects PdfBookReader::prepareXObjects(shared_ptr<PdfObject> resourcesObject, ZLInputStream &stream) {
	PdfXObjects myXObjects2; /* name -> image-data */
	if(resourcesObject.isNull() || resourcesObject->type() != PdfObject::DICTIONARY) {
		myXObjects2.clear();
		return myXObjects2;
	}
	const PdfDictionaryObject& resourceDictionary = (const PdfDictionaryObject&)*resourcesObject;
	if (resourceDictionary["XObject"].isNull()) {
		myXObjects2.clear();
	} else if(resourceDictionary["XObject"]->type() == PdfObject::DICTIONARY) {
		shared_ptr<PdfObject> fXObjects = resourceDictionary["XObject"];
		/* some documents use a 1x1 XObject to be able to draw a line, so remember it here: */
		PdfDictionaryObject& xObjects = (PdfDictionaryObject&)*fXObjects;
		std::list<shared_ptr<PdfObject> > xObjectKeys = xObjects.keys();
		for(std::list<shared_ptr<PdfObject> >::const_iterator iter = xObjectKeys.begin(); iter != xObjectKeys.end(); ++iter) {
			const shared_ptr<PdfObject>& key = *iter;
			shared_ptr<PdfObject> image = xObjects[key];
			myXObjects2[key] = image;
			if(isLineImage(image)) {
				myLines.insert(key);
			}
		}
	} else {
		myXObjects2.clear();
	}
	return myXObjects2;
}

void PdfBookReader::processPage(shared_ptr<PdfObject> pageObject, ZLInputStream &stream) {
	myLeading = 0.0;
	myTextRise = 0.0;
	myPositionV = 0.0;
	myEffectiveTextRise = 0.0;
	myIsFirstOnLine = true;
	                        
	//pageObject = resolveReference(pageObject, stream);
	if (pageObject.isNull() || pageObject->type() != PdfObject::DICTIONARY) {
		return;
	}
	const PdfDictionaryObject &pageDictionary = *(const PdfDictionaryObject*)&*pageObject;
	if (pageDictionary["Type"].isNull() || pageDictionary["Type"] != PdfNameObject::nameObject("Page")) {
		return;
	}

	// TODO "UserUnit" for non-1/72 of an inch Tf unit.
	//pageDictionary.dump();
	resourcesObject = pageDictionary["Resources"];
	//resourcesObject = resolveReferences(resourcesObject, stream);
	loadFonts(resourcesObject, stream);
	myXObjects2 = prepareXObjects(resourcesObject, stream);
	shared_ptr<PdfObject> contents = pageDictionary["Contents"];
	if (contents.isNull()) {
		return;
	}
	beginParagraph();
	switch (contents->type()) {
		case PdfObject::STREAM:
		//case PdfObject::REFERENCE: // no.
		case PdfObject::ARRAY:
			processContents(contents, stream);
			break;
		default:
			break;
	}
	endParagraph();
	cancelSubscriptSuperscript();
}

/* precondition: prepareXObjects() was called. */
shared_ptr<PdfObject> PdfBookReader::findXObject(const shared_ptr<PdfObject>& name) const {
	PdfXObjects::const_iterator iter = myXObjects2.find(name);
	if(iter != myXObjects2.end())
		return iter->second;
	else
		return 0;
}

void PdfBookReader::addData(const std::string& text) {
	myIsFirstOnLine = false;
	BookReader::addData(text);
	if(text.length() > 0) {
		int count = text.length();
		int i = count - 1;
		unsigned char c;
		// find the first byte of a UTF-8 sequence.
		while(i >= 0 && ((c = (unsigned char) text[i]) >= 0x80 && c < 0xC0)) {
			--i;
		}
		if(i >= 0) {
			int j;
			for(j = 0; j < 4 && i < count; ++j, ++i)
				myPreviousCharacter[j] = text[i];
			myPreviousCharacter[j] = 0;
		} else
			myPreviousCharacter[0] = 0;
	}
}

void PdfBookReader::setBoldness(bool value) {
	if(myBBoldness == value)
		return;
		
	if(myBBoldness)
		addControl(BOLD, false);
	if(value)
		addControl(BOLD, true);
	myBBoldness = value;
}

void PdfBookReader::addWordSpace() {
	if(myPreviousCharacter[0] == '^')
		return;
	if(myPreviousCharacter[0] == 0xCB && myPreviousCharacter[1] == 0x86 && myPreviousCharacter[2] == 0) // circumflex-2
		return;
	if(myPreviousCharacter[0] == 0xCB && myPreviousCharacter[1] == 0x99 && myPreviousCharacter[2] == 0) // dot above
		return;
	if(myPreviousCharacter[0] == 0xE2 && myPreviousCharacter[1] == 0x86 && myPreviousCharacter[2] == 0x92 && myPreviousCharacter[3] == 0) // vector arrow
		return;
	if(myPreviousCharacter[0] == 0xC2 && myPreviousCharacter[1] == 0xA8 && myPreviousCharacter[2] == 0) // two dots above
		return;
	/* wrong delta if(myPreviousCharacter[0] == 0xCE && myPreviousCharacter[1] == 0x94 && myPreviousCharacter[2] == 0) // delta.
		return;*/
	if(myPreviousCharacter[0] == 0xE2 && myPreviousCharacter[1] == 0x88 && myPreviousCharacter[2] == 0x86 && myPreviousCharacter[3] == 0) // delta
		return;
	/*if(myPreviousCharacter[0] == '(' && myPreviousCharacter[1] == 0)
		return;*/
	/*if(myPreviousCharacter[0] == ')' && myPreviousCharacter[1] == 0)
		return;*/
	if(myPreviousCharacter[0] == ',')
		return;
	if(myPreviousCharacter[0] == '+' || myPreviousCharacter[0] == '-') // number sign, word break dash.
		return;
	if(myPreviousCharacter[0] == 0xE2 && myPreviousCharacter[1] == 0x88) // partial derivation
		return;
	if(myPreviousCharacter[0] == 0xCB && myPreviousCharacter[1] == 0x98) // ˘
		return;
	addData(" ");
}

// this will resolve references IN PLACE (as a memory usage optimization). Be careful.
shared_ptr<PdfObject> PdfBookReader::resolveReferences(shared_ptr<PdfObject> ref, ZLInputStream &stream) {
	while(!ref.isNull() && ref->type() == PdfObject::REFERENCE)
		ref = myParser->resolveReference(ref);
	if(ref.isNull())
		return(ref);
	assert(ref->type() != PdfObject::REFERENCE);
	switch(ref->type()) {
	case PdfObject::ARRAY:
		{
			if(myResolvedTraces.find(ref) != myResolvedTraces.end())
				return(ref);
			myResolvedTraces.insert(ref);
			PdfArrayObject& arr = (PdfArrayObject&) *ref;
			for(int i = 0, size = arr.size(); i < size; ++i) {
				arr[i] = resolveReferences(arr[i], stream);
				assert(arr[i].isNull() || arr[i]->type() != PdfObject::REFERENCE);
			}
		}
		break;
	case PdfObject::DICTIONARY:
		{
			if(myResolvedTraces.find(ref) != myResolvedTraces.end())
				return(ref);
			myResolvedTraces.insert(ref);
			PdfDictionaryObject& dict = (PdfDictionaryObject&) *ref;
			std::list<shared_ptr<PdfObject> > keys = dict.keys();
			for(std::list<shared_ptr<PdfObject> >::const_iterator iter = keys.begin(), end_iter = keys.end(); iter != end_iter; ++iter) {
				shared_ptr<PdfObject> key = *iter;
				//std::cout << "key " << ((PdfNameObject&) *key).id() << std::endl;
				shared_ptr<PdfObject> value = dict[key];
				value = resolveReferences(value, stream);
				assert(value.isNull() || value->type() != PdfObject::REFERENCE);
				dict[key] = value;
				assert(dict[key].isNull() || dict[key]->type() != PdfObject::REFERENCE);
			}
		}
		break;
	case PdfObject::STREAM:
		{
			if(myResolvedTraces.find(ref) != myResolvedTraces.end())
				return(ref);
			myResolvedTraces.insert(ref);
			PdfStreamObject& stream2 = (PdfStreamObject&) *ref;
			shared_ptr<PdfObject> dict = stream2.dictionary();
			dict = resolveReferences(dict, stream);
			stream2.setDictionary(dict);
		}
		break;
	//NIL,
	//REFERENCE done,
	//CONTENT,
	default:
		return(ref);
	}
	return(ref);
}

void PdfBookReader::setTextScale(double h, double v) {
	myTextScaleH = h;
	myTextScaleV = v;
}

