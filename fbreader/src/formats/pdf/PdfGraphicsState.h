#ifndef __PDF_GRAPHICS_STATE_H
#define __PDF_GRAPHICS_STATE_H
/*PDF parser.
Copyright (C) 2008 Danny Milosavljevic <danny_milo@yahoo.com>

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if !, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

class PdfGraphicsState {
private:
	double myCTMScaleH;
	double myCTMScaleV;
public:
	PdfGraphicsState(double aCTMScaleH, double aCTMScaleV);
	PdfGraphicsState(const PdfGraphicsState& source);
	void setCTMScale(double aCTMScaleH, double aCTMScaleV);
	double CTMScaleH() const {
		return myCTMScaleH;
	}
	double CTMScaleV() const {
		return myCTMScaleV;
	}
};

#endif /* ndef __PDF_GRAPHICS_STATE_H */
