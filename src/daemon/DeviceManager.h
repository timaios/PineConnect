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



#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H


#include <mutex>


class Device;
class ManagedDevice;
class GattService;



class DeviceManager
{
public:

        DeviceManager();
        ~DeviceManager();

        bool scan();
        void connectedDiscoveredManagedDevices();

        void clearManagedDevices();
        int managedDevicesCount();
        bool addManagedDevice(const char *address);
        ManagedDevice *managedDeviceByIndex(int index);
        ManagedDevice *managedDeviceByAddress(const char *address);
        int indexOfManagedDevice(const char *address);
        bool isManagedDevice(const char *address) { return (indexOfManagedDevice(address) >= 0); };
        bool allManagedDevicesConnected();

        void runService(GattService *service);


private:

        std::mutex _discoveredDevicesMutex;
        int _discoveredDevicesCount;
        int _discoveredDevicesCapacity;
        Device **_discoveredDevices;
        std::mutex _managedDevicesMutex;
        int _managedDevicesCount;
        int _managedDevicesCapacity;
        ManagedDevice **_managedDevices;

        int unsafe_indexOfManagedDevice(const char *address) const;

        void clearDiscoveredDevicesList();
        void addDiscoveredDevice(const char *address, const char *name);

        static void deviceDiscoveredCallback(void *adapter, const char *address, const char *name, void *userData);
};

#endif // DEVICEMANAGER_H
