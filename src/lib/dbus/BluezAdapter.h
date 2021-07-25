/*
 *
 *  BluezAdapter - Communication with the Bluez daemon using the low level DBus API
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



#ifndef BLUEZADAPTER_H
#define BLUEZADAPTER_H


#include <stdlib.h>
#include <stdint.h>
#include <dbus/dbus.h>



class BluezAdapter
{
public:

        struct DeviceInfo
        {
        public:
                DeviceInfo();
                ~DeviceInfo();
                const char *address() const { return static_cast<const char *>(_address); }
                void setAddress(const char *value);
                const char *name() const { return static_cast<const char *>(_name); }
                void setName(const char *value);
        private:
                char *_address;
                char *_name;
        };


        BluezAdapter(const char *hci);
        ~BluezAdapter();

        bool dbusConnected() const { return (_connection); }

        int timeout() const { return _timeout; }
        void setTimeout(int value) { _timeout = value; }

        bool powered();

        bool isDiscovering();
        bool startDiscovery();
        bool stopDiscovery();
        int discoveredDevicesCount() const { return _discoveredDevicesCount; }
        const DeviceInfo *discoveredDeviceAt(int index) const;

        bool isDeviceConnected(const char *address);
        bool connectDevice(const char *address, bool verify = true);
        bool disconnectDevice(const char *address, bool verify = true);
        bool removeDevice(const char *address);

        char *findCharacteristicPath(const char *deviceAddress, const char *charUUID);
        int readCharacteristic(const char *charPath, uint8_t *buffer, int bufferSize);
        bool writeCharacteristic(const char *charPath, uint8_t *buffer, int length);

protected:

        DBusConnection *_connection;
        char *_hci;
        int _timeout;
        int _discoveredDevicesCount;
        int _discoveredDevicesCapacity;
        DeviceInfo **_discoveredDevices;

        bool isValidAddress(const char *address) const;
        void getDevicePath(const char *address, char *path);

        const char *getStringFromVariant(DBusMessageIter *variantIter);
        bool getBooleanFromVariant(DBusMessageIter *variantIter);

        bool callMethod(const char *path, const char *interface, const char *method);
        bool readBooleanProperty(const char *path, const char *interface, const char *propName, bool logErrors = true);

private:

        void clearDiscoveredDevicesList();
        void updateDiscoveredDevicesList();
        void updateDiscoveredDevicesList_object(DBusMessageIter *objIter);
        void updateDiscoveredDevicesList_device(DBusMessageIter *deviceIter);
        bool canReadConnectedProperty(const char *address);

        const char *findCharacteristicPath_object(DBusMessageIter *objIter, const char *devicePath, const char *charUUID);
        bool findCharacteristicPath_device(DBusMessageIter *deviceIter, const char *charUUID);

        void addReadWriteOptions(DBusMessage *query);
};

#endif // BLUEZADAPTER_H
