#include "utils/log.hpp"

#include <unistd.h>

#include <set>
#include <fstream>
#include <mutex>
#include <cstdlib>
#include <ios>
#include <sstream>
#include <string>

using namespace Log;

bool Internal::isSetup{};

std::set<std::string> Internal::debugModules;
bool Internal::debugAllModules{};

std::ofstream Internal::logFile;
std::mutex Internal::logMutex;

void Internal::setup() {
     if (isSetup) return;
     isSetup = true;

     // open log file
     const char* env_log_file = std::getenv("LSFG_LOG_FILE");
     if (env_log_file) {
        std::ostringstream filename;
        filename << getpid() << "_" << env_log_file;
        logFile.open(filename.str(), std::ios::app);
     }

     // parse debug modules
     const char* env_log_debug = std::getenv("LSFG_LOG_DEBUG");
     if (!env_log_debug)
         return;
     const std::string debugModulesStr(env_log_debug);

     std::stringstream ss(debugModulesStr);
     std::string item;
     while (std::getline(ss, item, ',')) {
         if (item == "all") {
             debugAllModules = true;
             return;
         }
         debugModules.insert(item);
     }
 }
