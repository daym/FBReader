/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <assert.h>
#include <math.h>
#include "PdfFont.h"
#include "PdfToUnicodeParser.h"
#include "PdfType1FontFileParser.h"
#include "PdfDefaultCharMap.h"

/* TODO have one place to define this */
#define EPSILON 0.0002           

/* widths is an array of:
   number [array]
Widths are in font coordinates.
 */
PdfFont::PdfFont(shared_ptr<PdfObject> metadata, 
                 shared_ptr<PdfObject> fontDescriptor, 
                 shared_ptr<PdfObject> encoding, 
                 shared_ptr<PdfObject> fontFile, 
                 shared_ptr<PdfObject> toUnicode, 
                 PdfXObjects resources, 
                 shared_ptr<PdfObject> widths, 
                 shared_ptr<PdfObject> ws,
                 int widthsMissingWidth,
                 int widthsFirstChar) : 
                 myResources(resources), 
                 myWidths(widths), 
                 myWs(ws), 
                 myWidthsMissingWidth(widthsMissingWidth),
                 myWidthsFirstChar(widthsFirstChar) {
	if(metadata->type() != PdfObject::DICTIONARY)
		return;
	PdfDictionaryObject& fontDictionary = (PdfDictionaryObject&)*metadata;
	if(fontDictionary["Type"].isNull() || fontDictionary["Type"] != PdfNameObject::nameObject("Font"))
		return;
	if(containsUnicode()) {
		parseEncoding(encoding);
		if(!parseToUnicodeTable(toUnicode)) {
			parseFontFile(fontFile);
		}
	}

	myScale = 0.001; // default as per spec.
	myBBoldness = false;
	std::string fontName = "?";
	if(!fontDescriptor.isNull()) {
		const PdfDictionaryObject& fontDescriptorDictionary = (const PdfDictionaryObject&)*fontDescriptor;
		doubleFromPdfObject(fontDescriptorDictionary["Descent"], myDescent);
		doubleFromPdfObject(fontDescriptorDictionary["Ascent"], myAscent);
		shared_ptr<PdfObject> fontNameX = fontDescriptorDictionary["FontName"];
		if(!fontNameX.isNull() && fontNameX->type() == PdfObject::NAME) {
			fontName = ((PdfNameObject&)*fontNameX).id();
			myBBoldness = fontName.find("Bold") != std::string::npos;
		}
		int flags;
		if(integerFromPdfObject(fontDescriptorDictionary["Flags"], flags)) {
			if(flags & (1 << 18)) { // force bold.
				myBBoldness = true;
			}
		}
		shared_ptr<PdfObject> fontBBoxX = fontDescriptorDictionary["FontBBox"];
		if(!fontBBoxX.isNull() && fontBBoxX->type() == PdfObject::ARRAY) {
			const PdfArrayObject& fontBBox = (const PdfArrayObject&)*fontBBoxX;
			double topBBox, bottomBBox;
			if(fontBBox.size() >= 4 && doubleFromPdfObject(fontBBox[1], topBBox) && doubleFromPdfObject(fontBBox[3], bottomBBox)) {
				if(myAscent < EPSILON && myDescent > -EPSILON) {
					myDescent = topBBox;
					myAscent = bottomBBox;
				}
#if 0
				myScale = fabs(bottomBBox - topBBox);
				if(myScale > EPSILON)
						myScale = 1/myScale;
				else {
					std::cerr << "warning: BBox of font \"" << fontName << "\" is invalid. Ignoring." << std::endl;
					myScale = 0.001;
				}
#endif
			}
		}
	} else {
		myDescent = myAscent = 0.0;
		std::cerr << "warning: no ascent/descent information for font." << std::endl;
	}
	
	
	shared_ptr<PdfObject> fontMatrixX = fontDictionary["FontMatrix"];
	if(!fontMatrixX.isNull() && fontMatrixX->type() == PdfObject::ARRAY) {
		const PdfArrayObject& fontMatrix = (const PdfArrayObject&)*fontMatrixX;
		if(fontMatrix.size() >= 1) {
			if(doubleFromPdfObject(fontMatrix[0], myScale)) {
			}
		}
		// 6 values. default: [0.001 0 0 0.001 0 0] for the default 1000 units glyph space = 1 unit text space.
	}

	if(myToUnicodeMap.size() == 0 && containsUnicode()) { 
		std::cerr << "warning: font \"" << fontName << "\" has no known unicode codepoints, although it should." << std::endl;
	}
	//std::cout << "font attrs: " << fontName << ": ascent " << myAscent << ", descent " << myDescent << std::endl;
	prepare();
}

static unsigned unicodeFromAnsiTableHigh[32] = {
	0x20AC, /* 128 */
	0x81, /* 129 */
	0x201A, /* 130 */
	0x192, /* 131 */
	0x201E, /* 132 */
	0x2026, /* 133 */
	0x2020, /* 134 */
	0x2021, /* 135 */
	0x2C6, /* 136 */
	0x2030, /* 137 */
	0x160, /* 138 */
	0x2039, /* 139 */
	0x152, /* 140 */
	0x8D, /* 141 */
	0x17D, /* 142 */
	0x8F, /* 143 */
	0x90, /* 144 */
	0x2018, /* 145 */
	0x2019, /* 146 */
	0x201C, /* 147 */
	0x201D, /* 148 */
	0x2022, /* 149 */
	0x2013, /* 150 */
	0x2014, /* 151 */
	0x2DC, /* 152 */
	0x2122, /* 153 */
	0x161, /* 154 */
	0x203A, /* 155 */
	0x153, /* 156 */
	0x9D, /* 157 */
	0x17E, /* 158 */
	0x178, /* 159 */
};

static unsigned unicodeFromMacRomanTableHigh[] = {
	0x00C4, /* 128 */
	0x00C5, /* 129 */
	0x00C7, /* 130 */
	0x00C9, /* 131 */
	0x00D1, /* 132 */
	0x00D6, /* 133 */
	0x00DC, /* 134 */
	0x00E1, /* 135 */
	0x00E0, /* 136 */
	0x00E2, /* 137 */
	0x00E4, /* 138 */
	0x00E3, /* 139 */
	0x00E5, /* 140 */
	0x00E7, /* 141 */
	0x00E9, /* 142 */
	0x00E8, /* 143 */
	0x00EA, /* 144 */
	0x00EB, /* 145 */
	0x00ED, /* 146 */
	0x00EC, /* 147 */
	0x00EE, /* 148 */
	0x00EF, /* 149 */
	0x00F1, /* 150 */
	0x00F3, /* 151 */
	0x00F2, /* 152 */
	0x00F4, /* 153 */
	0x00F6, /* 154 */
	0x00F5, /* 155 */
	0x00FA, /* 156 */
	0x00F9, /* 157 */
	0x00FB, /* 158 */
	0x00FC, /* 159 */
	0x2020, /* 160 */
	0x00B0, /* 161 */
	0x00A2, /* 162 */
	0x00A3, /* 163 */
	0x00A7, /* 164 */
	0x2022, /* 165 */
	0x00B6, /* 166 */
	0x00DF, /* 167 */
	0x00AE, /* 168 */
	0x00A9, /* 169 */
	0x2122, /* 170 */
	0x00B4, /* 171 */
	0x00A8, /* 172 */
	0x2260, /* 173 */
	0x00C6, /* 174 */
	0x00D8, /* 175 */
	0x221E, /* 176 */
	0x00B1, /* 177 */
	0x2264, /* 178 */
	0x2265, /* 179 */
	0x00A5, /* 180 */
	0x00B5, /* 181 */
	0x2202, /* 182 */
	0x2211, /* 183 */
	0x220F, /* 184 */
	0x03C0, /* 185 */
	0x222B, /* 186 */
	0x00AA, /* 187 */
	0x00BA, /* 188 */
	0x03A9, /* 189 */
	0x00E6, /* 190 */
	0x00F8, /* 191 */
	0x00BF, /* 192 */
	0x00A1, /* 193 */
	0x00AC, /* 194 */
	0x221A, /* 195 */
	0x0192, /* 196 */
	0x2248, /* 197 */
	0x2206, /* 198 */
	0x00AB, /* 199 */
	0x00BB, /* 200 */
	0x2026, /* 201 */
	0x00A0, /* 202 */
	0x00C0, /* 203 */
	0x00C3, /* 204 */
	0x00D5, /* 205 */
	0x0152, /* 206 */
	0x0153, /* 207 */
	0x2013, /* 208 */
	0x2014, /* 209 */
	0x201C, /* 210 */
	0x201D, /* 211 */
	0x2018, /* 212 */
	0x2019, /* 213 */
	0x00F7, /* 214 */
	0x25CA, /* 215 */
	0x00FF, /* 216 */
	0x0178, /* 217 */
	0x2044, /* 218 */
	0x20AC, /* 219 */
	0x2039, /* 220 */
	0x203A, /* 221 */
	0xFB01, /* 222 */
	0xFB02, /* 223 */
	0x2021, /* 224 */
	0x00B7, /* 225 */
	0x201A, /* 226 */
	0x201E, /* 227 */
	0x2030, /* 228 */
	0x00C2, /* 229 */
	0x00CA, /* 230 */
	0x00C1, /* 231 */
	0x00CB, /* 232 */
	0x00C8, /* 233 */
	0x00CD, /* 234 */
	0x00CE, /* 235 */
	0x00CF, /* 236 */
	0x00CC, /* 237 */
	0x00D3, /* 238 */
	0x00D4, /* 239 */
	0xF8FF, /* 240 */
	0x00D2, /* 241 */
	0x00DA, /* 242 */
	0x00DB, /* 243 */
	0x00D9, /* 244 */
	0x0131, /* 245 */
	0x02C6, /* 246 */
	0x02DC, /* 247 */
	0x00AF, /* 248 */
	0x02D8, /* 249 */
	0x02D9, /* 250 */
	0x02DA, /* 251 */
	0x00B8, /* 252 */
	0x02DD, /* 253 */
	0x02DB, /* 254 */
	0x02C7, /* 255 */
};

static std::vector<unsigned int> vec(unsigned int i) {
	std::vector<unsigned int> result;
	result.push_back(i);
	return(result);
}


void PdfFont::parseDefaultEncoding(shared_ptr<PdfObject> encoding) {
	if(encoding.isNull() || encoding->type() != PdfObject::NAME)
		return;
	if(encoding == PdfNameObject::nameObject("WinAnsiEncoding")) {
		// XXX FIXME provide actual conversion table.
		assert(myToUnicodeMap.size() == 0);
		char s[2] = {0, 0};
		char t[2] = {0, 0};
		s[0] = 1;
		t[0] = 127;
		myToUnicodeMap.addRange(s, t, vec(1));
		for(int i = 128; i < 160; ++i) {
			s[0] = i;
			myToUnicodeMap.addRange(s, s, vec(unicodeFromAnsiTableHigh[i - 128]));
		}
		for(int i = 160; i < 256; ++i) {
			s[0] = i;
			myToUnicodeMap.addRange(s, s, vec(i));
		}
	} else if(encoding == PdfNameObject::nameObject("MacRomanEncoding")) {
		char s[2] = {0, 0};
		char t[2] = {0, 0};
		s[0] = 1;
		t[0] = 127;
		myToUnicodeMap.addRange(s, t, vec(1));
		for(int i = 128; i < 256; ++i) {
			s[0] = i;
			myToUnicodeMap.addRange(s, s, vec(unicodeFromMacRomanTableHigh[i - 128]));
		}
	}
}

void PdfFont::parseEncoding(shared_ptr<PdfObject> encoding) {
	if(encoding.isNull())
		return;
	if(encoding->type() != PdfObject::DICTIONARY)
		return parseDefaultEncoding(encoding);
	unsigned int unicode;
	unsigned int nativeCode = 1;
	const PdfDictionaryObject& encodingDictionary = (const PdfDictionaryObject&)*encoding;
	shared_ptr<PdfObject> actual_data = encodingDictionary["Differences"];
	if(actual_data.isNull() || actual_data->type() != PdfObject::ARRAY)
		return;
	const PdfArrayObject& items = (const PdfArrayObject&)*actual_data;
	
	int count = items.size();	
	for(int i = 0; i < count; ++i) {
		shared_ptr<PdfObject> item = items[i];
		PdfObject& xitem = *item;
		if(xitem.type() == PdfObject::NAME) {
			PdfNameObject& name = (PdfNameObject&) xitem;
			std::string name_id = name.id();
			unicode = getUnicodeFromDefaultCharMap(name_id);
			if(unicode == cNilCodepoint && name_id.length() == 1) { // fallback for graphical fonts. These want you to leave the code alone.
				unicode = name_id[0];
				assert(unicode < 256);
			}
			if(unicode == cNilCodepoint) {
	                        std::cerr << "unknown encoding entry: /" << name_id << std::endl;
			} /*else FIXME*/ {
				std::string s2;
				s2.append(1, nativeCode);
				assert(s2.length() == 1);
				myToUnicodeMap.addRange(s2, s2, vec(unicode));
			}
			++nativeCode;
		} else if(xitem.type() == PdfObject::INTEGER_NUMBER) {
			PdfIntegerObject& number = (PdfIntegerObject&) xitem;
			nativeCode = number.value();
		} else {
			std::cerr << "warning: PdfFont: Encoding parser: ignored unknown PdfObject." << std::endl;
		}
	}
}

void PdfFont::parseFontFile(shared_ptr<PdfObject> actual_data) {
	// TODO it is possible for the FontDescriptor entry to be missing and an Resources entry to be there instead. Then, the Resources entry's value points to an XObject which contains an array of images that are the glyphs for the font. Handle these.
	// fall back to FontDescriptor, resolve, DICT, /Encoding|/FontFile.
	//dictionary.dump();
	if(!actual_data.isNull() && actual_data->type() == PdfObject::STREAM) {
		((PdfStreamObject&)*actual_data).dump(); /* FIXME remove */
		shared_ptr<ZLInputStream> actual_stream = ((PdfStreamObject&)*actual_data).stream();
		PdfType1FontFileParser parser(actual_stream, myToUnicodeMap);
		parser.parse();
	}
}

bool PdfFont::parseToUnicodeTable(shared_ptr<PdfObject> actual_data) {
	if(!actual_data.isNull()) {
		if(!actual_data.isNull() && actual_data->type() == PdfObject::STREAM) {
			shared_ptr<ZLInputStream> actual_stream = ((PdfStreamObject&)*actual_data).stream();
			PdfToUnicodeParser parser(actual_stream, myToUnicodeMap);
			parser.parse();
		}
		return true;
	} else 
		return false;
}

void PdfFont::prepare() {
	/* TODO non-integer widths? */
	int i;
	/* TODO prepare width cache for the usual characters */
	for(i = 0; i < 256; ++i)
		myCachedWidths[i] = -1;
	if(!myWidths.isNull() && myWidths->type() == PdfObject::ARRAY) {
		PdfArrayObject& items = (PdfArrayObject&) *myWidths;
		int size = items.size();
		for(i = 0; i < size; ++i) {
			int o = myWidthsFirstChar + i;
			if(o >= 0 && o < 256) {
				integerFromPdfObject(items[i], myCachedWidths[o]);
			}
		}
	}
	if(!myWs.isNull() && myWs->type() == PdfObject::ARRAY) {
		PdfArrayObject& items = (PdfArrayObject&) *myWs;
		int size = items.size();
		int width;
		int x_code = 0;
		for(i = 0; i < size; ++i) {
			shared_ptr<PdfObject> item = items[i];
			if(!item.isNull() && item->type() == PdfObject::INTEGER_NUMBER) {
				if(!integerFromPdfObject(item, x_code))
					return;
			} else { /* sub-array */
				PdfArrayObject& x_items = (PdfArrayObject&) *item;
				int x_size = x_items.size();
				for(int j = 0; j < x_size; ++j) {
					if(!integerFromPdfObject(x_items[j], width))
						width = -1;
					int o = x_code + j;
					if(o >= 0 && o < 256)
						myCachedWidths[o] = width;
				}
			}
		}
	}
	/*std::cerr << "cached widths: ";
	for(i = 0; i < 256; ++i) 
		if(myCachedWidths[i] != -1)
			std::cerr << myCachedWidths[i] << " *** ";
	std::cerr << std::endl;*/
}
int PdfFont::width(int code) const {
	if(code >= 0 && code < 256)
		return(myCachedWidths[code]);
	else
		return(-1); /* TODO traverse manually */
}
