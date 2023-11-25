#ifndef __PDF_MATRIX_H
#define __PDF_MATRIX_H

class PdfMatrix {
private:
	double items[3][3];
public:
	PdfMatrix(void);
	PdfMatrix(double diagonal);
	PdfMatrix& setFromRowVector(double x, double y, double w);
	PdfMatrix& setFromTranslation(double x, double y);
	PdfMatrix& setFromItems(double a, double b, double c,
	                        double d, double e, double f,
	                        double g, double h, double i);
	PdfMatrix multiply(const PdfMatrix& b) const;
	PdfMatrix add(const PdfMatrix& b) const;
	double getItem(int i, int j) const {
		return items[i][j];
	}
};

PdfMatrix operator*(const PdfMatrix& a, const PdfMatrix& b);
PdfMatrix operator+(const PdfMatrix& a, const PdfMatrix& b);
PdfMatrix makePdfMatrixFromRowVector(double a, double b, double c);
PdfMatrix makePdfMatrixFromTranslation(double x, double y);
PdfMatrix makePdfMatrixFromItems(double a, double b, double c,
	                        double d, double e, double f,
	                        double g, double h, double i);

#endif /* ndef __PDF_MATRIX_H */
