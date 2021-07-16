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

#include "Logger.h"
#include "gattlib.h"
#include "Device.h"
#include "ManagedDevice.h"
#include "GattService.h"



#define DEVICES_CAPACITY_INITIAL   64
#define DEVICES_CAPACITY_GROWTH    32
#define SCAN_TIMEOUT_SECS          4



DeviceManager::DeviceManager()
{
        _discoveredDevicesCount = 0;
        _discoveredDevicesCapacity = DEVICES_CAPACITY_INITIAL;
        _discoveredDevices = new Device*[DEVICES_CAPACITY_INITIAL];
        _managedDevicesCount = 0;
        _managedDevicesCapacity = DEVICES_CAPACITY_INITIAL;
        _managedDevices = new ManagedDevice*[DEVICES_CAPACITY_INITIAL];
}


DeviceManager::~DeviceManager()
{
        clearDiscoveredDevicesList();
        delete[] _discoveredDevices;
        clearManagedDevices();
        delete[] _managedDevices;
}


bool DeviceManager::scan()
{
        // start with an empty list
        clearDiscoveredDevicesList();

        // initialize the managed devices's discovered flag
        _managedDevicesMutex.lock();
        for (int i = 0; i < _managedDevicesCount; i++)
                _managedDevices[i]->setDiscovered(false);
        _managedDevicesMutex.unlock();

        // open the default bluetooth adapter
        LOG_VERBOSE("Starting device scan...");
        void *adapter = nullptr;
        if (gattlib_adapter_open(nullptr, &adapter) != 0)
        {
                LOG_ERROR("Could not open default bluetooth adapter.");
                return false;
        }

        // start scanning for BLE devices
        if (gattlib_adapter_scan_enable(adapter, deviceDiscoveredCallback, SCAN_TIMEOUT_SECS, reinterpret_cast<void *>(this)) != GATTLIB_SUCCESS)
        {
                LOG_ERROR("Could not start scanning for BLE devices.");
                gattlib_adapter_close(adapter);
                return false;
        }

        // scanning's done
        gattlib_adapter_scan_disable(adapter);
        gattlib_adapter_close(adapter);
        LOG_VERBOSE("Device scan completed.");

        // done
        return true;
}


void DeviceManager::connectedDiscoveredManagedDevices()
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);

        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (_managedDevices[i]->discovered())
                        _managedDevices[i]->connect();
        }
}


void DeviceManager::clearManagedDevices()
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
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
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);

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
        ManagedDevice *device = new ManagedDevice(address);
        _managedDevices[_managedDevicesCount] = device;
        _managedDevicesCount++;
        LOG_INFO("Added managed device: %s", address);
        return true;
}


ManagedDevice *DeviceManager::managedDeviceByIndex(int index)
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
        if ((index >= 0) && (index < _managedDevicesCount))
                return _managedDevices[index];
        return nullptr;
}


ManagedDevice *DeviceManager::managedDeviceByAddress(const char *address)
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
        int index = unsafe_indexOfManagedDevice(address);
        if (index >= 0)
                return _managedDevices[index];
        return nullptr;
}


int DeviceManager::indexOfManagedDevice(const char *address)
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
        return unsafe_indexOfManagedDevice(address);
}


bool DeviceManager::allManagedDevicesConnected()
{
        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
        for (int i = 0; i < _managedDevicesCount; i++)
                if (!_managedDevices[i]->isConnected())
                        return false;
        return true;
}


void DeviceManager::runService(GattService *service)
{
        if (!service)
                return;

        std::lock_guard<std::mutex> guard(_managedDevicesMutex);
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (_managedDevices[i]->isConnected())
                        service->run(_managedDevices[i]);
        }
}


int DeviceManager::unsafe_indexOfManagedDevice(const char *address) const
{
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (strcasecmp(_managedDevices[i]->address(), address) == 0)
                        return i;
        }
        return -1;
}


void DeviceManager::clearDiscoveredDevicesList()
{
        std::lock_guard<std::mutex> guard(_discoveredDevicesMutex);

        for (int i = 0; i < _discoveredDevicesCount; i++)
        {
                delete _discoveredDevices[i];
                _discoveredDevices[i] = nullptr;
        }
        _discoveredDevicesCount = 0;
}


void DeviceManager::addDiscoveredDevice(const char *address, const char *name)
{
        std::lock_guard<std::mutex> managedDevicesGuard(_managedDevicesMutex);

        // check if it's a managed device
        int managedIndex = unsafe_indexOfManagedDevice(address);
        if (managedIndex >= 0)
        {
                // update its name and discovery status
                _managedDevices[managedIndex]->setName(name);
                _managedDevices[managedIndex]->setDiscovered(true);

                // log that event
                if (name)
                        LOG_VERBOSE("Discovered managed device %s (%s).", address, name);
                else
                        LOG_INFO("Discovered managed device %s.", address);
        }
        else
        {
                // log that event
                if (name)
                        LOG_VERBOSE("Discovered unmanaged device %s (%s).", address, name);
                else
                        LOG_VERBOSE("Discovered unmanaged device %s.", address);
        }

        std::lock_guard<std::mutex> discoveredDevicesGuard(_discoveredDevicesMutex);

        // make sure there's enough capacity to store the device
        if (_discoveredDevicesCount == _discoveredDevicesCapacity)
        {
                _discoveredDevicesCapacity += DEVICES_CAPACITY_GROWTH;
                Device **newList = new Device*[_discoveredDevicesCapacity];
                for (int i = 0; i < _discoveredDevicesCount; i++)
                        newList[i] = _discoveredDevices[i];
                delete[] _discoveredDevices;
                _discoveredDevices = newList;
                LOG_DEBUG("Discovered devices list capacity grew to %d.", _discoveredDevicesCapacity);
        }

        // add the device
        Device *device = new Device(address, name);
        _discoveredDevices[_discoveredDevicesCount] = device;
        _discoveredDevicesCount++;
}


void DeviceManager::deviceDiscoveredCallback(void *adapter, const char *address, const char *name, void *userData)
{
        (void)adapter;

        // guards
        if (!address)
        {
                LOG_WARNING("No address was provided.");
                return;
        }
        if (!userData)
        {
                LOG_WARNING("No userData was provided.");
                return;
        }

        // extract the DeviceManager instance from the userData and forward the call
        DeviceManager *mgr = reinterpret_cast<DeviceManager *>(userData);
        mgr->addDiscoveredDevice(address, name);
}
