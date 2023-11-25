/* PDF content parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdlib.h>
#include <assert.h>
#include "PdfContentParser.h"

// FIXME ensure args are indeed PdfStreamObjects, and not null.

PdfContentParser::PdfContentParser(std::list<shared_ptr<PdfObject> >& sources, shared_ptr<PdfContent> output) : 
	PdfScanner(""),
	//((PdfStreamObject&)*(*sources.begin())).stream()),
	myContent(output),
	mySources(sources)
{
	mySourcesIter = mySources.begin();
	/* debugging */
	shared_ptr<PdfObject> item = *mySourcesIter;
        if(item->type() != PdfObject::STREAM) {
		std::cerr << "PdfContentParser: warning: cannot parse non-stream" << std::endl;
	} else {
		/*PdfStreamObject& stream = (PdfStreamObject&) *item;
		std::cerr << "XX" << std::endl;
		stream.dump();*/
	}
}


shared_ptr<PdfInstruction> PdfContentParser::parseInstruction() {
  shared_ptr<PdfObject> vArgument;
  shared_ptr<PdfInstruction> result = new PdfInstruction();
  result->setOperator(opInvalid);
  parseOptionalWhitespace();
  while(result->operator_() == opInvalid) {
    if((input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z') || input == '"' || input == '\'') { // operator.
      result->setOperator(parseOperator());
      if(result->operator_() == opBeginText) {
        parseWhitespace();
        //Result.AddTextBlock(TextBlockBodyE()); // FIXME use.
        /*if(result->arguments()->size() != 0)
          abort();*/
        result->addArgument(parseTextBlockBodyE());
        //consume('E');
      } else if(result->operator_() == opBeginInlineImage) {
        //parseSingleWhitespace();
        result->addArgument(parseInlineImageBody());
       }
      break;
    } else { // argument.
      vArgument = parseValueOrBracedValueList();
      result->addArgument(vArgument);
    };
    parseOptionalWhitespace(); // yeah, really optional.
  };
  return result;
};

PdfOperator PdfContentParser::parseOperator() {
  // TODO: use 'switch', sort.

  if(input == 'B') {
    consume();
    switch(input) {
      case '*': {
        consume();
        return opPaintFillAndStrokeEvenOdd /*B**/;
      };
      case 'M': {
       consume();
       consume('C');
       return opBeginMarkedContentBlock/*BMC*/;
      };
      case 'D': {
        consume();
        consume('C');
        return opBeginMarkedContentBlockWithAttributes/*BDC*/;
      };
      case 'T': {
        consume();
        return opBeginText;
      };
      case 'X': {
        consume();
        return opBeginCompabilitySection /*BX*/;
      };
      case 'I': {
        consume();
        return opBeginInlineImage /*BI*/; /* ID will then follow */
      }
      default:
        return opPaintFillAndStroke /*B, like construct, f, construct, S*/;
       //raiseError('B[MT*]', 'B' + input);
    };
  } else if(input == 'C') {
    consume();
    consume('S');
    return opColorSetStrokingColorspace /*CS*/;
  } else if(input == 'D') {
    consume();
    switch(input) {
      case 'o': {
        consume();
        return opPaintExternalObject/*Do*/;
      };
      case 'P': {
        consume();
        return opSetMarkedContentPointWithAttributes/*DP*/;
      };
      default:
        raiseError("D[oP]", "D" + input);
    };
  } else if(input == 'E') {
    consume();
    switch(input) { 
      case 'T': {
        consume();
        return opEndText;
      };
      case 'M': {
        consume();
        consume('C');
        return opEndMarkedContentBlock/*EMC*/;
      };
      case 'X': {
        consume();
        return opEndCompabilitySection/*EX*/;
      };
      case 'I': {
        consume();
        return opEndInlineImage /*EI*/;
      }
      default:
        raiseError("E[TM]", "E" + input);
    };
  } else if(input == 'M') {
    consume();
    switch(input) { 
      case 'P': {
        consume();
        return opSetMarkedContentPoint/*MP*/;
      };
      default: { // 'M' alone.
        // consume();
        return opSetMiterLimit /*M*/;
      };
    };
  } else if(input == 'T') {
    consume();
    if(input == 'j') {
      consume();
      return opShowString;
    } else if(input == 'w') {
      consume();
      return opSetWordSpacing;
    } else if(input == 'c') {
      consume();
      if(input == 'm') { // FIXME is that right?
        consume();
        return opTransformationMatrixAppend;
      } else
        return opSetCharacterSpacing;
    } else if(input == 'J') {
      consume();
      return opShowStringWithVariableSpacing;
    } else if(input == 'f') {
      consume();
      return opSetFontAndSize;
    } else if(input == 'd') {
      consume();
      return opMoveCaret;
    } else if(input == 'm') {
      consume();
      return opSetTextMatrix /*Tm*/;
    } else if(input == 'L') {
      consume();
      return opSetTextLeading /*TL*/ /*for T*, ', "*/;
    } else if(input == '*') {
      consume();
      return opMoveCaretToStartOfNextLine/*T**/;
    } else if(input == 'r') {
      consume();
      return opSetTextRenderingMode/*Tr*/;
    } else if(input == 's') {
      consume();
      return opSetTextRise/*Ts*/;
    } else if(input == 'D') {
      consume();
      return opMoveCaretToStartOfNextLineAndOffsetAndSetLeading/*TD*/;
    } else if(input == 'z') {
      consume();
      return opSetHorizontalScaling/*Tz*/; // in percent of normal width.
    } else
      raiseError("<text command>", "T" + input);
  } else if(input == '\'') {
    consume();
    return opNextLineShowString;
  } else if(input == '"') {
    consume();
    return opNextLineSpacedShowString;
  } else if(input == 'w') {
    consume();
    return opSetLineWidth  /*w*/;
  } else if(input == 'J') {
    consume();
    return opSetLineCap    /*J*/;
  } else if(input == 'j') {
    consume();
    return opSetLineJoin   /*j*/;
  } else if(input == 'd') {
    consume();
    return opSetDashPattern/*d*/;
  } else if(input == 'i') {
    consume();
    return opFlatness      /*i*/;
  } else if(input == 'g') {
    consume();
    if(input == 's') {
      consume();
      return opSetParameterValue/*gs*/;
    } else
      return opColorSetNonstrokingGrayColor/*g*/;
  } else if(input == 'G') {
    consume();
    return opColorSetStrokingGrayColor/*G*/;
  } else if(input == 'k') {
    consume();
    return opColorSetNonstrokingCMYKColor /*k*/; // colorspace = CMYK, Color = <arguments>.
  } else if(input == 'K') {
    consume();
    return opColorSetStrokingCMYKColor /*K*/; // colorspace = CMYK, Color = <arguments>.
  } else if(input == 'q') {
    consume();
    return opPushGraphicsState;
  } else if(input == 'Q') {
    consume();
    return opPopGraphicsState;
  } else if(input == 'm') { 
    consume();
    return opPathBegin;
  } else if(input == 'l') {
    consume();
    return opPathAddLine;
  } else if(input == 'c') {
    consume();
    if(input == 's') {
      consume();
      return opColorSetNonstrokingColorspace /*cs*/;
    } else if(input == 'm') {
      consume();
      return opTransformationMatrixAppend /*cm*/; // FIXME remove dupe
    } else
      return opPathAddCubicBezier123;
  } else if(input == 'v') {
    consume();
    return opPathAddCubicBezier23 /*v*/;
  } else if(input == 'y') {
    consume();
    return opPathAddCubicBezier13 /*y*/;
  } else if(input == 'h') {
    consume();
    return opPathClose /*h*/;
  } else if(input == 'R') {
    consume();
    
    consume('G');
    return opColorSetStrokingRGBColor /*RG*/; // colorspace = RGB, Color = <arguments>.
  } else if(input == 'r') {
    consume();
    if(input == 'i') {
      consume();
      return opSetColorIntent/*ri*/;
    } else if(input =='g') {
      consume();
      return opColorSetNonstrokingRGBColor /*rg*/; // colorspace = RGB, Color = <arguments>
    } else {
      consume('e');
      return opPathRectangle /*re*/;
    };
  } else if(input == 'S') {
    consume();
    if(input == 'C') {
      consume();
      if(input == 'N') {
        consume();
        return opColorSetStrokingColor /*SCN*/;
      } else
        return opColorSetStrokingColorLimited /*SC*/; // in current color space.
    } else
      return opPaintStroke /*S*/;
  } else if(input == 's') {
    consume();
    if(input == 'c') {
      consume();
      if(input == 'n') {
        consume();
        return opColorSetNonstrokingColor /*scn*/;
      } else {
        return opColorSetNonstrokingColorLimited /*sc*/;
      }
    } else
      return opPaintCloseAndStroke /*s*/; // == h S.
  } else if(input == 'f' || input == 'F') {
    if(consume() == 'f') {
      if(input == '*') {
        consume();
        return opPaintFillEvenOdd /*f**/;
      } else
        return opPaintCloseAndFill /*f, F*/;
    } else
        return opPaintCloseAndFill /*f, F*/;
  } else if(input == 'b') {
    consume();
    if(input == '*') { 
      consume();
      return opPaintCloseAndFillAndStrokeEvenOdd /*b**/;
    } else
      return opPaintCloseAndFillAndStroke /*b*/;
  } else if(input == 'n') {
    consume();
    return opPaintNoPaint /*n*/;
  } else if(input == 'W') {
    consume();
    if(input == '*') {
      consume();
      return opClipIntersectEvenOdd /*W**/;
    } else
      return opClipIntersect /*W*/;
  } else if(input == 'I') {
      consume();
      switch(input) {
        case 'D': {
          consume();
          return opInlineImageData /*ID*/;
        } 
        default:
          raiseError("<command>");
      }
  } else
    raiseError("<command>");
  return opInvalid;
};

shared_ptr<PdfObject> PdfContentParser::parseInlineImageBody() {
  parseWhitespace();
  parseAttributesBody(false, 'I'); // TODO process
  /* TODO parse stupid attribute name abbreviations:
     BPC BitsPerComponent
     CS ColorSpace
     D  Decode
     DP DecodeParams
     F  Filter
     H  Height
     IM ImageMask
     Intent Intent
     I Interpolate
     W Width
     G DeviceGray
     RGB DeviceRGB
     CMYK DeviceCMYK
     I Indexed
     AHx ASCIIHexDecode
     A85 ASCII85Decode
     LZW LZWDecode
     Fl FlateDecode
     RL RunLengthDecode
     CCF CCITTFaxDecode
     DCT DCTDecode
  */
  consume('I');
  consume('D');
  parseSingleWhitespace();
  while(input != EOF) {
    if(input == 'E') {
      consume();
      if(input == 'I') { // FIXME make this nicer
        consume();
        break;
      }
    } else
      consume();
  }
  //if(parseOperator() != opEndInlineImage)
  //  raiseError("<end-of-inline-image>");
  // FIXME create PdfImage (if applicable)
  return shared_ptr<PdfObject>();
}
shared_ptr<PdfObject> PdfContentParser::parseTextBlockBodyE() { /* actually returns PdfContentParser */
  PdfOperator vOperator;
  shared_ptr<PdfInstruction> vInstruction;
  PdfContent* result = new PdfContent();
  vOperator = opInvalid;
  while (vOperator != opEndText && input != EOF) {
    vInstruction = parseInstruction();
    vOperator = vInstruction->operator_();
    myContent->addInstruction(vInstruction);
  };
  return result;
  /*} catch(...) {
    delete result;
    throw;
  }*/
};


shared_ptr<PdfContent> PdfContentParser::parse(std::list<shared_ptr<PdfObject> >& sources) {
  shared_ptr<PdfContent> result = new PdfContent();
  shared_ptr<PdfContentParser> parser = new PdfContentParser(sources, result);
  parser->consume();
  while(parser->input != EOF) {
    shared_ptr<PdfInstruction> vInstruction = parser->parseInstruction(); /* can recurse using opBeginText */
    result->addInstruction(vInstruction);
    parser->parseOptionalWhitespace();
  };
  return result;
};

/* BEGIN: really basic stuff */

shared_ptr<PdfObject> PdfContentParser::parseValue() {
  // FIXME other values.
  
  if(input == 't' ) {
    consume("true");
    return PdfBooleanObject::TRUE();
  } else if(input == 'f' ) {
    consume("false");
    return PdfBooleanObject::FALSE();
  } else if(input == 'n' ) {
    consume("null");
    return 0;
  } else {
    int result = (input != '.') ? parseInteger() : 0;
    if(input == '.' ) // floating-point, ! integer.
      return PdfRealObject::realObject(result + parseFloatingFraction() * ((result > 0) ? 1 : -1)); // converts to Double, I hope.
    return PdfIntegerObject::integerObject(result);
  };
};


// [1 2 3]
shared_ptr<PdfObject> PdfContentParser::parseSquareBracedValueList() { /* actually returns PdfArrayObject */
  int i = 0;

  PdfArrayObject* result = new PdfArrayObject();
  /* TODO try catch to delete */
  consume('[');
  parseOptionalWhitespace();

  while(input != ']' && input != EOF) {
    // /ID [<DEC7FD857A7F7236327743F487C9FBC7> <DEC7FD857A7F7236327743F487C9FBC7>] >>

    if(input == 'R' ) {
      //Grow(i + 1);
      raiseError("<not-object_reference>");
#if 0
      consume('R');
      assert(i >= 2);
      result[i - 2] = parseObjectReference(result[i - 2], result[i - 1]);
      result[i - 1] = variants.Null;
      //result[i] = variants.Null;
      Dec(i, 1);
#endif
    } else {
      result->addObject(parseValueOrBracedValueList()); // Value();
      ++i;
    };

    if(input == 10 || input == 13)
      parseNewline();
    else if(input == ']' )
      ;
    else
      parseOptionalWhitespace(); // aaargh! '(In)26(tro)-26(duction)-302(to)-302(tensors)]TJ'. you can't be serious.
    /*else if(input != ']' )
      Break; some PDFs have: [15/foo] */
  };

  parseOptionalWhitespace();
  consume(']');
  //OptionalWhitespace();
  return result;
};

shared_ptr<PdfObject> PdfContentParser::parseValueOrBracedValueList() {
  if(input == '[' )
    return parseSquareBracedValueList();
  else if(input == '/' )
    return PdfNameObject::nameObject(parseAttributeName()); // FIXME SymbolReference(AttributeName())
  else if(input == '<' ) {
    consume();
    if(input == '<' ) {
      consume('<');
      parseAttributesBody(true, '>'); /* note: ignored */
      consume(">>");
      return new PdfStringObject("");
    } else {
      std::string result = parseHexadecimalStringBody();
      consume('>');
      return new PdfStringObject(result);
    };
  } else if(input == '(' )
    return new PdfStringObject(parseBracedString());
  else
    return parseValue();
};

/* END: really basic stuff */

shared_ptr<ZLInputStream> PdfContentParser::goToNextStream() {
	//std::cerr << "PdfContentParser:: going to next stream" << std::endl;
	if(mySourcesIter == mySources.end())
		return shared_ptr<ZLInputStream>();
	shared_ptr<PdfObject> item = *mySourcesIter;
	++mySourcesIter;
	if(item->type() != PdfObject::STREAM)
		return shared_ptr<ZLInputStream>();
	return ((PdfStreamObject&)*item).stream();
}

shared_ptr<PdfObject> PdfContentParser::parseAttributesBody(bool BAllowObjectReferences, int delimiter) {
  // TODO actually store stuff somewhere.
  parseOptionalWhitespace();
  while(input && input != delimiter) {
    std::string key;
    parseAttribute(BAllowObjectReferences, key);
    //Result.Add(key, value);
    parseOptionalWhitespace();
  }
  return shared_ptr<PdfObject>();
}

shared_ptr<PdfObject> PdfContentParser::parseAttribute(bool BAllowObjectReferences, std::string& key) {
  shared_ptr<PdfObject> result;
  unsigned version;
  unsigned ID;
  key = parseAttributeName();
  parseWhitespace(); 
  // /Contents 3 0 R
  result = parseValueOrBracedValueList();
  //OptionalWhitespace();
  // FIXME allow references?
#if 0
  if(input == ' ') {
    parseWhitespace(); // OptionalSpaces();
    if Input in ['0'..'9'] then begin
      vVersion := parsePositiveInteger();
      parseWhitespace();
      consume('R');
      if(BAllowObjectReferences/*avoid reference cycles*/) {
        ID = atoi(result.c_str());
        Result := parseObjectReference(result, version);
        if(!result.isNull())
          IPDFObject(result).SetID(ID);
      } else
        result set null; // useful for Trailer parsing...
    }
  }
#endif
  parseOptionalWhitespace();
  return result;
}

