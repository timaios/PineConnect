/*
 *
 *  DBusWatcher - Listen for method calls and signals on the DBus
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



#include "DBusEventWatcher.h"

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "Logger.h"



#define SINK_LIST_CAPACITY_INITIAL   16
#define SINK_LIST_CAPACITY_GROWTH    8



DBusEventWatcher::EventSink::EventSink()
{
}


DBusEventWatcher::EventSink::~EventSink()
{
}


bool DBusEventWatcher::EventSink::isMethodCall(DBusMessage *message, const char *interface, const char *method)
{
        // guards
        if (!message || !interface || !method)
                return false;

        // there's a special method for that check
        return (dbus_message_is_method_call(message, interface, method) == TRUE);
}


bool DBusEventWatcher::EventSink::isMethodReturn(DBusMessage *message, const char *interface, const char *method)
{
        // guards
        if (!message || !interface || !method)
                return false;

        // check message type
        if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_RETURN)
                return false;

        // check interface
        const char *msgInterface = dbus_message_get_interface(message);
        if (msgInterface)
        {
                if (strcmp(msgInterface, interface) != 0)
                        return false;
        }

        // check method name
        const char *msgMethod = dbus_message_get_member(message);
        if (msgMethod)
        {
                if (strcmp(msgMethod, method) != 0)
                        return false;
        }

        // everything matches
        return true;
}


bool DBusEventWatcher::EventSink::isSignal(DBusMessage *message, const char *interface, const char *signal)
{
        // guards
        if (!message || !interface || !signal)
                return false;

        // there's a special method for that check
        return (dbus_message_is_signal(message, interface, signal) == TRUE);
}



DBusEventWatcher::DBusEventWatcher(bool watchSessionBus)
{
        // initialize
        _sinks.clear();

        // open DBus connection
        DBusError dbusError;
        dbus_error_init(&dbusError);
        if (watchSessionBus)
                _connection = dbus_bus_get(DBUS_BUS_SESSION, &dbusError);
        else
                _connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbusError);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Could not get a connection to DBus: %s", dbusError.message);
                dbus_error_free(&dbusError);
                _connection = nullptr;
                return;
        }
        if (!_connection)
        {
                LOG_ERROR("Could not get a connection to DBus.");
                return;
        }
        LOG_DEBUG("Established DBus connection. Unique name: %s", dbus_bus_get_unique_name(_connection));
}


DBusEventWatcher::~DBusEventWatcher()
{
}


void DBusEventWatcher::registerSink(EventSink *sink)
{
        // guard
        if (!sink)
                return;

        // add the sink
        _sinks.push_back(sink);
        LOG_DEBUG("Added event sink: %s", sink->id());

        // add the sink's matches
        int sinksAdded = 0;
        for (int i = 0; i < sink->matchesCount(); i++)
        {
                DBusError dbusError;
                dbus_error_init(&dbusError);
                dbus_bus_add_match(_connection, sink->match(i), &dbusError);
                if (dbus_error_is_set(&dbusError))
                        LOG_ERROR("Couldn't add match for sink %s: %s", sink->id(), dbusError.message);
                else
                        sinksAdded++;
        }
        LOG_DEBUG("Added %d of %d matches for sink %s.", sinksAdded, sink->matchesCount(), sink->id());
}


bool DBusEventWatcher::checkQueue(int timeoutSecs)
{
        // guard
        if (!_connection)
                return false;

        // check for messages
        bool pendingMessages = false;
        for (int i = 0; i < (timeoutSecs * 10); i++)
        {
                // give the connection some time to process messages
                dbus_connection_read_write(_connection, 100);

                // process all pending messages
                while (true)
                {
                        // get the next message from the queue
                        DBusMessage *message = dbus_connection_pop_message(_connection);
                        if (message)
                                pendingMessages = true;
                        else
                                break;

                        // notify the event sinks
                        for (EventSink *sink : _sinks)
                                sink->inspectMessage(message);

                        // clean up
                        dbus_message_unref(message);
                }

                // stop waiting if there are pending messages
                if (pendingMessages)
                        break;
        }
        return pendingMessages;
}
