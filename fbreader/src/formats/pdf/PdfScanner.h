#ifndef __PDF_SCANNER_H
#define __PDF_SCANNER_H
/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <danny_milo@yahoo.com>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if !, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <shared_ptr.h>
#include <ZLInputStream.h>
#include <ZLStringInputStream.h>

class PdfScanner {
protected:
	shared_ptr<ZLInputStream> inputStream;
	int input;
	void raiseError(const char* expected, const char* got = NULL) {
		/* FIXME error printing */
		/* FIXME throw exception */
		char inputString[5] = {0, 0, 0, 0};
		inputString[0] = input;
		if(input == EOF) {
			inputString[0] = '\\';
			inputString[1] = 'E';
			inputString[2] = 'O';
			inputString[3] = 'F';
		} else if(input < 33) {
			inputString[0] = '\\';
			inputString[1] = 'A' + input - 1;
		}
		std::cerr << "PdfScanner parse error: expected " << expected << " but got " << (got ? got : inputString) << std::endl;
		std::cout << "***";
		while(input != EOF) {
			consume();
		}
		abort();
	}
	virtual shared_ptr<ZLInputStream> goToNextStream();
	int consume() {
		int old_input = input;
#ifdef DEBUG_STREAM
		if(old_input != EOF)
			std::cout << (char) old_input; /* FIXME */
		std::cout.flush();
#endif
		assert(!inputStream.isNull());
		if(input != EOF) {
			unsigned char c_input;
			if(inputStream->read((char*) &c_input, 1) < 1)
				input = EOF;
			else
				input = c_input;
			// was: input = inputStream->get();
		} else
			return old_input;
		if(input == EOF) {
			while(input == EOF && !(inputStream = goToNextStream()).isNull()) {
				unsigned char c_input;
				if(inputStream->read((char*) &c_input, 1) < 1)
					input = EOF;
				else
					input = c_input;
				// was: input = inputStream->get();
			}
		}
		if(input != EOF && input < 0) /* damn signed char */
			input = 256 + input;
		return old_input;
	}
	void consume(const char* expectedInput) {
		for(;*expectedInput;++expectedInput) {
			if(input != *expectedInput) {
				raiseError(expectedInput);
			}
			consume();
		}
	}
	void consume(char expectedInput) {
		char expectedInputString[2] = {0, 0};
		expectedInputString[0] = expectedInput;
		if(input != expectedInput) {
			raiseError(expectedInputString);
		}
		consume();
	}
	
	/* PDF specific: */
	std::string parseHexadecimalStringBody();
	std::string parseBracedString();
	int parseInteger();
	unsigned parsePositiveInteger();
	double parseFloatingFraction();
	void parseNewline();
	char parseEscapedAttributeNameCharBody();
	char parseOctalEscapeBody(char aFirstChar);
	void parseComment();
	void parseOptionalWhitespace();
	void parseWhitespace();
	void parseSingleWhitespace();
	std::string parseAttributeName();
	void scan(std::string aNeedle);
	
public:
	PdfScanner(shared_ptr<ZLInputStream> stream) : inputStream(stream), input(0) {
	}
	PdfScanner(const std::string& inputString) : inputStream(new ZLStringInputStream(inputString)), input(0) {
	}
	virtual ~PdfScanner() {}
};

#endif /* ndef __PDF_SCANNER_H */
