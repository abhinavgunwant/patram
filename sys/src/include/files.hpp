/**
 * This handles *mostly* everything to do with files.
 */
#ifndef PATRAM_SYS_FILES
#define PATRAM_SYS_FILES

#include <string>

std::string getHomeDirectory();
std::string getCacheDirectory();
bool cacheDirectoryExists();
void createCacheDirectory();

#endif // PATRAM_SYS_FILES



