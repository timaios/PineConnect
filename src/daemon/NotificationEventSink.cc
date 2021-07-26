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



#include "NotificationEventSink.h"

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "Logger.h"



#define NOTIFICATIONS_CAPACITY_INITIAL   16
#define NOTIFICATIONS_CAPACITY_GROWTH    16

#define APP_NAME_LENGTH   32
#define APP_ICON_LENGTH   32
#define SUMMARY_LENGTH    64
#define BODY_LENGTH       256



NotificationEventSink::Notification::Notification()
{
        appName = new char[APP_NAME_LENGTH];
        appIcon = new char[APP_ICON_LENGTH];
        summary = new char[SUMMARY_LENGTH];
        body = new char[BODY_LENGTH];

        id = -1;
        appName[0] = '\0';
        appIcon[0] = '\0';
        summary[0] = '\0';
        body[0] = '\0';
}


NotificationEventSink::Notification::~Notification()
{
        delete[] appName;
        delete[] appIcon;
        delete[] summary;
        delete[] body;
}



NotificationEventSink::NotificationEventSink()
{
        _notificationsCapacity = NOTIFICATIONS_CAPACITY_INITIAL;
        _notificationsCount = 0;
        _notifications = new Notification*[_notificationsCapacity];
}


NotificationEventSink::~NotificationEventSink()
{
        clearNotificationQueue();
        delete[] _notifications;
}


int NotificationEventSink::matchesCount() const
{
        return 3;
}


const char *NotificationEventSink::match(int index) const
{
        switch (index)
        {
        case 0:
                return "type='method_call',interface='org.freedesktop.Notifications',member='Notify',eavesdrop='true'";
        case 1:
                return "type='method_return',sender='org.freedesktop.Notifications',eavesdrop='true'";
        case 2:
                return "type='signal',sender='org.freedesktop.Notifications',path='/org/freedesktop/Notifications',interface='org.freedesktop.Notifications',member='NotificationClosed'";
        default:
                return nullptr;
        }
}


void NotificationEventSink::inspectMessage(DBusMessage *message)
{
        // store Notify method call's parameters
        if (isMethodCall(message, "org.freedesktop.Notifications", "Notify"))
        {
                // grow the notifications list if necessary
                if (_notificationsCount == _notificationsCapacity)
                {
                        _notificationsCapacity += NOTIFICATIONS_CAPACITY_GROWTH;
                        Notification **newList = new Notification*[_notificationsCapacity];
                        for (int i = 0; i < _notificationsCount; i++)
                                newList[i] = _notifications[i];
                        delete[] _notifications;
                        _notifications = newList;
                        LOG_DEBUG("Notification list capacity grew to %d items.", _notificationsCapacity);
                }

                // create a new notification object
                Notification *notification = new Notification();
                _notifications[_notificationsCount] = notification;
                _notificationsCount++;

                // save the app's name
                DBusMessageIter paramsIter;
                dbus_message_iter_init(message, &paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *appName = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &appName);
                        strncpy(notification->appName, appName, APP_NAME_LENGTH - 1);
                        notification->appName[APP_NAME_LENGTH - 1] = '\0';
                }

                // skip the "replace ID"
                dbus_message_iter_next(&paramsIter);

                // save the app's icon
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *appIcon = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &appIcon);
                        strncpy(notification->appIcon, appIcon, APP_ICON_LENGTH - 1);
                        notification->appIcon[APP_ICON_LENGTH - 1] = '\0';
                }

                // save the summary
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *summary = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &summary);
                        strncpy(notification->summary, summary, SUMMARY_LENGTH - 1);
                        notification->summary[SUMMARY_LENGTH - 1] = '\0';
                }

                // save the body
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *body = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &body);
                        strncpy(notification->body, body, BODY_LENGTH - 1);
                        notification->body[BODY_LENGTH - 1] = '\0';
                }

                // done
                LOG_DEBUG("Got a Notify method call from %s.", notification->appName);
                return;
        }

        // store the assigned notification ID
        if (isMethodReturn(message, "org.freedesktop.Notifications", "Notify"))
        {
                // save the notification's ID
                DBusMessageIter paramsIter;
                dbus_message_iter_init(message, &paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_UINT32)
                {
                        dbus_uint32_t id = 0;
                        dbus_message_iter_get_basic(&paramsIter, &id);
                        if (_notificationsCount > 0)
                                _notifications[_notificationsCount - 1]->id = static_cast<int>(id);
                }

                // done
                LOG_DEBUG("Got a Notify method return.");
                return;
        }

        // remove the notification from the list if it has been closed
        if (isSignal(message, "org.freedesktop.Notifications", "NotificationClosed"))
        {
                // delete the notification with the received ID
                DBusMessageIter paramsIter;
                dbus_message_iter_init(message, &paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_UINT32)
                {
                        dbus_uint32_t id = 0;
                        dbus_message_iter_get_basic(&paramsIter, &id);
                        for (int i = 0; i < _notificationsCount; i++)
                        {
                                if (_notifications[i]->id == static_cast<int>(id))
                                {
                                        deleteNotificationAt(i);
                                        break;
                                }
                        }
                }

                // done
                LOG_DEBUG("Got a NotificationClosed signal.");
                return;
        }
}


const NotificationEventSink::Notification *NotificationEventSink::pendingNotification(int index) const
{
        // guard
        if ((index < 0) || (index >= _notificationsCount))
                return nullptr;

        return static_cast<const Notification *>(_notifications[index]);
}


void NotificationEventSink::clearNotificationQueue()
{
        for (int i = 0; i < _notificationsCount; i++)
        {
                delete _notifications[i];
                _notifications[i] = nullptr;
        }
        _notificationsCount = 0;
}


void NotificationEventSink::deleteNotificationAt(int index)
{
        // guard
        if ((index < 0) || (index >= _notificationsCount))
                return;

        delete _notifications[index];
        for (int j = index + 1; j < _notificationsCount; j++)
                _notifications[j - 1] = _notifications[j];
        _notificationsCount--;
        _notifications[_notificationsCount] = nullptr;
}
