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
#include "BluezAdapter.h"



#define CONNECT_TIMEOUT      4000
#define DISCONNECT_TIMEOUT   5000



ManagedDevice::ManagedDevice(BluezAdapter *bluezAdapter, const char *address)
        : Device(address, nullptr)
{
        _bluezAdapter = bluezAdapter;
}


ManagedDevice::~ManagedDevice()
{
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
                size_t len = strlen(value) + 1;
                _name = new char[len];
                strncpy(_name, value, len);
        }
}


bool ManagedDevice::isConnected()
{
        return _bluezAdapter->isDeviceConnected(_address);
}


bool ManagedDevice::connect()
{
        LOG_INFO("Connecting to device %s...", _address);
        int prevTimeout = _bluezAdapter->timeout();
        _bluezAdapter->setTimeout(CONNECT_TIMEOUT);
        bool result = _bluezAdapter->connectDevice(_address, true);
        if (result)
                LOG_INFO("Connected to device %s.", _address);
        else
                LOG_WARNING("Could not connect to device %s.", _address);
        _bluezAdapter->setTimeout(prevTimeout);
        return result;
}


bool ManagedDevice::disconnect()
{
        LOG_INFO("Disconnecting device %s...", _address);
        int prevTimeout = _bluezAdapter->timeout();
        _bluezAdapter->setTimeout(DISCONNECT_TIMEOUT);
        bool result = _bluezAdapter->disconnectDevice(_address);
        if (result)
                LOG_INFO("Disconnected device %s.", _address);
        else
                LOG_WARNING("Could not disconnected device %s.", _address);
        _bluezAdapter->setTimeout(prevTimeout);
        return result;
}


int ManagedDevice::readCharacteristic(const char *charGuid, uint8_t *buffer, int bufferSize)
{
        // get the characteristic's path
        char *charPath = _bluezAdapter->findCharacteristicPath(_address, charGuid);
        if (!charPath)
        {
                LOG_WARNING("Could not find GATT characteristic %s on device %s.", charGuid, _address);
                return -1;
        }

        // read the data
        int readBytes = _bluezAdapter->readCharacteristic(charPath, buffer, bufferSize);
        delete[] charPath;

        // check result
        if (readBytes < 0)
                LOG_ERROR("Error while reading from GATT characteristic %s on device %s.", charGuid, _address);
        else
                LOG_DEBUG("Read %d bytes from GATT characteristic %s on device %s.", readBytes, charGuid, _address);
        return readBytes;
}


bool ManagedDevice::writeCharacteristic(const char *charGuid, uint8_t *buffer, int length)
{
        // get the characteristic's path
        char *charPath = _bluezAdapter->findCharacteristicPath(_address, charGuid);
        if (!charPath)
        {
                LOG_WARNING("Could not find GATT characteristic %s on device %s.", charGuid, _address);
                return false;
        }

        // write the data
        bool result = _bluezAdapter->writeCharacteristic(charPath, buffer, length);
        delete[] charPath;

        // check result
        if (!result)
                LOG_ERROR("Error while writing to GATT characteristic %s on device %s.", charGuid, _address);
        else
                LOG_DEBUG("Wrote %d bytes to GATT characteristic %s on device %s.", length, charGuid, _address);
        return result;
}
