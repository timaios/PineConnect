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



#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>


#define LOG_ERROR(...)   Logger::Log(__FILE__, __LINE__, Logger::Error, __VA_ARGS__)
#define LOG_WARNING(...) Logger::Log(__FILE__, __LINE__, Logger::Warning, __VA_ARGS__)
#define LOG_INFO(...)    Logger::Log(__FILE__, __LINE__, Logger::Info, __VA_ARGS__)
#define LOG_VERBOSE(...) Logger::Log(__FILE__, __LINE__, Logger::Verbose, __VA_ARGS__)
#define LOG_DEBUG(...)   Logger::Log(__FILE__, __LINE__, Logger::Debug, __VA_ARGS__)



class Logger
{
public:

        enum LogLevel
        {
            	Error = 0,
            	Warning = 1,
                Info = 2,
            	Verbose = 3,
            	Debug = 4
        };

        static LogLevel logLevel() { return _logLevel; }

        static void setLogLevel(LogLevel logLevel);
        static void Log(const char *file, int line, LogLevel level, const char *message, ...);

        static const char *getLogLevelText(LogLevel logLevel);

private:

        Logger();

        static LogLevel _logLevel;
};

#endif // LOGGER_H
