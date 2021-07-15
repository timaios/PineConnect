/*
 *
 *  Logger - A class and a set of macros for debugging and logging
 *
 *  Copyright (C) 2021  Tim Taenny
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */



#include "Logger.h"

#include <time.h>
#include <stdio.h>
#include <string.h>



Logger::LogLevel Logger::_logLevel = Logger::Info;



void Logger::setLogLevel(LogLevel logLevel)
{
        _logLevel = logLevel;
}


void Logger::Log(const char *file, int line, const LogLevel level, const char *message, ...)
{
        // get current time
        time_t currentTime;
        time(&currentTime);
        struct tm *timeInfo = localtime(&currentTime);

        va_list ap;
        if (_logLevel >= level)
        {
                // output time and log level
                fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d",
                        timeInfo->tm_year + 1900,
                        timeInfo->tm_mon + 1,
                        timeInfo->tm_mday,
                        timeInfo->tm_hour,
                        timeInfo->tm_min,
                        timeInfo->tm_sec);
                switch (level)
                {
                case Debug:
                        fprintf(stderr, " | DBG");
                        break;
                case Verbose:
                        fprintf(stderr, " | VRB");
                        break;
                case Info:
                        fprintf(stderr, " | INF");
                        break;
                case Warning:
                        fprintf(stderr, " | WRN");
                        break;
                case Error:
                        fprintf(stderr, " | ERR");
                        break;
                default:
                        break;
                }

                // strip path from file name
                const char *nameOnly = file;
                const char *lastSlash = strrchr(file, '/');
                if (lastSlash)
                        nameOnly = lastSlash + 1;

                // output file and line number
                char buffer[1024];
                snprintf(buffer, 1024, "%s:%d", nameOnly, line);
                fprintf(stderr, " | %-32s | ", buffer);

                // output message
                va_start(ap, message);
                vfprintf(stderr, message, ap);
                va_end(ap);
                fprintf(stderr, "\n");
        }
}


const char *Logger::getLogLevelText(LogLevel logLevel)
{
        switch (logLevel)
        {
        case Debug:
                return "Debug";
        case Verbose:
                return "Verbose";
        case Info:
                return "Info";
        case Warning:
                return "Warning";
        case Error:
                return "Error";
        default:
                return nullptr;
        }
}


Logger::Logger()
{
}
