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

#include <vector>
#include <map>
#include <string>



class NotificationEventSink : public DBusEventWatcher::EventSink
{
public:

        struct Notification
        {
        public:
                int id;
                std::string appName;
                std::string appIcon;
                std::string summary;
                std::string body;
                std::vector<std::string> actions;
                std::map<std::string, std::string> hints;
                int expiryTimeout;
        };


        NotificationEventSink();
        ~NotificationEventSink() override;

        const char *id() const override { return "NotificationEventSink"; }

        int matchesCount() const override;
        const char *match(int index) const override;

        void inspectMessage(DBusMessage *message) override;

        int pendingNotificationsCount() const { return static_cast<int>(_notifications.size()); }
        const Notification *pendingNotification(int index) const;
        void clearNotificationQueue();

private:

        std::vector<Notification *> _notifications;

        void deleteNotificationAt(int index);
};

#endif // NOTIFICATIONEVENTSINK_H
