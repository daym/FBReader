#ifndef __PDF_CONTENT_H
#define __PDF_CONTENT_H
#include <iostream>
#include <sstream>

#include <string>
#include <vector>
#include <map>

#include <shared_ptr.h>
/* cross dependency */
#include "PdfObject.h"
#include "PdfScanner.h"

typedef enum {
  // see Page 652.
               opInvalid, 
               opShowString /*Tj*/,
               opNextLineShowString /*'*/,
               opNextLineSpacedShowString /*"*/,
               opSetWordSpacing /*Tw*/,
               opSetCharacterSpacing /*Tc*/,
               opSetHorizontalScaling/*Tz*/,
               opSetFontAndSize /*Tf*/ /* second parameter: 0=auto-size */,
               opSetTextRenderingMode /*Tr*/,
               opSetTextRise /*Ts*/,
               opMoveCaret /*Td*/,
               opMoveCaretToStartOfNextLine/*T**/,
               opMoveCaretToStartOfNextLineAndOffsetAndSetLeading/*TD*/,
               opSetTextMatrix /*Tm*/,
               opShowStringWithVariableSpacing /*TJ*/,
               opSetTextLeading /*TL*/ /*for T*, ', "*/,
               opBeginText /*BT*/,
               opEndText /*ET*/,
               
               // special graphics state:
               
               opPushGraphicsState /*q*/,
               opPopGraphicsState /*Q*/,
               opTransformationMatrixAppend /*cm*/,

               // drawing:
               
               opColorSetStrokingColorspace /*CS*/,
               opColorSetStrokingColorLimited /*SC*/,
               opColorSetStrokingColor /*SCN*/,
               opColorSetStrokingGrayColor /*G*/,
               opColorSetStrokingRGBColor /*RG*/, // colorspace := RGB, Color := <arguments>.
               opColorSetStrokingCMYKColor /*K*/, // colorspace := CMYK, Color := <arguments>.
               
               opColorSetNonstrokingColorspace /*cs*/,
               opColorSetNonstrokingColorLimited /*sc*/,
               opColorSetNonstrokingColor /*scn*/,
               opColorSetNonstrokingGrayColor /*g*/, // DUPE.
               opColorSetNonstrokingRGBColor /*rg*/, // colorspace := RGB, Color := <arguments>
               opColorSetNonstrokingCMYKColor /*k*/, // colorspace := CMYK, Color := <arguments>.
               
               opPathBegin /*m*/,
               opPathAddLine /*l*/,
               opPathAddCubicBezier123 /*c*/,
               opPathAddCubicBezier23 /*v*/,
               opPathAddCubicBezier13 /*y*/,
               opPathClose /*h*/,
               opPathRectangle /*re*/,

               opPaintStroke /*S*/,
               opPaintCloseAndStroke /*s*/, // = h S.
               opPaintCloseAndFill /*f, F*/,
               opPaintFillEvenOdd /*f**/,
               opPaintFillAndStroke /*B, like construct, f, construct, S*/,
               opPaintFillAndStrokeEvenOdd /*B**/,
               opPaintCloseAndFillAndStroke /*b*/,
               opPaintCloseAndFillAndStrokeEvenOdd /*b**/,
               opPaintNoPaint /*n*/,
               opPaintExternalObject /*Do*/,
               
               opClipIntersect /*W*/,
               opClipIntersectEvenOdd /*W**/,
               
               // general graphics state:
               
               opSetLineWidth  /*w*/,
               opSetLineCap    /*J*/,
               opSetLineJoin   /*j*/,
               opSetMiterLimit /*M*/,
               opSetDashPattern/*d*/,
               opSetColorIntent/*ri*/,
               opFlatness      /*i*/,
               opSetParameterValue/*gs*/,
               
               // marks:
               
               opSetMarkedContentPoint/*MP*/,
               opSetMarkedContentPointWithAttributes/*DP*/,
               opBeginMarkedContentBlock/*BMC*/,
               opBeginMarkedContentBlockWithAttributes/*BDC*/,
               opEndMarkedContentBlock/*EMC*/,
               
               // compat:
               opBeginCompabilitySection/*EX*/,
               opEndCompabilitySection/*EX*/,
               
               opInlineImageData/*ID*/,
               opBeginInlineImage/*BI*/,
               opEndInlineImage/*EI*/,
} PdfOperator;

class PdfInstruction {
private:
	PdfOperator myOperator_;
	shared_ptr<PdfArrayObject> myArguments;
public:
	PdfInstruction() { myOperator_ = opInvalid; myArguments = new PdfArrayObject(); }
	PdfOperator operator_() const { return myOperator_; }
	shared_ptr<PdfArrayObject> arguments() const { return myArguments; } /* overkill. Can only contain simple types. TODO simplify. */
	void addArgument(shared_ptr<PdfObject> argument) { myArguments->addObject(argument); }
	void setOperator(PdfOperator value) { myOperator_ = value; }
	void dump() const { std::cerr << "instruction " << (int) myOperator_ << std::endl; }
	shared_ptr<PdfObject> extractFirstArgument() const;
	bool extractFloatArgument(double& a) const;
	std::vector<double> extractFloatArguments() const;
	bool extractTwoFloatArguments(double& a, double& b) const;
};

class PdfContent : public PdfObject {
private:
	std::vector<shared_ptr<PdfInstruction> > myInstructions;
public:
	PdfContent();
	virtual Type type() const { return CONTENT; }
	std::vector<shared_ptr<PdfInstruction> > instructions() const { return myInstructions; }
	void addInstruction(shared_ptr<PdfInstruction> item) { myInstructions.push_back(item); }
};

/*class PdfTextBlock : public PdfContent {
};*/

/* PdfInstruction helpers: */


#endif /* ndef __PDF_CONTENT_H */
