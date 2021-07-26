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


class Device;
class ManagedDevice;
class GattService;
class BluezAdapter;



class DeviceManager
{
public:

        DeviceManager(BluezAdapter *bluezAdapter);
        ~DeviceManager();

        bool startScan();
        bool stopScan();
        void connectedDiscoveredManagedDevices();

        void clearManagedDevices();
        int managedDevicesCount() const { return _managedDevicesCount; }
        bool addManagedDevice(const char *address);
        ManagedDevice *managedDeviceByIndex(int index) const ;
        ManagedDevice *managedDeviceByAddress(const char *address) const;
        int indexOfManagedDevice(const char *address) const;
        bool isManagedDevice(const char *address) const { return (indexOfManagedDevice(address) >= 0); };
        bool allManagedDevicesConnected();

        void runService(GattService *service);


private:

        BluezAdapter *_bluezAdapter;
        int _managedDevicesCount;
        int _managedDevicesCapacity;
        ManagedDevice **_managedDevices;
        bool _wasScanning;
};

#endif // DEVICEMANAGER_H
