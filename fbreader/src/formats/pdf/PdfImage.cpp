#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <fstream> /* for debugging */
#include <ZLInputStream.h>
#include "PdfImage.h"
#include "PdfObject.h"
#include "PdfTiffWriter.h"
#include "PdfColorTable.h"
#include "PdfColorTableParser.h"

struct BMPMagic {
	unsigned char magic[2];// 0x42 0x4D (BM)
};

struct BMPFileHeader {
	uint32_t   size; /* of the entire file, in Bytes. */
	uint16_t   __reserved_1;
	uint16_t   __reserved_2;
	uint32_t   data_offset;
}; // total size = 14 bytes.

static void convertToLittleEndian(uint16_t& value) {
	// FIXME
}

static void convertToLittleEndian(uint32_t& value) {
	// FIXME
}

static void prepareStorage(struct BMPFileHeader& header) {
	assert(sizeof(header) == 12);
	convertToLittleEndian(header.size);
	convertToLittleEndian(header.__reserved_1);
	convertToLittleEndian(header.__reserved_2);
	convertToLittleEndian(header.data_offset);
}

#if 0
struct BMPDIBHeader {
	uint32_t header_size;
	uint32_t width;
	uint32_t height;
	uint16_t plane_count;
	uint16_t bits_per_pixel;
	uint32_t compression_type; // 0
	uint32_t data_size;
	uint32_t horizontal_resolution; // pixel per meter.
	uint32_t vertical_resolution; // pixel per meter.
	uint32_t palette_count;
	uint32_t palette_important_count;
};
#endif

struct BMPOS2Header {
	// header size (4 byte)
	uint16_t width;
	uint16_t height;
	uint16_t plane_count; // 1
	uint16_t bits_per_pixel; // 24. 32 doesn't work.
}; // total size = 12.

static void prepareStorage(struct BMPOS2Header& header) {
	assert(sizeof(header) == 8);
	convertToLittleEndian(header.width);
	convertToLittleEndian(header.height);
	convertToLittleEndian(header.plane_count);
	convertToLittleEndian(header.bits_per_pixel);
}

static void wrapBMP(int width, int height, int componentSize, int componentCount, size_t dataSize, std::ostream& destination) {
	// magic:
	destination << (char) 0x42 << (char) 0x4D;
	struct BMPFileHeader mainHeader;
	mainHeader.__reserved_1 = 1;
	mainHeader.__reserved_2 = 2;
	mainHeader.data_offset = 2+12+12;
	mainHeader.size = dataSize + mainHeader.data_offset;
	prepareStorage(mainHeader);
	destination.write((const char*) &mainHeader, sizeof(mainHeader));
	struct BMPOS2Header imageHeader;
	uint32_t os2Size = 12;
	convertToLittleEndian(os2Size);
	destination.write((const char*) &os2Size, sizeof(os2Size));
	imageHeader.width = width;
	imageHeader.height = height;
	imageHeader.plane_count = 1;
	imageHeader.bits_per_pixel = componentSize * componentCount;
	destination.write((const char*) &imageHeader, sizeof(imageHeader));
	prepareStorage(imageHeader);
}

struct TGAHeader {
   char  idlength; // str length.
   char  colourmaptype; // 0
   char  datatypecode; // 2=RGB
   short int colourmaporigin; // 0
   short int colourmaplength; // 0
   char  colourmapdepth; // 0
   short int x_origin; // 0
   short int y_origin; // 0
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor; // 0
};

void wrapTGA(int width, int height, int componentSize, int componentCount, std::ostream& destination, shared_ptr<PdfColorTable> colorTableO) {
	// TODO http://local.wasp.uwa.edu.au/~pbourke/dataformats/tga/
	int paletteEntryCount = 0;
	if(!colorTableO.isNull()) {
		PdfColorTable& colorTable = (PdfColorTable& ) *colorTableO;
		paletteEntryCount = colorTable.size();
	}
	destination << '\3'; /* length of image ID */
	destination << (colorTableO.isNull() ? '\0' : '\1'); /* palette flag */
	destination << (colorTableO.isNull() ? '\2' : '\1'); /* image type */
	destination << '\0' << '\0'; /* first palette index */
	destination << (unsigned char) (paletteEntryCount & 0xFF)  << (unsigned char) ((paletteEntryCount >> 8) & 0xFF);
	destination << '\0' << '\0'<< '\0'<< '\0'<< '\40';
	destination << (char) (width & 0x00FF) << (char) ((width & 0xFF00) >>8);
	destination << (char) (height & 0x00FF) << (char) ((height & 0xFF00) >>8);
	destination << (char) (componentSize * componentCount); // bits per pixel.
	destination << (char) 0;
	destination << "TGA"; /* image ID */
	if(!colorTableO.isNull()) {
		PdfColorTable& colorTable = (PdfColorTable& ) *colorTableO;
		for(int i = 0; i < paletteEntryCount; ++i) {
			PdfColorTableEntry entry = colorTable[i];
			destination << (unsigned char) entry.components[2];
			destination << (unsigned char) entry.components[1];
			destination << (unsigned char) entry.components[0];
		}
	}
}
void wrapTIFF(int width, int height, int componentSize, int size, int k, bool blackis1, std::ostream& destination) {
	destination << PdfTiffWriter(k, width, height, blackis1, size).str();
}

static std::string getMimeType(const shared_ptr<PdfObject>& xObject) {
	return "image/auto";
#if 0
	PdfStreamObject& stream1 = (PdfStreamObject&) *xObject;
	shared_ptr<ZLInputStream> stream = stream1.stream();
	if(stream.isNull())
		return "application/octet-stream";
	char magic[4096];
	size_t count = stream->read(magic, 4096);
	if(count >= 3 && memcmp(magic, "\377\330\377" /* \341 or \356 */, 3) == 0)
		return "image/jpeg";
	else if(count >= 6 && memcmp(magic, "GIF89a", 6) == 0)
		return "image/gif";
	else if(count >= 6 && memcmp(magic, "\211PNG\r\n", 6) == 0)
		return "image/png";
	// } else if (substr($first_line, 0, 2) == "BM" && unpack_I4(substr($first_line, 2, 4)) == filesize($path)) return "image/x-ms-bmp"
	return "application/octet-stream";
#endif
}

static int getComponentCount(PdfNameObject& colorSpace) {
	/* FIXME */
	std::cerr << "warning: PdfImage: getComponentCount: unknown ColorSpace " << colorSpace.id() << std::endl;
	return(3);
}
static shared_ptr<PdfColorTable> buildColorTable(const PdfDictionaryObject& dict) {
	shared_ptr<PdfObject> a = dict[PdfNameObject::nameObject("ColorSpace")];
	if(!a.isNull() && a->type() == PdfObject::ARRAY) {
		PdfArrayObject& arr = (PdfArrayObject&) *a;
		if(arr.size() >= 4) {
			shared_ptr<PdfObject> colorSpace = arr[0];
			shared_ptr<PdfObject> baseColorSpaceO = arr[1];
			shared_ptr<PdfObject> hivalO = arr[2];
			shared_ptr<PdfObject> paletteO = arr[3]; // image.dict[ColorSpace];
			if(!colorSpace.isNull() && !baseColorSpaceO.isNull() && !hivalO.isNull() && !paletteO.isNull() &&
			   colorSpace->type() == PdfObject::NAME && baseColorSpaceO->type() == PdfObject::NAME &&
			   hivalO->type() == PdfObject::INTEGER_NUMBER) {
				int hival = 0;
				if(colorSpace == PdfNameObject::nameObject("Indexed") && integerFromPdfObject(hivalO, hival) && hival < 256) {
					PdfNameObject& baseColorSpace = (PdfNameObject&) *baseColorSpaceO;
					int componentCount = getComponentCount(baseColorSpace);
					int count = hival + 1;
					shared_ptr<PdfColorTable> result(new PdfColorTable());
					if(paletteO->type() == PdfObject::STREAM) {
						PdfStreamObject& palette = (PdfStreamObject&) *paletteO;
						PdfColorTableParser parser(palette.stream(), componentCount, *result);
					} else if(paletteO->type() == PdfObject::STRING) {
						PdfStringObject& palette = (PdfStringObject&) *paletteO;
						PdfColorTableParser parser(palette.value(), componentCount, *result);
					}
					if(count == result->size()) {
						return(result);
					}
				}
			}
		}
	}
	return shared_ptr<PdfColorTable>();
}

PdfImage::PdfImage(shared_ptr<PdfObject> xObject) : ZLSingleImage(getMimeType(xObject)), myXObject(xObject) {
	assert(!myXObject.isNull() && myXObject->type() == PdfObject::STREAM);
}

const shared_ptr<std::string> PdfImage::stringData() const {
	PdfStreamObject& stream1 = (PdfStreamObject&) *myXObject;
	shared_ptr<ZLInputStream> stream = stream1.stream();
	std::stringstream result(std::ios_base::out | std::ios_base::binary);
#if 0
	/* doesn't work. */
	std::copy(std::istream_iterator<char>(*stream), std::istream_iterator<char>(), std::ostream_iterator<char>(result));
#endif
	ZLInputStream& inputStream = *stream;

	char buffer[4096];
	size_t count;
	while((count = inputStream.read(buffer, 4096)) > 0) {
		result.write(buffer, count);
	}
	std::string body = result.str();

	int myWidth;
	int myHeight;
	int myBitsPerComponent;
	int mySize;

	shared_ptr<PdfObject> imageDictionaryX = stream1.dictionary();
	if(!imageDictionaryX.isNull() && imageDictionaryX->type() == PdfObject::DICTIONARY) {
		const PdfDictionaryObject& imageDictionary = (const PdfDictionaryObject&) *imageDictionaryX;
		if(integerFromPdfObject(imageDictionary["Width"], myWidth) &&
		   integerFromPdfObject(imageDictionary["Height"], myHeight) &&
		   integerFromPdfObject(imageDictionary["BitsPerComponent"], myBitsPerComponent)
		 ) {
			result.str("");
			if(!imageDictionary["Filter"].isNull() &&
			    imageDictionary["Filter"]->type() == PdfObject::NAME) {
				if(imageDictionary["Filter"] == PdfNameObject::nameObject("DCTDecode") && !imageDictionary["ColorSpace"].isNull() && imageDictionary["ColorSpace"] == PdfNameObject::nameObject("DeviceRGB")) {
					/* JPEG */
					/*std::ofstream f("/tmp/image", std::ios_base::out | std::ios_base::binary);
					f << result.str() << body;*/
				} else if(imageDictionary["Filter"] == PdfNameObject::nameObject("CCITTFaxDecode") &&
				        integerFromPdfObject(imageDictionary["Length"], mySize)) {
					bool blackis1 = false; // FIXME
					shared_ptr<PdfObject> DecodeParms = imageDictionary["DecodeParms"];
					// find the one with the columns...
					shared_ptr<PdfObject> x = DecodeParms;
					if(!DecodeParms.isNull() && DecodeParms->type() == PdfObject::ARRAY) {
						const PdfArrayObject& DecodeParmsArray = (const PdfArrayObject&) *DecodeParms;
						for(int i = 0, count = DecodeParmsArray.size(); i < count; ++i) {
							shared_ptr<PdfObject> c = DecodeParmsArray[i];
							if(!c.isNull() && c->type() == PdfObject::DICTIONARY) {
								const PdfDictionaryObject& cDictionary = (const PdfDictionaryObject&) *c;
								if(!cDictionary["Columns"].isNull()) {
									// FIXME what legitimate purpose does "null" have in this context?
									x = c;
									break;
								}
							}
						}
					}
					DecodeParms = x;
					int cols = 1728;
					int rows = 0;
					int k = 0;
					if(!DecodeParms.isNull() && DecodeParms->type() == PdfObject::DICTIONARY) {
						const PdfDictionaryObject& DecodeParmsDictionary = (const PdfDictionaryObject&) *DecodeParms;
						if(!integerFromPdfObject(DecodeParmsDictionary["Columns"], cols))
							cols = myWidth;
						if(!integerFromPdfObject(DecodeParmsDictionary["Rows"], rows))
							rows = myHeight;
						if(DecodeParmsDictionary["BlackIs1"] == PdfBooleanObject::TRUE())
							blackis1 = true;
						if(!integerFromPdfObject(DecodeParmsDictionary["K"], k))
							k = 0;
					} else
						k = 0;
					wrapTIFF(cols, rows, myBitsPerComponent, mySize, k, blackis1, result);
				} else if(!imageDictionary["ColorSpace"].isNull() && imageDictionary["ColorSpace"] == PdfNameObject::nameObject("DeviceRGB")) { /* fallback (especially Flate and LZW) */
					shared_ptr<PdfColorTable> colorTable = buildColorTable(imageDictionary);
					wrapTGA(myWidth, myHeight, myBitsPerComponent, 3, /*body.length(),*/ result, colorTable);
				}
			} else {
				wrapTGA(myWidth, myHeight, myBitsPerComponent, 3, /*body.length(),*/ result, shared_ptr<PdfColorTable>());
			}
			return new std::string(result.str() + body);
		}
	}
	
	return new std::string(body);
}

