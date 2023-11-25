/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <assert.h>
#include <sstream>
#include "PdfType1FontFileParser.h"
#include "PdfDefaultCharMap.h"


bool PdfType1FontFileParser::parse() {
  consume();
  if(input == EOF) { /* some PDFs have a zero-size stream here. */
    return false;
  }
  parse_();
  return true;
}

// FIXME parse this properly.

static std::vector<unsigned int> vec(unsigned int i) {
	std::vector<unsigned int> result;
	result.push_back(i);
	return(result);
}

void PdfType1FontFileParser::parse_() {
  // parse magic
  //printf("%X\n", (int) input);
  consume("%!");
  // "FontType1-1.0: SFBX1728 0.3"
  if(input != 'P') 
    return;
  consume("PS-AdobeFont-");
  while(input != EOF && input != '\n')
    consume();
    
  scan("/Encoding");
  parseWhitespace();
  if(input < '0' || input > '9') // "/Encoding StandardEncoding def" TODO; then "currentdict end" and then "currentfile eexec".
    return;
  parsePositiveInteger();
  parseWhitespace();
  consume("array");
  parseWhitespace();
  scan("dup");
  parseWhitespace();
  while(input != EOF) {
    unsigned localCode = parsePositiveInteger();
    parseWhitespace();
    std::string name = parseAttributeName();
    parseWhitespace();
    consume("put");
    parseWhitespace();
    assert(localCode < 256); /* && localCode >= 0);*/
    unsigned vUnicodeCharacter = getUnicodeFromDefaultCharMap(name);
    if(vUnicodeCharacter != cNilCodepoint) {
      std::string sNativeBeginning;
      sNativeBeginning.append(1, localCode);
      fMap.addRange(sNativeBeginning, sNativeBeginning, vec(vUnicodeCharacter));
    } else {
      std::cerr << "error: (Type1 font file parser) could not find character '/" << name << "' (for native code " << localCode << ")." << std::endl;
    }

    if(input != 'd')
      break;
    consume("dup");
    parseWhitespace();
  }
}

PdfType1FontFileParser::PdfType1FontFileParser(shared_ptr<ZLInputStream> stream, PdfToUnicodeMap& map) : PdfScanner(stream), fMap(map) {
}

