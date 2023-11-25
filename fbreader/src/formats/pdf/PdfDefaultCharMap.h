#ifndef __PDF_DEFAULT_CHAR_MAP_H
#define __PDF_DEFAULT_CHAR_MAP_H
#include <string>

#define cNilCodepoint 0xFFFD
unsigned int getUnicodeFromDefaultCharMap(const std::string& key);

#endif /* ndef __PDF_DEFAULT_CHAR_MAP_H */
