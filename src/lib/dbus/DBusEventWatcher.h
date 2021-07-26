/*
 *
 *  DBusEventWatcher - Listen for method calls and signals on the DBus
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



#ifndef DBUSEVENTWATCHER_H
#define DBUSEVENTWATCHER_H


#include <vector>
#include <dbus/dbus.h>



class DBusEventWatcher
{
public:

        class EventSink
        {
        public:
                EventSink();
                virtual ~EventSink();
                virtual const char *id() const = 0;
                virtual int matchesCount() const = 0;
                virtual const char *match(int index) const = 0;
                virtual void inspectMessage(DBusMessage *message) = 0;
        protected:
                bool isMethodCall(DBusMessage *message, const char *interface, const char *method);
                bool isMethodReturn(DBusMessage *message, const char *interface, const char *method);
                bool isSignal(DBusMessage *message, const char *interface, const char *signal);
        };


        DBusEventWatcher(bool watchSessionBus = true);
        ~DBusEventWatcher();

        void registerSink(EventSink *sink);

        bool checkQueue(int timeoutSecs);

private:

        DBusConnection *_connection;
        std::vector<EventSink *> _sinks;
};

#endif // DBUSEVENTWATCHER_H
