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



#include "AlertNotificationService.h"

#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "NotificationEventSink.h"
#include "ManagedDevice.h"



#define UUID_SERVICE_ALERT_NOTIFICATION                    "00001811-0000-1000-8000-00805f9b34fb"
#define UUID_CHARACTERISTIC_ALERT_NOTIFICATION_NEW_ALERT   "00002a46-0000-1000-8000-00805f9b34fb"
#define UUID_CHARACTERISTIC_ALERT_NOTIFICATION_CONTROL     "00002a44-0000-1000-8000-00805f9b34fb"
#define UUID_CHARACTERISTIC_ALERT_NOTIFICATION_EVENT       "00020001-78fc-48fe-8e23-433b3a1942d0"



AlertNotificationService::AlertNotificationService(NotificationEventSink *eventSink)
        : GattService()
{
        _eventSink = eventSink;
}


bool AlertNotificationService::run(ManagedDevice *device)
{
        // guard
        if (!device)
                return false;

        // send all pending alerts
        bool allSucceeded = true;
        for (int i = 0; i < _eventSink->pendingNotificationsCount(); i++)
        {
                // create an alert
                const NotificationEventSink::Notification *notification = _eventSink->pendingNotification(i);
                uint8_t buffer[64];
                buffer[0] = 1;    // category: e-mail
                buffer[1] = 1;    // number of new alerts (messages)
                buffer[2] = 34;   // E-mail icon
                strncpy(reinterpret_cast<char *>(buffer + 3), notification->summary.c_str(), 32);
                buffer[35] = 0;

                // send it
                bool success = device->writeCharacteristic(UUID_CHARACTERISTIC_ALERT_NOTIFICATION_NEW_ALERT, buffer, 36);
                if (success)
                        LOG_INFO("Sent notification to device %s: %s", device->address(), notification->summary.c_str());
                else
                {
                        LOG_WARNING("Could not send notification to device %s.", device->address());
                        allSucceeded = false;
                }
        }

        // done
        return allSucceeded;
}
