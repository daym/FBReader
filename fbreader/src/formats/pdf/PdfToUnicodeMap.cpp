/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <assert.h>
#include <string.h>
#include <sstream>
#include <ZLUnicodeUtil.h>
#include "PdfToUnicodeMap.h"
#include "PdfDefaultCharMap.h"

static std::string unicharToUTF8(unsigned int aCodepoint) {
	char buf[5] = {0,0,0,0,0};
	buf[0] = (char) aCodepoint;
	/*buf[1] = 0;*/
	ZLUnicodeUtil::ucs4ToUtf8(buf, aCodepoint);
	return buf;
}

void PdfToUnicodeMap::addRange(const std::string aNativeBeginning, const std::string aNativeEnd, std::vector<unsigned int> aUnicodeBeginnings) {
	unsigned int nativeCode;
	unsigned char nativeBeginningTail;
	unsigned char nativeEndTail;
	if(aUnicodeBeginnings.size() > MAX_TARGET) {
	   std::cerr << "error: ToUnicode map doesn't support unicode target of length >14 (i.e. " << aUnicodeBeginnings.size() << ")" << std::endl;
	   return;
	}
	if(aUnicodeBeginnings.size() < 1) {
	   std::cerr << "error: ToUnicode map target is missing." << std::endl;
	   return;
	}
	unsigned int aUnicodeBeginning = aUnicodeBeginnings[0];
	assert(aNativeBeginning.size() > 0);
	assert(aNativeEnd.size() > 0);
	// common prefix
	size_t vPrefixLength = aNativeBeginning.size() - 1;
	assert(vPrefixLength == aNativeEnd.size() - 1);
	assert(aNativeBeginning.substr(0, vPrefixLength) == aNativeEnd.substr(0, vPrefixLength));
	nativeBeginningTail = aNativeBeginning[vPrefixLength];
	nativeEndTail = aNativeEnd[vPrefixLength];
	if(vPrefixLength == 0) {
		for(nativeCode = nativeBeginningTail; nativeCode <= nativeEndTail; ++nativeCode, ++aUnicodeBeginning) {
			if(fPrefixlessCodes[nativeCode] && fPrefixlessCodes[nativeCode] != aUnicodeBeginning) {
				std::cerr << "warning: PdfToUnicodeMap: code " << std::dec << (unsigned)nativeCode << " was mapped to unicode U+" << std::hex << fPrefixlessCodes[nativeCode] << " but is now remapped to unicode U+" << std::hex << aUnicodeBeginning << std::dec << std::endl;
			}
			fPrefixlessCodes[nativeCode] = aUnicodeBeginning;
			++fPrefixlessCodesUsed;
		}
		return;
	}
  
	struct PdfToUnicodeMapRangeEntry entry;
	assert(vPrefixLength <= sizeof(entry.nativePrefix)/sizeof(entry.nativePrefix[0]));
	if(vPrefixLength > 0)
	for(size_t vPrefixIndex = 0; vPrefixIndex < vPrefixLength; ++vPrefixIndex)
		entry.nativePrefix[vPrefixIndex] = aNativeBeginning[vPrefixIndex];
      
	entry.nativePrefixSize = vPrefixLength;
	entry.nativeBeginning = nativeBeginningTail;
	entry.nativeEnd = nativeEndTail;
	{
		struct PdfToUnicodeMapChars& en = entry.unicodeBeginnings;
		int i;
		int size = aUnicodeBeginnings.size();
		for(i = 0; i < size; ++i)
			en.items[i] = aUnicodeBeginnings[i];
		en.count = size;
	}
	fRanges.push_back(entry);  
}

PdfToUnicodeMap::PdfToUnicodeMap() {
	fPrefixlessCodesUsed = 0;
	for(int i = 0; i < 256; ++i)
		fPrefixlessCodes[i] = 0;
}

struct PdfToUnicodeMapChars PdfToUnicodeMap::nextUnicodeCodepoint(unsigned char*& aNativeString, size_t& aNativeStringSize) const {
  struct PdfToUnicodeMapChars result;
  unsigned char vNativeCharacter;
  int vPrefixCMP;
  unsigned int resultC = cNilCodepoint;
  result.count = 1;
  result.items[0] = resultC;
  
  assert(aNativeStringSize > 0);
  
  if((resultC = fPrefixlessCodes[*aNativeString]) != 0) {
    ++aNativeString;
    --aNativeStringSize;
    result.count = 1;
    result.items[0] = resultC;
    return result;
  }

  for(std::vector<struct PdfToUnicodeMapRangeEntry>::const_iterator iter_entries = fRanges.begin(); iter_entries != fRanges.end(); ++iter_entries) {
    const struct PdfToUnicodeMapRangeEntry& entry = *iter_entries;
    if((entry.nativePrefixSize > 0) and (aNativeStringSize >= entry.nativePrefixSize) ) {
        vPrefixCMP = memcmp(&entry.nativePrefix[0], aNativeString, entry.nativePrefixSize);
    } else if((entry.nativePrefixSize > 0) /* and ! (aNativeStringSize >== NativePrefixSize)*/ )
       vPrefixCMP = 1;
    else
       vPrefixCMP = 0;

      if((vPrefixCMP == 0) && (aNativeStringSize > entry.nativePrefixSize) ) {
        vNativeCharacter = *(aNativeString + entry.nativePrefixSize);

        if((vNativeCharacter >= entry.nativeBeginning) and (vNativeCharacter <= entry.nativeEnd) ) {
          aNativeString += entry.nativePrefixSize + 1;
          aNativeStringSize -= entry.nativePrefixSize + 1;
          resultC = entry.unicodeBeginnings.items[0] + (vNativeCharacter) - (entry.nativeBeginning);
          result = entry.unicodeBeginnings;
          result.items[0] = resultC; // FIXME carry.
          break;
      };
    };
  };
  return result;
};

size_t PdfToUnicodeMap::size() const {
  return fPrefixlessCodesUsed + fRanges.size();
}

std::string PdfToUnicodeMap::convertStringToUnicode(const std::string& aValue) const {
	unsigned char* fValue;
	unsigned char* fValuePrev;
	size_t fValueSize;
	size_t fPreviousValueSize;
	struct PdfToUnicodeMapChars fCodepoints;
	/*FILE* fff = fopen("/tmp/Q", "a");
	fputs(aValue.c_str(), fff);
	fclose(fff);*/
	if(this == 0)
		return aValue; /* FIXME "XXX";*/
	std::string result = ""; // '(' + aValue + ')';
	fValueSize = aValue.length();
	fValue = (unsigned char*) aValue.c_str(); /* FIXME just use unsigned char in the compiler settings */
	do {
		fPreviousValueSize = fValueSize;
		if(fValueSize == 0)
			break;
		fValuePrev = fValue;
		fCodepoints = nextUnicodeCodepoint(fValue, fValueSize);
		if(fCodepoints.count <= 0 || fCodepoints.items[0] == cNilCodepoint || fPreviousValueSize == fValueSize)
			break;
		for(int i = 0; i < fCodepoints.count; ++i) {
			unsigned int fCodepoint = fCodepoints.items[i];
			if(fCodepoint >= 0xFB00 && fCodepoint < 0xFB04) { // Replace ligatures. TODO still good?
				switch(fCodepoint) {
				case 0xFB00:
					result += "ff";
					break;
				case 0xFB01:
					result += "fi";
					break;
				case 0xFB02:
					result += "fl";
					break;
				case 0xFB03:
					result += "ffi";
					break;
				default: ;
				}
				continue;
			}
			result = result + unicharToUTF8(fCodepoint);
		}
	} while(fCodepoints.count > 0 && fCodepoints.items[0] != cNilCodepoint);
	return result;
}
