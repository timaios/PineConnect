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



#include "ManagedDevice.h"

#include <stdlib.h>
#include <string.h>

#include "Logger.h"
#include "gattlib.h"



ManagedDevice::ManagedDevice(const char *address)
        : Device(address, nullptr)
{
        _connection = nullptr;
        _discovered = false;
}


void ManagedDevice::setName(const char *value)
{
        if (_name)
        {
                delete[] _name;
                _name = nullptr;
        }
        if (value)
        {
                _name = new char[strlen(value) + 1];
                strcpy(_name, value);
        }
}


bool ManagedDevice::isConnected()
{
        std::lock_guard<std::mutex> guard(_deviceMutex);
        return (_connection);
}


bool ManagedDevice::connect()
{
        std::lock_guard<std::mutex> guard(_deviceMutex);

        if (_connection)
                LOG_WARNING("Overwriting an existing connection!");

        LOG_INFO("Connecting to device %s...", _address);
        _connection = gattlib_connect(nullptr, _address, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
        if (_connection)
        {
                // make sure we get notified if the device disconnects
                gattlib_register_on_disconnect(_connection, deviceDisconnectedCallback, reinterpret_cast<void *>(this));
                LOG_INFO("Connected to device %s.", _address);
                return true;
        }
        else
        {
                LOG_WARNING("Could not connect to device %s.", _address);
                return false;
        }
}


void ManagedDevice::disconnect()
{
        std::lock_guard<std::mutex> guard(_deviceMutex);

        if (_connection)
        {
                LOG_INFO("Disconnecting device %s...", _address);
                gattlib_disconnect(_connection);
                _connection = nullptr;
                _discovered = false;
                LOG_INFO("Disconnected device %s.", _address);
        }
}


bool ManagedDevice::readCharacteristic(const char *charGuid, uint8_t **buffer, size_t *bufferSize, bool disconnectOnFailure)
{
        std::lock_guard<std::mutex> guard(_deviceMutex);

        if (!_connection)
        {
                LOG_WARNING("Tried to read characteristic from a closed connection.");
                return false;
        }

        uuid_t ctsCharId;
        gattlib_string_to_uuid(charGuid, strlen(charGuid) + 1, &ctsCharId);
        int result = gattlib_read_char_by_uuid(_connection, &ctsCharId, reinterpret_cast<void **>(buffer), bufferSize);
        if (result != GATTLIB_SUCCESS)
        {
                if (result == GATTLIB_NOT_FOUND)
                {
                        LOG_WARNING("Could not find the GATT characteristic on device %s.", _address);
                        if (disconnectOnFailure)
                        {
                                // in case of GATTLIB_NOT_FOUND the connection can usually still be closed
                                gattlib_disconnect(_connection);
                                _connection = nullptr;
                                _discovered = false;
                                LOG_INFO("Device %s has disconnected.", _address);
                        }
                }
                else
                {
                        // in case of another error, the connection is already toast
                        LOG_ERROR("Error while reading the GATT characteristic on device %s.", _address);
                        _connection = nullptr;
                        _discovered = false;
                        LOG_INFO("Device %s has disconnected.", _address);
                }
                return false;
        }

        return true;
}


bool ManagedDevice::writeCharacteristic(const char *charGuid, uint8_t *buffer, size_t length, bool disconnectOnFailure)
{
        std::lock_guard<std::mutex> guard(_deviceMutex);

        if (!_connection)
        {
                LOG_WARNING("Tried to write characteristic to a closed connection.");
                return false;
        }

        uuid_t ctsCharId;
        gattlib_string_to_uuid(charGuid, strlen(charGuid) + 1, &ctsCharId);
        int result = gattlib_write_char_by_uuid(_connection, &ctsCharId, reinterpret_cast<const void *>(buffer), length);
        if (result != GATTLIB_SUCCESS)
        {
                if (result == GATTLIB_NOT_FOUND)
                {
                        LOG_WARNING("Could not find GATT characteristic on device %s.", _address);
                        if (disconnectOnFailure)
                        {
                                // in case of GATTLIB_NOT_FOUND the connection can usually still be closed
                                gattlib_disconnect(_connection);
                                _connection = nullptr;
                                _discovered = false;
                                LOG_INFO("Device %s has disconnected.", _address);
                        }
                }
                else
                {
                        // in case of another error, the connection is already toast
                        LOG_ERROR("Error while writing GATT characteristic on device %s.", _address);
                        _connection = nullptr;
                        _discovered = false;
                        LOG_INFO("Device %s has disconnected.", _address);
                }
                return false;
        }

        return true;
}


void ManagedDevice::handleDisconnect()
{
        std::lock_guard<std::mutex> guard(_deviceMutex);

        _connection = nullptr;
        _discovered = false;
        LOG_INFO("Device %s disconnected.", _address);
}


void ManagedDevice::deviceDisconnectedCallback(void *userData)
{
        // guard
        if (!userData)
        {
                LOG_WARNING("No userData was provided.");
                return;
        }

        // extract the ManagedDevice instance from the userData and forward the call
        ManagedDevice *device = reinterpret_cast<ManagedDevice *>(userData);
        device->handleDisconnect();
}
