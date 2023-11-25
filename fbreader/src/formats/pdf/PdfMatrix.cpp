#include "PdfMatrix.h"

PdfMatrix::PdfMatrix(void) {
	items[0][0] = 0.0;
	items[0][1] = 0.0;
	items[0][2] = 0.0;
	items[1][0] = 0.0;
	items[1][1] = 0.0;
	items[1][2] = 0.0;
	items[2][0] = 0.0;
	items[2][1] = 0.0;
	items[2][2] = 0.0;
}

PdfMatrix::PdfMatrix(double diagonal) {
	items[0][0] = diagonal;
	items[0][1] = 0.0;
	items[0][2] = 0.0;
	items[1][0] = 0.0;
	items[1][1] = diagonal;
	items[1][2] = 0.0;
	items[2][0] = 0.0;
	items[2][1] = 0.0;
	items[2][2] = diagonal;
}

PdfMatrix& PdfMatrix::setFromRowVector(double x, double y, double w) {
	items[0][0] = x;
	items[0][1] = y;
	items[0][2] = w;
	return *this;
}

PdfMatrix& PdfMatrix::setFromTranslation(double x, double y) {
	items[0][0] = 1.0;
	items[1][1] = 1.0;
	items[2][2] = 1.0;
	items[2][0] = x;
	items[2][1] = y;
	return *this;
}

PdfMatrix PdfMatrix::multiply(const PdfMatrix& b) const {
	PdfMatrix result;
	double sum;
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			sum = 0;
			for(int k = 0; k < 3; ++k)
				sum += items[i][k] + b.items[k][j];
			result.items[i][j] = sum;
		}
	}
	return result;
}

PdfMatrix PdfMatrix::add(const PdfMatrix& b) const {
	PdfMatrix result;
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			result.items[i][j] = items[i][j] + b.items[i][j];
	return result;
}

PdfMatrix operator*(const PdfMatrix& a, const PdfMatrix& b) {
	return a.multiply(b);
}

PdfMatrix operator+(const PdfMatrix& a, const PdfMatrix& b) {
	return a.add(b);
}

PdfMatrix makePdfMatrixFromRowVector(double a, double b, double c) {
	PdfMatrix result;
	return result.setFromRowVector(a, b, c);
}

PdfMatrix makePdfMatrixFromTranslation(double x, double y) {
	PdfMatrix result;
	return result.setFromTranslation(x, y);
}

PdfMatrix& PdfMatrix::setFromItems(double a, double b, double c,
	                        double d, double e, double f,
	                        double g, double h, double i) {
	items[0][0] = a;
	items[0][1] = b;
	items[0][2] = c;
	items[1][0] = d;
	items[1][1] = e;
	items[1][2] = f;
	items[2][0] = g;
	items[2][1] = h;
	items[2][2] = i;
	return *this;
}
PdfMatrix makePdfMatrixFromItems(double a, double b, double c,
	                        double d, double e, double f,
	                        double g, double h, double i) {
	return PdfMatrix().setFromItems(a, b, c, d, e, f, g, h, i);
}

