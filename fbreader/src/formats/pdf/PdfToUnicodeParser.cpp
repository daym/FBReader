/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <assert.h>
#include <sstream>
#include "PdfToUnicodeMap.h"
#include "PdfToUnicodeParser.h"

/* Convert UTF-16 to UCS-4.
 *
 * # UTF-16 surrogate pair:
 * 1st UTF-16 thing: 0b110110..........
 * 2st UTF-16 thing: 0b110111..........
 *
 * UTF-16 encodes codepoints >= 0x10000, so subtract 0x10000 before encoding.
 * vHighSurrogate = ((aValue - 0x10000) div 0x400) + 0xD800;
 * vLowSurrogate = ((aValue - 0x10000) % 0x400) + 0xDC00;
 * The code area U+D800...U+DBFF (high surrogates) and the code area U+DC00...U+DFFF (low surrogates) is reserved.
 * Lastly, if a CID does not map to a Unicode code point, the value 0xFFFD shall be used as its Unicode code point.
 */
static std::vector<unsigned int> UCS_4_code_from_UTF_16(const std::string& aValue) {
  unsigned int code;
  std::vector<unsigned int> result;
  for(size_t i = 0; i < aValue.length() - 1; i+=2) {
    // Compiler settings are such that char is signed, yay.
    unsigned int part = (((unsigned int) aValue[i]) << 8) | ((unsigned int) aValue[i + 1]);
    if(part >= 0xD800 && part <= 0xDBFF) { /* surrogate pair */
      i += 2;
      if(i < aValue.length() - 1) {
        // Compiler settings are such that char is signed, yay.
        unsigned int vLowSurrogate = (((unsigned int) aValue[i]) << 8) | ((unsigned int) aValue[i + 1]);
        if ((vLowSurrogate >= 0xDC00) && (vLowSurrogate <= 0xDFFF)) {
            code = 0x10000 + (part - 0xD800) * 0x400 + (vLowSurrogate - 0xDC00);
            result.push_back(code);
        } else {
            std::cerr << "warning: ignored invalid surrogate pair." << std::endl;
            result.push_back(0xFFFD);
        }
      } else {
        std::cerr << "warning: ignored incomplete surrogate pair." << std::endl;
        result.push_back(0xFFFD);
      }
    } else {
      code = part;
      result.push_back(code);
    }
  }
  return result;
};

void PdfToUnicodeParser::parseRangeBody() {
  std::string vNativeBeginning;
  std::string vNativeEnd;
  std::vector<unsigned int> vUnicodeCharacters;
  while(input != 'e') {
    vNativeBeginning = parseValue();
    parseWhitespace();
    vNativeEnd = parseValue();
    parseWhitespace();
    if(input == '[') { /* multiple characters */
		consume();
		parseOptionalWhitespace();
		std::stringstream result;
		size_t vPrefixLength = vNativeBeginning.size() - 1;
		assert(vNativeBeginning.substr(0, vPrefixLength) == vNativeEnd.substr(0, vPrefixLength));
		while(input != ']' && input != EOF && vNativeBeginning <= vNativeEnd) {
			std::string part = parseValue(); /* <006B> */ /* UTF-16BE */
			parseOptionalWhitespace();
			vUnicodeCharacters = UCS_4_code_from_UTF_16(part); /* TODO multi-character things (hex: <40213222> 4 digits are 1 character)) */
			fMap.addRange(vNativeBeginning, vNativeBeginning, vUnicodeCharacters);
			//std::cout << (int) vNativeBeginning[0] << "XX" << (int) vNativeBeginning[1] << std::endl;
			++vNativeBeginning[vPrefixLength]; /* FIXME */
		}
		consume(']');
		parseOptionalWhitespace();

    } else {
      vUnicodeCharacters = UCS_4_code_from_UTF_16(parseValue());
      parseWhitespace();
      fMap.addRange(vNativeBeginning, vNativeEnd, vUnicodeCharacters);
    }
    
  };
};

void PdfToUnicodeParser::parseCharBody()
{
  std::string vNativeBeginning;
  std::vector<unsigned int> vUnicodeCharacters;
  std::string temp;
  while(input != 'e') {
    vNativeBeginning = parseValue();
    parseWhitespace();
    temp = parseValue();
    assert(temp.length() > 0);
    vUnicodeCharacters = UCS_4_code_from_UTF_16(temp);
    //assert(vUnicodeCharacter != 4294967268); // if you get this, you have signed/unsigned char mismatch.
    parseWhitespace();

    //Writeln(Hex(vNativeBeginning), ' ==> ', vUnicodeCharacter);
    fMap.addRange(vNativeBeginning, vNativeBeginning, vUnicodeCharacters);
  };
};


void PdfToUnicodeParser::parse() {
  consume();
  parse_();
}



void PdfToUnicodeParser::parse_() {
  // "beginbfrange" || "beginbfchar"
  while(input != EOF) {
    scan("beginbf");
  
    if(input == EOF)
      break;
    
    if(input == 'r' ) {
      consume("range");
      parseWhitespace();
      parseRangeBody();
      parseOptionalWhitespace();
      consume("endbfrange");
    } else if(input == 'c' ) {
      consume("char");
      parseWhitespace();
      parseCharBody();
      parseOptionalWhitespace();
      consume("endbfchar");
    } else {
      std::string got = (std::string("beginbf") + (char)input);
      raiseError("beginbfrange|beginbfchar", got.c_str());
    };
    parseOptionalWhitespace();
  };
  
  // "/CMapType 2 def"

}

PdfToUnicodeParser::PdfToUnicodeParser(shared_ptr<ZLInputStream> stream, PdfToUnicodeMap& map) : PdfScanner(stream), fMap(map) {
}

std::string PdfToUnicodeParser::parseValue() {
	if(input == '<') {
		consume();
		if(input != '<') {
			std::string result = parseHexadecimalStringBody();
			consume('>');
			return result;
		} else {
			std::cerr << "warning: ignoring empty hex string." << std::endl;
			return "";
		}
	}else {
		if(input != '(') {
			std::cerr << "warning: ToUnicode table contains junk:";
			while(input != EOF) {
				std::cerr << (char) input;
				consume();
			}
			std::cerr << std::endl;
		}
		return parseBracedString();
	}
}

