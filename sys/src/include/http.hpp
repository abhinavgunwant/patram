#ifndef PATRAM_SYS_HTTP
#define PATRAM_SYS_HTTP

#include <string>
#include <vector>

void initCURL();
void cleanupCURL();
void downloadToU8Vector(std::string url, std::vector<uint8_t>& output);
void downloadToFile(std::string url, std::string location);
bool isValidURL(std::string location);

#endif // PATRAM_SYS_HTTP

