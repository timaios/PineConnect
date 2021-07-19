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



#include "DeviceManager.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Logger.h"
#include "BluezAdapter.h"
#include "Device.h"
#include "ManagedDevice.h"
#include "GattService.h"



#define DEVICES_CAPACITY_INITIAL   64
#define DEVICES_CAPACITY_GROWTH    32
#define SCAN_TIMEOUT_SECS          4



DeviceManager::DeviceManager(BluezAdapter *bluezAdapter)
{
        _bluezAdapter = bluezAdapter;
        _managedDevicesCount = 0;
        _managedDevicesCapacity = DEVICES_CAPACITY_INITIAL;
        _managedDevices = new ManagedDevice*[DEVICES_CAPACITY_INITIAL];
}


DeviceManager::~DeviceManager()
{
        clearManagedDevices();
        delete[] _managedDevices;
}


bool DeviceManager::scan()
{
        // start scanning for BLE devices
        bool wasScanning = _bluezAdapter->isDiscovering();
        if (wasScanning)
                LOG_VERBOSE("Device scan is already ongoing.");
        else
        {
                if (_bluezAdapter->startDiscovery())
                        LOG_VERBOSE("Started device scan.");
                else
                {
                        LOG_ERROR("Could not start scanning for devices.");
                        return false;
                }
        }

        // give the adapter some time to discover new devices
        sleep(SCAN_TIMEOUT_SECS);

        // scanning's done
        if (!wasScanning)
        {
                if (_bluezAdapter->stopDiscovery())
                        LOG_VERBOSE("Stopped device scan.");
                else
                {
                        LOG_ERROR("Could not stop scanning for devices.");
                        return false;
                }
        }

        // show discovered devices
        for (int i = 0; i < _bluezAdapter->discoveredDevicesCount(); i++)
        {
                const BluezAdapter::DeviceInfo *device = _bluezAdapter->discoveredDeviceAt(i);
                if (device->name())
                        LOG_VERBOSE("Detected device %s (%s).", device->address(), device->name());
                else
                        LOG_VERBOSE("Detected device %s.", device->address());
        }

        // done
        return true;
}


void DeviceManager::connectedDiscoveredManagedDevices()
{
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                ManagedDevice *device = _managedDevices[i];
                bool discovered = false;
                for (int j = 0; j < _bluezAdapter->discoveredDevicesCount(); j++)
                {
                        if (strcasecmp(_bluezAdapter->discoveredDeviceAt(j)->address(), device->address()) == 0)
                        {
                                discovered = true;
                                break;
                        }
                }
                if (discovered)
                        device->connect();
        }
}


void DeviceManager::clearManagedDevices()
{
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                _managedDevices[i]->disconnect();
                delete _managedDevices[i];
                _managedDevices[i] = nullptr;
        }
        _managedDevicesCount = 0;
}


bool DeviceManager::addManagedDevice(const char *address)
{
        // make sure that device hasn't been added before
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (strcasecmp(_managedDevices[i]->address(), address) == 0)
                {
                        LOG_WARNING("The device %s has already been added.", address);
                        return false;
                }
        }

        // make sure there's enough capacity to store the device
        if (_managedDevicesCount == _managedDevicesCapacity)
        {
                _managedDevicesCapacity += DEVICES_CAPACITY_GROWTH;
                ManagedDevice **newList = new ManagedDevice*[_managedDevicesCapacity];
                for (int i = 0; i < _managedDevicesCount; i++)
                        newList[i] = _managedDevices[i];
                delete[] _managedDevices;
                _managedDevices = newList;
                LOG_DEBUG("Managed devices list capacity grew to %d.", _managedDevicesCapacity);
        }

        // add the device
        ManagedDevice *device = new ManagedDevice(_bluezAdapter, address);
        _managedDevices[_managedDevicesCount] = device;
        _managedDevicesCount++;
        LOG_INFO("Added managed device: %s", address);
        return true;
}


ManagedDevice *DeviceManager::managedDeviceByIndex(int index) const
{
        if ((index >= 0) && (index < _managedDevicesCount))
                return _managedDevices[index];
        return nullptr;
}


ManagedDevice *DeviceManager::managedDeviceByAddress(const char *address) const
{
        int index = indexOfManagedDevice(address);
        if (index >= 0)
                return _managedDevices[index];
        return nullptr;
}


int DeviceManager::indexOfManagedDevice(const char *address) const
{
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (strcasecmp(_managedDevices[i]->address(), address) == 0)
                        return i;
        }
        return -1;
}


bool DeviceManager::allManagedDevicesConnected()
{
        for (int i = 0; i < _managedDevicesCount; i++)
                if (!_managedDevices[i]->isConnected())
                        return false;
        return true;
}


void DeviceManager::runService(GattService *service)
{
        if (!service)
                return;

        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (_managedDevices[i]->isConnected())
                        service->run(_managedDevices[i]);
        }
}
