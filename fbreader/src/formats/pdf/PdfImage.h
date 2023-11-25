#ifndef __PDF_IMAGE_H
#define __PDF_IMAGE_H
#include <vector>
#include <iostream>
#include <ZLImage.h>
#include "PdfObject.h"

class PdfImage : public ZLSingleImage {
public:
	PdfImage(shared_ptr<PdfObject> xObject);
	virtual /* override */ const shared_ptr<std::string> stringData() const;
private:
	shared_ptr<PdfObject> myXObject;
};

//void wrapTGA(int width, int height, int componentSize, int componentCount, std::ostream& destination);

#endif /* __PDF_IMAGE_H */
