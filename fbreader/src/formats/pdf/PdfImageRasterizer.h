#ifndef __PDF_IMAGE_RASTERIZER_H
#define __PDF_IMAGE_RASTERIZER_H
#ifdef USE_CAIRO
#include <vector>
#include <shared_ptr.h>
#include <ZLImage.h>
#include <cairo.h>
#include "PdfContent.h"
#include "PdfFont.h"
#include "PdfToUnicodeMap.h"

class PdfImageRasterizer : public ZLSingleImage {
private:
	double x, y;
	double tx, ty;
	cairo_surface_t* mySurface;
	cairo_t* myContext;
	const PdfToUnicodeMap* myToUnicodeTable; /* current one, depending on font. */
	std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> >& myFontsByName;
public:
	PdfImageRasterizer(std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> >& fontsByName);
	void processInstruction(const PdfInstruction& instruction);
	void processTextInstruction(const PdfInstruction& instruction);
	void transform(double x, double y, double& tx, double& ty);
	void processInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions); // non-text.
	void processTextInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions);
	virtual /* override */ const shared_ptr<std::string> stringData() const;
protected:
	void showString(shared_ptr<PdfObject> item);
	void setNonstrokingColor(std::vector<double> color);
	void appendTransformationMatrix(std::vector<double> components);
};
#endif
#endif /*ndef __PDF_IMAGE_RASTERIZER_H*/
