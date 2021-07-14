#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "gattlib.h"


#define BLE_SCAN_TIMEOUT   4

#define UUID_SERVICE_CURRENT_TIME          "00001805-0000-1000-8000-00805f9b34fb"
#define UUID_CHARACTERISTIC_CURRENT_TIME   "00002a2b-0000-1000-8000-00805f9b34fb"

#define MAX_MANAGED_DEVICES    16

static int _managedDevicesCount;
static char *_managedDeviceAddress[MAX_MANAGED_DEVICES];
static bool _managedDeviceDiscovered[MAX_MANAGED_DEVICES];
static gatt_connection_t *_managedDeviceConnection[MAX_MANAGED_DEVICES];



static void deviceDiscoveredCallback(void *adapter, const char *address, const char *name, void *userData)
{
        (void)adapter;
        (void)name;
        (void)userData;

        printf("Discovered device %s", address);
        if (name && (strlen(name) > 0))
                printf(" (%s)", name);
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (strcasecmp(address, _managedDeviceAddress[i]) == 0)
                {
                        _managedDeviceDiscovered[i] = true;
                        printf(" --> managed device");
                        break;
                }
        }
        printf("\n");
}


static void deviceDisconnectedCallback(void *userData)
{
        const char *address = reinterpret_cast<const char *>(userData);
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (strcasecmp(address, _managedDeviceAddress[i]) == 0)
                {
                        _managedDeviceDiscovered[i] = false;
                        _managedDeviceConnection[i] = nullptr;
                        printf("Device %s disconnected.\n", _managedDeviceAddress[i]);
                        break;
                }
        }
}


void updateManagedDevicesList()
{
        //TODO: load from config file

        if (_managedDevicesCount == 0)
        {
                _managedDevicesCount = 2;

                _managedDeviceAddress[0] = new char[24];
                strcpy(_managedDeviceAddress[0], "FB:89:02:47:5F:C6");  // sealed PineTime
                _managedDeviceDiscovered[0] = false;
                _managedDeviceConnection[0] = nullptr;

                _managedDeviceAddress[1] = new char[24];
                strcpy(_managedDeviceAddress[1], "D9:C7:C5:38:D0:CB");  // development PineTime
                _managedDeviceDiscovered[1] = false;
                _managedDeviceConnection[1] = nullptr;
        }
}


void connectDevices()
{
        // only continue if at least one of the managed devices isn't connected
        bool allConnected = true;
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                if (!_managedDeviceConnection[i])
                {
                        allConnected = false;
                        break;
                }
        }
        if (allConnected)
        {
                printf("All managed devices are connected, skipping scan.\n");
                return;
        }

        // mark all not-connected devices as not-discovered
        for (int i = 0; i < _managedDevicesCount; i++)
                if (!_managedDeviceConnection[i])
                        _managedDeviceDiscovered[i] = false;

        // scan for BLE devices
        printf("Starting device scan...\n");
        void *adapter = nullptr;
        if (gattlib_adapter_open(nullptr, &adapter) != 0)
        {
                fprintf(stderr, "Could not open default bluetooth adapter.\n");
                return;
        }
        if (gattlib_adapter_scan_enable(adapter, deviceDiscoveredCallback, BLE_SCAN_TIMEOUT, nullptr) != 0)
        {
                fprintf(stderr, "Could not start scanning for BLE devices.\n");
                gattlib_adapter_close(adapter);
                return;
        }
        gattlib_adapter_scan_disable(adapter);
        gattlib_adapter_close(adapter);
        printf("Device scan completed.\n");

        // try to connect newly discovered devices
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                // skip the device if it's not visible or if it has already been connected
                if (!_managedDeviceDiscovered[i] || _managedDeviceConnection[i])
                        continue;

                // try to connect to the device
                printf("Connecting to device %s...\n", _managedDeviceAddress[i]);
                _managedDeviceConnection[i] = gattlib_connect(nullptr, _managedDeviceAddress[i], GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
                if (_managedDeviceConnection[i])
                {
                        // make sure we get notified if the device disconnects
                        gattlib_register_on_disconnect(_managedDeviceConnection[i],
                                                       deviceDisconnectedCallback,
                                                       reinterpret_cast<void *>(_managedDeviceAddress[i]));
                        printf("Connected to device %s.\n", _managedDeviceAddress[i]);
                }
                else
                        fprintf(stderr, "Could not connect to newly discovered device %s.\n", _managedDeviceAddress[i]);
        }
}


void setCurrentTime()
{
        for (int i = 0; i < _managedDevicesCount; i++)
        {
                // this only works for connected devices...
                if (!_managedDeviceConnection[i])
                        continue;

                // get the device's current time
                const char *ctsCharString = UUID_CHARACTERISTIC_CURRENT_TIME;
                uuid_t ctsCharId;
                gattlib_string_to_uuid(ctsCharString, strlen(ctsCharString) + 1, &ctsCharId);
                uint8_t *buffer = nullptr;
                size_t bufferLength = 0;
                int result = gattlib_read_char_by_uuid(_managedDeviceConnection[i], &ctsCharId, reinterpret_cast<void **>(&buffer), &bufferLength);
                if (result != GATTLIB_SUCCESS)
                {
                        if (result == GATTLIB_NOT_FOUND)
                                fprintf(stderr, "Could not find GATT characteristic for CTS on that device.\n");
                        else
                                fprintf(stderr, "Error while reading GATT characteristic for CTS.\n");
                        //TODO: check if the method failed on the first call
                        // device seems to have disconnected
                        gattlib_disconnect(_managedDeviceConnection[i]);
                        _managedDeviceConnection[i] = nullptr;
                        _managedDeviceDiscovered[i] = false;
                        printf("Device %s seems to have disconnected.\n", _managedDeviceAddress[i]);
                        continue;
                }

                // decode the received data
                int devYear = 0;
                int devMonth = 0;
                int devDay = 0;
                int devHour = 0;
                int devMinute = 0;
                int devSecond = 0;
                if (bufferLength < 7)
                {
                        fprintf(stderr, "Received too few bytes.\n");
                        printf("Unknown time on device %s", _managedDeviceAddress[i]);
                }
                else
                {
                        devYear = static_cast<int>(buffer[0]) + (static_cast<int>(buffer[1]) << 8);
                        devMonth = static_cast<int>(buffer[2]);
                        devDay = static_cast<int>(buffer[3]);
                        devHour = static_cast<int>(buffer[4]);
                        devMinute = static_cast<int>(buffer[5]);
                        devSecond = static_cast<int>(buffer[6]);
                        printf("Current time on device %s: %04d-%02d-%02d %02d:%02d:%02d",
                                        _managedDeviceAddress[i],
                                        devYear,
                                        devMonth,
                                        devDay,
                                        devHour,
                                        devMinute,
                                        devSecond);
                }
                free(buffer);
                double devTimestamp = static_cast<double>(devYear) * (60.0 * 60.0 * 24.0 * 31.0 * 12.0)
                        + static_cast<double>(devMonth) * (60.0 * 60.0 * 24.0 * 31.0)
                        + static_cast<double>(devDay) * (60.0 * 60.0 * 24.0)
                        + static_cast<double>(devHour) * (60.0 * 60.0)
                        + static_cast<double>(devMinute) * 60.0
                        + static_cast<double>(devSecond);

                // get current local time
                time_t rawTime;
                time(&rawTime);
                struct tm *timeInfo = localtime(&rawTime);
                double localTimestamp = static_cast<double>(timeInfo->tm_year + 1900) * (60.0 * 60.0 * 24.0 * 31.0 * 12.0)
                        + static_cast<double>(timeInfo->tm_mon + 1) * (60.0 * 60.0 * 24.0 * 31.0)
                        + static_cast<double>(timeInfo->tm_mday) * (60.0 * 60.0 * 24.0)
                        + static_cast<double>(timeInfo->tm_hour) * (60.0 * 60.0)
                        + static_cast<double>(timeInfo->tm_min) * 60.0
                        + static_cast<double>(timeInfo->tm_sec);

                // set the device's time if it deviates more than one second
                double delta = localTimestamp - devTimestamp;
                if (delta < 0.0)
                        delta *= -1.0;
                if (delta > 1.0)
                {
                        buffer = new uint8_t[9];
                        buffer[0] = static_cast<uint8_t>((timeInfo->tm_year + 1900) & 0xff);
                        buffer[1] = static_cast<uint8_t>((timeInfo->tm_year + 1900) >> 8);
                        buffer[2] = static_cast<uint8_t>(timeInfo->tm_mon + 1);
                        buffer[3] = static_cast<uint8_t>(timeInfo->tm_mday);
                        buffer[4] = static_cast<uint8_t>(timeInfo->tm_hour);
                        buffer[5] = static_cast<uint8_t>(timeInfo->tm_min);
                        buffer[6] = static_cast<uint8_t>(timeInfo->tm_sec);
                        buffer[7] = static_cast<uint8_t>(0);   // fractions of seconds
                        buffer[8] = static_cast<uint8_t>(0);   // reason for change
                        result = gattlib_write_char_by_uuid(_managedDeviceConnection[i], &ctsCharId, reinterpret_cast<const void *>(buffer), 9);
                        delete[] buffer;
                        if (result == GATTLIB_SUCCESS)
                        {
                                printf(", updated to %04d-%02d-%02d %02d:%02d:%02d.\n",
                                                timeInfo->tm_year + 1900,
                                                timeInfo->tm_mon + 1,
                                                timeInfo->tm_mday,
                                                timeInfo->tm_hour,
                                                timeInfo->tm_min,
                                                timeInfo->tm_sec);
                        }
                        else
                        {
                                printf(", update FAILED.\n");
                                if (result == GATTLIB_NOT_FOUND)
                                        fprintf(stderr, "Could not find GATT characteristic for CTS on that device.\n");
                                else
                                        fprintf(stderr, "Error while writing GATT characteristic for CTS.\n");
                        }
                }
                else
                        printf(", no update required.\n");
        }
}


int main(int argc, char **argv)
{
        (void)argc;
        (void)argv;

        // initialization
        _managedDevicesCount = 0;
        updateManagedDevicesList();

        // enter the daemon's main loop
        while (true)
        {
                // find and connect devices
                connectDevices();

                // set the devices' current time
                setCurrentTime();

                // sleep for a while
                sleep(10);
        }

        // done
        printf("\nDone.\n");
        return 0;
}

