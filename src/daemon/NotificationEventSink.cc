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



NotificationEventSink::NotificationEventSink()
{
        _notifications.clear();
}


NotificationEventSink::~NotificationEventSink()
{
        clearNotificationQueue();
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
                // create a new notification object
                Notification *notification = new Notification();
                _notifications.push_back(notification);

                // save the app's name
                DBusMessageIter paramsIter;
                dbus_message_iter_init(message, &paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *appName = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &appName);
                        if (appName)
                                notification->appName = appName;
                }

                // skip the "replace ID"
                dbus_message_iter_next(&paramsIter);

                // save the app's icon
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *appIcon = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &appIcon);
                        if (appIcon)
                                notification->appIcon = appIcon;
                }

                // save the summary
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *summary = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &summary);
                        if (summary)
                                notification->summary = summary;
                }

                // save the body
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_STRING)
                {
                        const char *body = nullptr;
                        dbus_message_iter_get_basic(&paramsIter, &body);
                        if (body)
                                notification->body = body;
                }

                // go through the actions
                dbus_message_iter_next(&paramsIter);
                DBusMessageIter actionsIter;
                dbus_message_iter_recurse(&paramsIter, &actionsIter);
                while (true)
                {
                        if (dbus_message_iter_get_arg_type(&actionsIter) == DBUS_TYPE_STRING)
                        {
                                const char *action = nullptr;
                                dbus_message_iter_get_basic(&actionsIter, &action);
                                if (action)
                                        notification->actions.push_back(action);
                        }
                        if (dbus_message_iter_next(&actionsIter) == FALSE)
                                break;
                }

                // go through the hints dictionary
                dbus_message_iter_next(&paramsIter);
                DBusMessageIter hintsIter;
                dbus_message_iter_recurse(&paramsIter, &hintsIter);
                while (true)
                {
                        if (dbus_message_iter_get_arg_type(&hintsIter) == DBUS_TYPE_DICT_ENTRY)
                        {
                                // get the key
                                DBusMessageIter hintIter;
                                dbus_message_iter_recurse(&hintsIter, &hintIter);
                                const char *hintKey = nullptr;
                                if (dbus_message_iter_get_arg_type(&hintIter) == DBUS_TYPE_STRING)
                                        dbus_message_iter_get_basic(&hintIter, &hintKey);

                                // get the value
                                dbus_message_iter_next(&hintIter);
                                const char *hintValue = nullptr;
                                DBusMessageIter valueIter;
                                dbus_message_iter_recurse(&hintIter, &valueIter);
                                if (dbus_message_iter_get_arg_type(&valueIter) == DBUS_TYPE_STRING)
                                        dbus_message_iter_get_basic(&valueIter, &hintValue);

                                // store hint
                                if (hintKey && hintValue)
                                        notification->hints[hintKey] = hintValue;

                        }
                        if (dbus_message_iter_next(&hintsIter) == FALSE)
                                break;
                }

                // get the expiry timeout
                dbus_message_iter_next(&paramsIter);
                if (dbus_message_iter_get_arg_type(&paramsIter) == DBUS_TYPE_INT32)
                {
                        dbus_int32_t timeout = 0;
                        dbus_message_iter_get_basic(&paramsIter, &timeout);
                        notification->expiryTimeout = static_cast<int>(timeout);
                }

                // done
                LOG_DEBUG("Got a Notify method call from %s.", notification->appName.c_str());
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
                        if (!_notifications.empty())
                                _notifications.back()->id = static_cast<int>(id);
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
                        for (int i = 0; i < static_cast<int>(_notifications.size()); i++)
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
        if ((index < 0) || (index >= static_cast<int>(_notifications.size())))
                return nullptr;

        return static_cast<const Notification *>(_notifications[index]);
}


void NotificationEventSink::clearNotificationQueue()
{
        for (Notification *notification : _notifications)
                delete notification;
        _notifications.clear();
}


void NotificationEventSink::deleteNotificationAt(int index)
{
        // guard
        if ((index < 0) || (index >= static_cast<int>(_notifications.size())))
                return;

        delete _notifications[index];
        _notifications.erase(_notifications.begin() + index);
}
