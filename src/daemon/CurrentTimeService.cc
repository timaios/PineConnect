/*
 *
 *  PineConnect - A companion daemon and app for the PineTime watch
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



#include "CurrentTimeService.h"

#include <stdlib.h>
#include <time.h>

#include "Logger.h"
#include "ManagedDevice.h"



#define UUID_SERVICE_CURRENT_TIME          "00001805-0000-1000-8000-00805f9b34fb"
#define UUID_CHARACTERISTIC_CURRENT_TIME   "00002a2b-0000-1000-8000-00805f9b34fb"

#define MAX_BUFFER_SIZE   32



CurrentTimeService::CurrentTimeService()
{
}


bool CurrentTimeService::run(ManagedDevice *device)
{
        // get the device's current time
        int devYear = 0;
        int devMonth = 0;
        int devDay = 0;
        int devHour = 0;
        int devMinute = 0;
        int devSecond = 0;
        uint8_t buffer[MAX_BUFFER_SIZE];
        int readBytes = device->readCharacteristic(UUID_CHARACTERISTIC_CURRENT_TIME, buffer, MAX_BUFFER_SIZE);
        if (readBytes < 7)
                LOG_WARNING("Could not read enough bytes from device %s.", device->address());
        {
                devYear = static_cast<int>(buffer[0]) + (static_cast<int>(buffer[1]) << 8);
                devMonth = static_cast<int>(buffer[2]);
                devDay = static_cast<int>(buffer[3]);
                devHour = static_cast<int>(buffer[4]);
                devMinute = static_cast<int>(buffer[5]);
                devSecond = static_cast<int>(buffer[6]);
                LOG_VERBOSE("Current time on device %s: %04d-%02d-%02d %02d:%02d:%02d",
                            device->address(),
                            devYear,
                            devMonth,
                            devDay,
                            devHour,
                            devMinute,
                            devSecond);
        }
        double devTimestamp = getComparableTimestamp(devYear, devMonth, devDay, devHour, devMinute, devSecond);

        // get current local time
        time_t rawTime;
        time(&rawTime);
        struct tm *timeInfo = localtime(&rawTime);
        double localTimestamp = getComparableTimestamp(timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

        // set the device's time if it deviates more than one second
        double delta = localTimestamp - devTimestamp;
        if (delta < 0.0)
                delta *= -1.0;
        if (delta > 1.0)
        {
                buffer[0] = static_cast<uint8_t>((timeInfo->tm_year + 1900) & 0xff);
                buffer[1] = static_cast<uint8_t>((timeInfo->tm_year + 1900) >> 8);
                buffer[2] = static_cast<uint8_t>(timeInfo->tm_mon + 1);
                buffer[3] = static_cast<uint8_t>(timeInfo->tm_mday);
                buffer[4] = static_cast<uint8_t>(timeInfo->tm_hour);
                buffer[5] = static_cast<uint8_t>(timeInfo->tm_min);
                buffer[6] = static_cast<uint8_t>(timeInfo->tm_sec);
                buffer[7] = static_cast<uint8_t>(0);   // fractions of seconds
                buffer[8] = static_cast<uint8_t>(0);   // reason for change
                int success = device->writeCharacteristic(UUID_CHARACTERISTIC_CURRENT_TIME, buffer, 9);
                if (success)
                {
                        LOG_INFO("Updated time on device %s: %04d-%02d-%02d %02d:%02d:%02d",
                                 device->address(),
                                 timeInfo->tm_year + 1900,
                                 timeInfo->tm_mon + 1,
                                 timeInfo->tm_mday,
                                 timeInfo->tm_hour,
                                 timeInfo->tm_min,
                                 timeInfo->tm_sec);
                }
                else
                        LOG_WARNING("Could not update time on device %s.", device->address());
                return success;
        }
        else
                return true;
}


double CurrentTimeService::getComparableTimestamp(int year, int month, int day, int hour, int minute, int second) const
{
        return static_cast<double>(year) * (60.0 * 60.0 * 24.0 * 31.0 * 12.0)
                        + static_cast<double>(month) * (60.0 * 60.0 * 24.0 * 31.0)
                        + static_cast<double>(day) * (60.0 * 60.0 * 24.0)
                        + static_cast<double>(hour) * (60.0 * 60.0)
                        + static_cast<double>(minute) * 60.0
                        + static_cast<double>(second);
}
