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



#include "BluezAdapter.h"

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "Logger.h"



#define DEVICE_ADDRESS_LENGTH          17
#define DEVICE_LIST_CAPACITY_INITIAL   64
#define DEVICE_LIST_CAPACITY_GROWTH    32

#define MAX_PATH_LENGTH                256

#define DEFAULT_TIMEOUT                4000



BluezAdapter::DeviceInfo::DeviceInfo()
{
        _address = nullptr;
        _name = nullptr;
}


BluezAdapter::DeviceInfo::~DeviceInfo()
{
        if (_address)
                delete[] _address;
        if (_name)
                delete[] _name;
}


void BluezAdapter::DeviceInfo::setAddress(const char *value)
{
        if (_address)
                delete[] _address;
        if (value)
        {
                size_t valueLength = strlen(value) + 1;
                _address = new char[valueLength];
                strncpy(_address, value, valueLength);
        }
        else
                _address = nullptr;
}


void BluezAdapter::DeviceInfo::setName(const char *value)
{
        if (_name)
                delete[] _name;
        if (value)
        {
                size_t valueLength = strlen(value) + 1;
                _name = new char[valueLength];
                strncpy(_name, value, valueLength);
        }
        else
                _name = nullptr;
}



BluezAdapter::BluezAdapter(const char *hci)
{
        // initialize
        _timeout = DEFAULT_TIMEOUT;
        _discoveredDevicesCapacity = DEVICE_LIST_CAPACITY_INITIAL;
        _discoveredDevicesCount = 0;
        _discoveredDevices = new DeviceInfo*[_discoveredDevicesCapacity];

        // copy adapter name
        if (hci)
        {
                size_t len = strlen(hci) + 1;
                _hci = new char[len];
                strncpy(_hci, hci, len);
        }
        else
        {
                _hci = new char[5];
                strncpy(_hci, "hci0", 5);
        }

        // open DBus connection
        DBusError dbusError;
        dbus_error_init(&dbusError);
        _connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbusError);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Could not get a connection to system DBus: %s", dbusError.message);
                dbus_error_free(&dbusError);
                _connection = nullptr;
                return;
        }
        if (!_connection)
        {
                LOG_ERROR("Could not get a connection to system DBus.");
                return;
        }
        LOG_DEBUG("Established DBus connection. Unique name: %s", dbus_bus_get_unique_name(_connection));
}


BluezAdapter::~BluezAdapter()
{
        if (_hci)
                delete[] _hci;
        clearDiscoveredDevicesList();
        delete[] _discoveredDevices;
}


bool BluezAdapter::powered()
{
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s", _hci);
        return readBooleanProperty(path, "org.bluez.Adapter1", "Powered");
}


bool BluezAdapter::isDiscovering()
{
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s", _hci);
        return readBooleanProperty(path, "org.bluez.Adapter1", "Discovering");
}


bool BluezAdapter::startDiscovery()
{
        // call the adapter's StartDiscovery method
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s", _hci);
        return callMethod(path, "org.bluez.Adapter1", "StartDiscovery");
}


bool BluezAdapter::stopDiscovery()
{
        // call the adapter's StopDiscovery method
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s", _hci);
        if (!callMethod(path, "org.bluez.Adapter1", "StopDiscovery"))
                return false;

        // update the list of discovered devices
        updateDiscoveredDevicesList();
        return true;
}


const BluezAdapter::DeviceInfo *BluezAdapter::discoveredDeviceAt(int index) const
{
        if ((index >= 0) && (index < _discoveredDevicesCount))
                return static_cast<const DeviceInfo*>(_discoveredDevices[index]);
        else
                return nullptr;
}


bool BluezAdapter::isDeviceConnected(const char *address)
{
        // guard
        if (!isValidAddress(address))
        {
                LOG_DEBUG("Invalid device address.");
                return false;
        }

        // read the device's Connect property
        char path[MAX_PATH_LENGTH];
        getDevicePath(address, path);
        return readBooleanProperty(path, "org.bluez.Device1", "Connected", false);
}


bool BluezAdapter::connectDevice(const char *address, bool verify)
{
        // guard
        if (!isValidAddress(address))
        {
                LOG_DEBUG("Invalid device address.");
                return false;
        }

        // call the device's Connect method
        char path[MAX_PATH_LENGTH];
        getDevicePath(address, path);
        if (!callMethod(path, "org.bluez.Device1", "Connect"))
                return false;

        // verify that the Connected property is true
        if (verify)
                return isDeviceConnected(address);
        else
                return true;
}


bool BluezAdapter::disconnectDevice(const char *address, bool verify)
{
        // guard
        if (!isValidAddress(address))
        {
                LOG_DEBUG("Invalid device address.");
                return false;
        }

        // call the device's Disconnect method
        char path[MAX_PATH_LENGTH];
        getDevicePath(address, path);
        if (!callMethod(path, "org.bluez.Device1", "Disconnect"))
                return false;

        // verify that the Connected property is true
        if (verify)
                return !isDeviceConnected(address);
        else
                return true;
}


bool BluezAdapter::removeDevice(const char *address)
{
        // guards
        if (!_connection)
                return false;
        if (!isValidAddress(address))
        {
                LOG_DEBUG("Invalid device address.");
                return false;
        }

        char devicePath[MAX_PATH_LENGTH];
        getDevicePath(address, devicePath);

        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s", _hci);
        DBusMessage *query = dbus_message_new_method_call("org.bluez", path, "org.bluez.Adapter1", "RemoveDevice");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return false;
        }

        const char *constDevicePath = static_cast<const char *>(devicePath);
        dbus_message_append_args(query, DBUS_TYPE_OBJECT_PATH, &constDevicePath, DBUS_TYPE_INVALID);

        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return false;
        }
        dbus_message_unref(reply);
        return true;
}


char *BluezAdapter::findCharacteristicPath(const char *deviceAddress, const char *charUUID)
{
        // guards
        if (!_connection)
                return nullptr;
        if (!isValidAddress(deviceAddress))
        {
                LOG_DEBUG("Invalid device address.");
                return nullptr;
        }

        // prepare the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return nullptr;
        }

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return nullptr;
        }

        // get the device's path
        char devicePath[MAX_PATH_LENGTH];
        getDevicePath(deviceAddress, devicePath);

        // go through the returned data
        DBusMessageIter objArrayIter;
        dbus_message_iter_init(reply, &objArrayIter);
        if (dbus_message_iter_get_arg_type(&objArrayIter) == DBUS_TYPE_ARRAY)
        {
                DBusMessageIter objIter;
                dbus_message_iter_recurse(&objArrayIter, &objIter);
                while (true)
                {
                        const char *result = findCharacteristicPath_object(&objIter, devicePath, charUUID);
                        if (result)
                        {
                                // copy the result to a new buffer
                                size_t resultLength = strlen(result) + 1;
                                char *resultBuffer = new char[resultLength];
                                strncpy(resultBuffer, result, resultLength);

                                // free the reply's ressources and return the new buffer
                                dbus_message_unref(reply);
                                return resultBuffer;
                        }
                        if (!dbus_message_iter_next(&objIter))
                                break;
                }
        }
        dbus_message_unref(reply);
        return nullptr;
}


int BluezAdapter::readCharacteristic(const char *charPath, uint8_t *buffer, int bufferSize)
{
        // guards
        if (!_connection)
                return -1;
        if (!buffer || (bufferSize < 0))
                return -1;

        // prepare the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", charPath, "org.bluez.GattCharacteristic1", "ReadValue");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return -1;
        }

        // we need an initial iterator for the query's parameters
        DBusMessageIter paramsIter;
        dbus_message_iter_init_append(query, &paramsIter);

        // add an empty String->Variant dictionary container (options parameter)
        DBusMessageIter dictIter;
        dbus_message_iter_open_container(&paramsIter, DBUS_TYPE_ARRAY, "{sv}", &dictIter);
        dbus_message_iter_close_container(&paramsIter, &dictIter);

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return -1;
        }

        // copy the returned byte array to the buffer
        int copiedBytes = 0;
        DBusMessageIter byteArrayIter;
        dbus_message_iter_init(reply, &byteArrayIter);
        if (dbus_message_iter_get_arg_type(&byteArrayIter) == DBUS_TYPE_ARRAY)
        {
                DBusMessageIter byteIter;
                dbus_message_iter_recurse(&byteArrayIter, &byteIter);
                while (true)
                {
                        if (dbus_message_iter_get_arg_type(&byteIter) == DBUS_TYPE_BYTE)
                        {
                                dbus_uint32_t value;
                                dbus_message_iter_get_basic(&byteIter, &value);
                                if (copiedBytes < bufferSize)
                                {
                                        buffer[copiedBytes] = static_cast<uint8_t>(value);
                                        copiedBytes++;
                                }
                                else
                                {
                                        LOG_WARNING("Buffer too small to contain all returned bytes.");
                                        break;
                                }
                        }
                        if (!dbus_message_iter_next(&byteIter))
                                break;
                }
        }

        // done
        dbus_message_unref(reply);
        LOG_DEBUG("Read %d bytes from characteristic %s.", copiedBytes, charPath);
        return copiedBytes;
}


bool BluezAdapter::writeCharacteristic(const char *charPath, uint8_t *buffer, int length)
{
        // guards
        if (!_connection)
                return false;
        if (!buffer || (length < 0))
                return false;

        // prepare the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", charPath, "org.bluez.GattCharacteristic1", "WriteValue");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return false;
        }

        // we need an initial iterator for the query's parameters
        DBusMessageIter paramsIter;
        dbus_message_iter_init_append(query, &paramsIter);

        // copy the buffer bytes into a byte array container parameter
        DBusMessageIter arrayIter;
        dbus_message_iter_open_container(&paramsIter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &arrayIter);
        for (int i = 0; i < length; i++)
        {
                dbus_uint32_t value = static_cast<dbus_uint32_t>(buffer[i]);
                dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_BYTE, &value);
        }
        dbus_message_iter_close_container(&paramsIter, &arrayIter);

        // add an empty String->Variant dictionary container (options parameter)
        DBusMessageIter dictIter;
        dbus_message_iter_open_container(&paramsIter, DBUS_TYPE_ARRAY, "{sv}", &dictIter);
        dbus_message_iter_close_container(&paramsIter, &dictIter);

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return false;
        }

        // done
        dbus_message_unref(reply);
        LOG_DEBUG("Wrote %d bytes to characteristic %s.", length, charPath);
        return true;
}


bool BluezAdapter::isValidAddress(const char *address) const
{
        // it shouldn't be NULL
        if (!address)
                return false;

        // the length must match
        int length = static_cast<int>(strlen(address));
        if (length != DEVICE_ADDRESS_LENGTH)
                return false;

        // check the significant (hex) characters
        for (int i = 0; i < length; i++)
        {
                // don't care about the separator char
                if ((i % 3) == 2)
                        continue;

                // the current char should be a hex char
                char ch = address[i];
                if ((ch >= '0') && (ch <= '9'))
                        continue;
                if (((ch >= 'a') && (ch <= 'f')) || ((ch >= 'A') && (ch <= 'F')))
                        continue;

                // if we're here, the address is invalid
                return false;
        }

        // if we're here, the address is valid
        return true;
}


void BluezAdapter::getDevicePath(const char *address, char *path)
{
        snprintf(path, MAX_PATH_LENGTH, "/org/bluez/%s/dev_", _hci);
        const char *src = address;
        char *dst = path + strlen(path);
        while (*src)
        {
                if (((*src >= '0') && (*src <= '9')) || ((*src >= 'A') && (*src <= 'F')))
                        *dst = *src;
                else if ((*src >= 'a') && (*src <= 'f'))
                        *dst = *src - 32;
                else
                        *dst = '_';
                src++;
                dst++;
        }
        *dst = '\0';
}


const char *BluezAdapter::getStringFromVariant(DBusMessageIter *variantIter)
{
        // make sure that it's really a variant
        if (dbus_message_iter_get_arg_type(variantIter) != DBUS_TYPE_VARIANT)
                return nullptr;

        // get the value
        DBusMessageIter valueIter;
        dbus_message_iter_recurse(variantIter, &valueIter);
        if (dbus_message_iter_get_arg_type(&valueIter) != DBUS_TYPE_STRING)
                return nullptr;
        const char *value = nullptr;
        dbus_message_iter_get_basic(&valueIter, &value);
        return value;
}


bool BluezAdapter::getBooleanFromVariant(DBusMessageIter *variantIter)
{
        // make sure that it's really a variant
        if (dbus_message_iter_get_arg_type(variantIter) != DBUS_TYPE_VARIANT)
                return false;

        // get the value
        DBusMessageIter valueIter;
        dbus_message_iter_recurse(variantIter, &valueIter);
        if (dbus_message_iter_get_arg_type(&valueIter) != DBUS_TYPE_BOOLEAN)
                return false;
        dbus_bool_t value = FALSE;
        dbus_message_iter_get_basic(&valueIter, &value);
        return (value == TRUE);
}


bool BluezAdapter::callMethod(const char *path, const char *interface, const char *method)
{
        // guard
        if (!_connection)
                return false;

        // create the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", path, interface, method);
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return false;
        }

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return false;
        }
        dbus_message_unref(reply);
        return true;
}


bool BluezAdapter::readBooleanProperty(const char *path, const char *interface, const char *propName, bool logErrors)
{
        // guard
        if (!_connection)
                return false;

        // create the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", path, "org.freedesktop.DBus.Properties", "Get");
        if (!query)
        {
                if (logErrors)
                        LOG_ERROR("Couldn't allocate memory for the query message.");
                return false;
        }
        dbus_message_append_args(query, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &propName, DBUS_TYPE_INVALID);

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                if (logErrors)
                        LOG_ERROR("Couldn't read property: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return false;
        }

        // extract the property's value from the Variant container
        DBusMessageIter varIter;
        dbus_message_iter_init(reply, &varIter);
        bool result = getBooleanFromVariant(&varIter);
        dbus_message_unref(reply);
        return result;
}


void BluezAdapter::clearDiscoveredDevicesList()
{
        for (int i = 0; i < _discoveredDevicesCount; i++)
        {
                delete _discoveredDevices[i];
                _discoveredDevices[i] = nullptr;
        }
        _discoveredDevicesCount = 0;
}


void BluezAdapter::updateDiscoveredDevicesList()
{
        // start with an empty list
        clearDiscoveredDevicesList();

        // guard
        if (!_connection)
                return;

        // prepare the query message
        DBusMessage *query = dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return;
        }

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                LOG_ERROR("Couldn't execute command: %s", dbusError.message);
                dbus_error_free(&dbusError);
                return;
        }

        // go through the returned data
        DBusMessageIter objArrayIter;
        dbus_message_iter_init(reply, &objArrayIter);
        if (dbus_message_iter_get_arg_type(&objArrayIter) == DBUS_TYPE_ARRAY)
        {
                DBusMessageIter objIter;
                dbus_message_iter_recurse(&objArrayIter, &objIter);
                while (true)
                {
                        updateDiscoveredDevicesList_object(&objIter);
                        if (!dbus_message_iter_next(&objIter))
                                break;
                }
        }
        dbus_message_unref(reply);
}


void BluezAdapter::updateDiscoveredDevicesList_object(DBusMessageIter *objIter)
{
        // the current object should be a ObjectPath->Array dictionary
        if (dbus_message_iter_get_arg_type(objIter) != DBUS_TYPE_DICT_ENTRY)
        {
                LOG_WARNING("Expected a dictionary, but it wasn't.");
                return;
        }

        // get the object path
        DBusMessageIter dictElementsIter;
        dbus_message_iter_recurse(objIter, &dictElementsIter);
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_OBJECT_PATH)
        {
                LOG_WARNING("Expected ObjectPath as first dictionary element, but it wasn't.");
                return;
        }
        const char *objPath = nullptr;
        dbus_message_iter_get_basic(&dictElementsIter, &objPath);

        // get the object's array
        if (!dbus_message_iter_next(&dictElementsIter))
        {
                LOG_WARNING("Object dictionary has no second element.");
                return;
        }
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_ARRAY)
        {
                LOG_WARNING("Expected Array as second dictionary element, but it wasn't.");
                return;
        }

        // go through the array of devices
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&dictElementsIter, &arrayIter);
        while (true)
        {
                updateDiscoveredDevicesList_device(&arrayIter);
                if (!dbus_message_iter_next(&arrayIter))
                        break;
        }
}


void BluezAdapter::updateDiscoveredDevicesList_device(DBusMessageIter *deviceIter)
{
        // the current element should be a String->Array dictionary
        if (dbus_message_iter_get_arg_type(deviceIter) != DBUS_TYPE_DICT_ENTRY)
        {
                LOG_WARNING("Expected a dictionary, but it wasn't.");
                return;
        }

        // get the device's interface name
        DBusMessageIter dictElementsIter;
        dbus_message_iter_recurse(deviceIter, &dictElementsIter);
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_STRING)
        {
                LOG_WARNING("Expected String as first dictionary element, but it wasn't.");
                return;
        }
        const char *deviceInterfaceName = nullptr;
        dbus_message_iter_get_basic(&dictElementsIter, &deviceInterfaceName);

        // we're only interested in "real" devices
        if (strcasecmp(deviceInterfaceName, "org.bluez.Device1") != 0)
                return;

        // get the device's attribute array
        if (!dbus_message_iter_next(&dictElementsIter))
        {
                LOG_WARNING("Device dictionary has no second element.");
                return;
        }
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_ARRAY)
        {
                LOG_WARNING("Expected Array as second dictionary element, but it wasn't.");
                return;
        }

        // create an info object for the discovered device
        DeviceInfo *device = new DeviceInfo();

        // go through the attributes array
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&dictElementsIter, &arrayIter);
        while (true)
        {
                // the current array element should be a String->Variant dictionary
                if (dbus_message_iter_get_arg_type(&arrayIter) != DBUS_TYPE_DICT_ENTRY)
                {
                        LOG_WARNING("Expected a dictionary for the attributes array element, but it wasn't.");
                        break;
                }

                // get the attribute's name
                DBusMessageIter attributeDictIter;
                dbus_message_iter_recurse(&arrayIter, &attributeDictIter);
                if (dbus_message_iter_get_arg_type(&attributeDictIter) != DBUS_TYPE_STRING)
                {
                        LOG_WARNING("Expected String as first attribute dictionary element, but it wasn't.");
                        break;
                }
                const char *attributeName = nullptr;
                dbus_message_iter_get_basic(&attributeDictIter, &attributeName);

                // make sure there's an attribute value
                if (!dbus_message_iter_next(&attributeDictIter))
                {
                        LOG_WARNING("Attribute dictionary has no second element.");
                        break;
                }

                // copy the interesting attributes
                if (strcasecmp(attributeName, "Address") == 0)
                        device->setAddress(getStringFromVariant(&attributeDictIter));
                else if (strcasecmp(attributeName, "Name") == 0)
                        device->setName(getStringFromVariant(&attributeDictIter));

                // get the next array element
                if (!dbus_message_iter_next(&arrayIter))
                        break;
        }

        // consider adding the device to the list of discovered devices
        if (isValidAddress(device->address()) && canReadConnectedProperty(device->address()))
        {
                // grow the list if necessary
                if (_discoveredDevicesCount == _discoveredDevicesCapacity)
                {
                        _discoveredDevicesCapacity += DEVICE_LIST_CAPACITY_GROWTH;
                        DeviceInfo **newList = new DeviceInfo*[_discoveredDevicesCapacity];
                        for (int i = 0; i < _discoveredDevicesCount; i++)
                                newList[i] = _discoveredDevices[i];
                        delete[] _discoveredDevices;
                        _discoveredDevices = newList;
                }

                // add the device
                _discoveredDevices[_discoveredDevicesCount] = device;
                _discoveredDevicesCount++;
                LOG_DEBUG("Found registered device: %s", device->address());
        }
        else
                delete device;
}


bool BluezAdapter::canReadConnectedProperty(const char *address)
{
        // guards
        if (!_connection)
                return false;
        if (!isValidAddress(address))
        {
                LOG_DEBUG("Invalid device address.");
                return false;
        }

        // create the query message
        char path[MAX_PATH_LENGTH];
        getDevicePath(address, path);
        const char *interface = "org.bluez.Device1";
        const char *propName = "Connected";
        DBusMessage *query = dbus_message_new_method_call("org.bluez", path, "org.freedesktop.DBus.Properties", "Get");
        if (!query)
        {
                LOG_ERROR("Couldn't allocate memory for the query message.");
                return false;
        }
        dbus_message_append_args(query, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &propName, DBUS_TYPE_INVALID);

        // send the query and wait for the reply
        DBusError dbusError;
        dbus_error_init(&dbusError);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(_connection, query, _timeout, &dbusError);
        dbus_message_unref(query);
        if (dbus_error_is_set(&dbusError))
        {
                dbus_error_free(&dbusError);
                return false;
        }
        dbus_message_unref(reply);
        return true;
}


const char *BluezAdapter::findCharacteristicPath_object(DBusMessageIter *objIter, const char *devicePath, const char *charUUID)
{
        // the current object should be a ObjectPath->Array dictionary
        if (dbus_message_iter_get_arg_type(objIter) != DBUS_TYPE_DICT_ENTRY)
        {
                LOG_WARNING("Expected a dictionary, but it wasn't.");
                return nullptr;
        }

        // get the object path
        DBusMessageIter dictElementsIter;
        dbus_message_iter_recurse(objIter, &dictElementsIter);
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_OBJECT_PATH)
        {
                LOG_WARNING("Expected ObjectPath as first dictionary element, but it wasn't.");
                return nullptr;
        }
        const char *objPath = nullptr;
        dbus_message_iter_get_basic(&dictElementsIter, &objPath);

        // the left part of the discovered path needs to match the device's path
        if (strncasecmp(objPath, devicePath, strlen(devicePath)) != 0)
                return nullptr;

        // get the object's array
        if (!dbus_message_iter_next(&dictElementsIter))
        {
                LOG_WARNING("Object dictionary has no second element.");
                return nullptr;
        }
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_ARRAY)
        {
                LOG_WARNING("Expected Array as second dictionary element, but it wasn't.");
                return nullptr;
        }

        // go through the array of devices
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&dictElementsIter, &arrayIter);
        while (true)
        {
                if (findCharacteristicPath_device(&arrayIter, charUUID))
                        return objPath;
                if (!dbus_message_iter_next(&arrayIter))
                        break;
        }

        // no match in this object
        return nullptr;
}


bool BluezAdapter::findCharacteristicPath_device(DBusMessageIter *deviceIter, const char *charUUID)
{
        // the current element should be a String->Array dictionary
        if (dbus_message_iter_get_arg_type(deviceIter) != DBUS_TYPE_DICT_ENTRY)
        {
                LOG_WARNING("Expected a dictionary, but it wasn't.");
                return false;
        }

        // get the interface name
        DBusMessageIter dictElementsIter;
        dbus_message_iter_recurse(deviceIter, &dictElementsIter);
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_STRING)
        {
                LOG_WARNING("Expected String as first dictionary element, but it wasn't.");
                return false;
        }
        const char *interfaceName = nullptr;
        dbus_message_iter_get_basic(&dictElementsIter, &interfaceName);

        // we're only interested in GATT characteristics
        if (strcasecmp(interfaceName, "org.bluez.GattCharacteristic1") != 0)
                return false;

        // get the characteristic's attribute array
        if (!dbus_message_iter_next(&dictElementsIter))
        {
                LOG_WARNING("Device dictionary has no second element.");
                return false;
        }
        if (dbus_message_iter_get_arg_type(&dictElementsIter) != DBUS_TYPE_ARRAY)
        {
                LOG_WARNING("Expected Array as second dictionary element, but it wasn't.");
                return false;
        }

        // go through the attributes array
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&dictElementsIter, &arrayIter);
        while (true)
        {
                // the current array element should be a String->Variant dictionary
                if (dbus_message_iter_get_arg_type(&arrayIter) != DBUS_TYPE_DICT_ENTRY)
                {
                        LOG_WARNING("Expected a dictionary for the attributes array element, but it wasn't.");
                        break;
                }

                // get the attribute's name
                DBusMessageIter attributeDictIter;
                dbus_message_iter_recurse(&arrayIter, &attributeDictIter);
                if (dbus_message_iter_get_arg_type(&attributeDictIter) != DBUS_TYPE_STRING)
                {
                        LOG_WARNING("Expected String as first attribute dictionary element, but it wasn't.");
                        break;
                }
                const char *attributeName = nullptr;
                dbus_message_iter_get_basic(&attributeDictIter, &attributeName);

                // make sure there's an attribute value
                if (!dbus_message_iter_next(&attributeDictIter))
                {
                        LOG_WARNING("Attribute dictionary has no second element.");
                        break;
                }

                // check if it's the correct UUID
                if (strcasecmp(attributeName, "UUID") == 0)
                {
                        const char *currentUUID = getStringFromVariant(&attributeDictIter);
                        if (strcasecmp(currentUUID, charUUID) == 0)
                                return true;
                        break;
                }

                // get the next array element
                if (!dbus_message_iter_next(&arrayIter))
                        break;
        }

        // no match here
        return false;
}
