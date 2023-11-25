/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <assert.h>
#include "PdfScanner.h"

std::string PdfScanner::parseHexadecimalStringBody() {
  unsigned char hexDigit1, hexDigit2;
  std::string result = "";
  while(input != '>' && input != EOF) {
    if((input >= '0' && input <= '9') || (input >= 'A' && input <= 'F') || (input >= 'a' && input <= 'f'))
      hexDigit1 = toupper(consume()) - '0';
    else
      raiseError("[0-9A-F]");

    if((input >= '0' && input <= '9') || (input >= 'A' && input <= 'F') || (input >= 'a' && input <= 'f'))
      hexDigit2 = toupper(consume()) - '0';
    else {
      while(input != '>' && input != EOF)
        consume();
      hexDigit2 = 0; // as per standard.
    };

    if(hexDigit1 > 9 )
      hexDigit1 = hexDigit1 - 7; // dist between '9' and 'A', plus 10.
      
    if(hexDigit2 > 9 )
      hexDigit2 = hexDigit2 - 7; // dist between '9' and 'A', plus 10.
            
    result = result + (char) ((hexDigit1 << 4) | hexDigit2);
  };
  return result;
};

std::string PdfScanner::parseBracedString() {
  unsigned vNestingLevel;
  char vEscaped;
  vNestingLevel = 0;
  std::string result = "";
  
  consume('(');
  while (input != ')' || vNestingLevel > 0) {
    if(input == EOF)
      break;
    if(input == '(' )
      ++vNestingLevel;
    else if(input == ')' )
      --vNestingLevel;
    else if(input == 13 ) { // only leave #10 in the string.
      consume();
      if(input != 10) // whoops, killed the newline?
        result = result + (char)10;
    } else if(input == '\\' ) {
      consume();
      vEscaped = consume();
      switch(vEscaped) {
      case 'n': vEscaped = 10; break;
      case 'r': vEscaped = 13; break;
      case 't': vEscaped = 9; break;
      case 'b': vEscaped = 8; break;
      case 'f': vEscaped = 12; break;
      case '(': vEscaped = '('; break;
      case ')': vEscaped = ')'; break;
      case '\\': vEscaped = '\\'; break;
      case 10:
      case 13:
        {
          parseOptionalWhitespace();
          continue;
        };
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
         vEscaped = parseOctalEscapeBody(vEscaped);
      };
      result = result + vEscaped;
      continue;
    };
      
    result = result + (char) consume();
  };
  consume(')');
  return result;
};

int PdfScanner::parseInteger() {
  unsigned vAbsolute;
  int result = 1;
  
  if(input == '-' ) {
    result = -1;
    consume();
  } else if(input == '+' ) {
    consume();
  };
  
  vAbsolute = parsePositiveInteger();
  result = result * vAbsolute;
  return result;
};

unsigned PdfScanner::parsePositiveInteger() {
  int vDigit;
  unsigned result = 0;

  if(input < '0' || input > '9') 
    raiseError("<digit>");
      
  while(input >= '0' && input <= '9') {
    vDigit = input - '0';
    result = result * 10 + vDigit;
    
    consume();
  };
  return result;
};

/* parses starting with the dot. */
double PdfScanner::parseFloatingFraction() {
  double result;
  char* end_ptr = NULL;
  std::string vString = "0";
/*  if(input == '-' )
    vString = consume()
  else if(input == '+' ) 
    consume();
    
  if(! (input in ['0'..'9', '.']) )
    raiseError('[0-9.]');
  */
  if(input != '.' )
    raiseError(".");
    
  while((input >= '0' && input <= '9') || input == '.') 
    vString = vString + (char) consume();
  setlocale(LC_NUMERIC,"C");  /* FIXME shoot me now */
  result = strtod(vString.c_str(), &end_ptr);
  if(end_ptr && *end_ptr) { /* failed conversion */
    raiseError("<number>");
  }
  return result; // loses precision and since object IDs are in this format too.... StrToFloat(vString);
};

/* TODO inline */
char PdfScanner::parseEscapedAttributeNameCharBody() {
    unsigned char hexDigit1;
    unsigned char hexDigit2;
    if((input >= '0' && input <= '9') || (input >= 'A' && input <= 'F') || (input >= 'a' && input <= 'f'))
      hexDigit1 = toupper(consume()) - '0';
    else
      raiseError("[0-9A-Fa-f]");

    if((input >= '0' && input <= '9') || (input >= 'A' && input <= 'F') || (input >= 'a' && input <= 'f'))
      hexDigit2 = toupper(consume()) - '0';
    else
      raiseError("[0-9A-Fa-f]");

    if(hexDigit1 > 9 )
      hexDigit1 = hexDigit1 - 7; // dist between '9' and 'A', plus 10.
      
    if(hexDigit2 > 9 )
      hexDigit2 = hexDigit2 - 7; // dist between '9' and 'A', plus 10.
            
    return ((hexDigit1 << 4) | hexDigit2);
};

void PdfScanner::parseNewline() {
  bool vBMatched = false;
  if(input == 13 ) {
    vBMatched = true;
    consume(13);
  };
  if(input == 10 ) {
    vBMatched = true;
    consume(10);
  };
  if(! vBMatched )
    consume(10);
};

char PdfScanner::parseOctalEscapeBody(char aFirstChar) {
  unsigned vCode;
  if(aFirstChar < '0' || aFirstChar > '7')
    raiseError("[01234567]");
  else
    vCode = aFirstChar - '0';
 
  if(input >= '0' && input <= '7') {
    vCode = (vCode << 3) | (consume() - '0');

    if(input >= '0' && input <= '7')
      vCode = (vCode << 3) | (consume() - '0');
  };
  // can be out of bounds!
  
  return vCode;
};

void PdfScanner::parseComment() {
  consume('%');
  while (input != 10 && input != 13 && input != EOF) 
    consume();
}

void PdfScanner::parseOptionalWhitespace() {
  while(input == 0 || input == 9 || input == 10 || input == 12 || input == 13 || input == 32 || input == '%') {
    if(input == '%')
      parseComment();
    else
      consume();
  }
}
void PdfScanner::parseSingleWhitespace() {
  if(input == 0 || input == 9 || input == 10 || input == 12 || input == 13 || input == 32) {
    // TODO should this include a comment? multiple comments?
    consume();
  } else
    consume(32);
}
void PdfScanner::parseWhitespace() {
  if(input == '<' || input == '/' || input == '(' || input == '[' || input == '{') // exempt from whitespace.
    return;
    
  if(input == 0 || input == 9 || input == 10 || input == 12 || input == 13 || input == 32 || input == '%') {
    if(input == '%')
      parseComment();
    else
      consume();
  } else
    consume(32);

  parseOptionalWhitespace(); 
}

std::string PdfScanner::parseAttributeName() {
  std::string result = "";
  //if(input == '/' ) {
    //result = '/';
    consume('/');
  //};
  // '!'..'~', then '#AB'
  // '#' is valid...
  while (input >= '!' && input <= '~' && !(input == '[' || input == ']' || input == '<' || input == '>' || input == '(' || input == ')' || input == '/' || input == '{' || input == '}')) {
    if(input == '#' ) { // escape.
      consume();
      result = result + parseEscapedAttributeNameCharBody();
    } else {
      assert(input != EOF);
      result = result + (char) input;
      consume();
    };
  };

/*  '/Length'
  '/Filter'
  '/Type'
  '/Contents'
  '/Resources'
  */
  return result;
};

void PdfScanner::scan(std::string aNeedle) {
  int vMatchCount;
  int vMatchMaximum;
  vMatchMaximum = aNeedle.length();
  vMatchCount = 0;
  
  assert(vMatchMaximum > 0);
  
  while (vMatchCount < vMatchMaximum && input != EOF) {
    if(input == aNeedle[vMatchCount] ) {
      ++vMatchCount;
    } else
      vMatchCount = 0;
    
    consume();
  };
};

shared_ptr<ZLInputStream> PdfScanner::goToNextStream() {
	return shared_ptr<ZLInputStream>();
}

