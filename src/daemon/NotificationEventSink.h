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



#ifndef NOTIFICATIONEVENTSINK_H
#define NOTIFICATIONEVENTSINK_H


#include "DBusEventWatcher.h"



class NotificationEventSink : public DBusEventWatcher::EventSink
{
public:

        struct Notification
        {
        public:
                Notification();
                ~Notification();
                int id;
                char *appName;
                char *appIcon;
                char *summary;
                char *body;
        };


        NotificationEventSink();
        ~NotificationEventSink() override;

        const char *id() const override { return "NotificationEventSink"; }

        int matchesCount() const override;
        const char *match(int index) const override;

        void inspectMessage(DBusMessage *message) override;

        int pendingNotificationsCount() const { return _notificationsCount; }
        const Notification *pendingNotification(int index) const;
        void clearNotificationQueue();

private:

        int _notificationsCapacity;
        int _notificationsCount;
        Notification **_notifications;

        void deleteNotificationAt(int index);
};

#endif // NOTIFICATIONEVENTSINK_H
