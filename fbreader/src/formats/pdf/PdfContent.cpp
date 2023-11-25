/* PDF content.
Copyright (C) 2008 Danny Milosavljevic <dannym+a@scratchpost.org>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdlib.h>
#include <assert.h>
#include "PdfContent.h"

PdfContent::PdfContent()
{
}

/* PdfInstruction */

shared_ptr<PdfObject> PdfInstruction::extractFirstArgument() const {
	const shared_ptr<PdfArrayObject>& arguments = this->arguments();
	int argument_count = arguments->size();
	if(argument_count == 0) /* !?!?! */
		return shared_ptr<PdfObject>();
	shared_ptr<PdfObject> name = (*arguments)[0];
	return name;
}

bool PdfInstruction::extractFloatArgument(double& a) const {
	shared_ptr<PdfObject> item = extractFirstArgument();
	return doubleFromPdfObject(item, a);
}

std::vector<double> PdfInstruction::extractFloatArguments() const {
	std::vector<double> result;
	double a;
	const shared_ptr<PdfArrayObject>& arguments = this->arguments();
	int argument_count = arguments->size();
	for(int i = 0; i < argument_count; ++i) {
		shared_ptr<PdfObject> item = (*arguments)[i];
		if(!doubleFromPdfObject(item, a))
			return std::vector<double>();
		result.push_back(a);
	}
	return result;
}

bool PdfInstruction::extractTwoFloatArguments(double& a, double& b) const {
	const shared_ptr<PdfArrayObject>& arguments = this->arguments();
	int argument_count = arguments->size();
	if(argument_count == 2) {
		shared_ptr<PdfObject> item = (*arguments)[0];
		if(!doubleFromPdfObject(item, a))
			return false;
		shared_ptr<PdfObject> item_2 = (*arguments)[1];
		if(!doubleFromPdfObject(item_2, b))
			return false;
		return true;
	} else
		return false;
}
