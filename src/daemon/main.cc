#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "Logger.h"
#include "BluezAdapter.h"
#include "DeviceManager.h"
#include "GattService.h"
#include "CurrentTimeService.h"



static bool _shutdown;



static void handleSignal(int signal)
{
        _shutdown = true;
        LOG_INFO("Will shut down due to %s signal.", strsignal(signal));
}


int main(int argc, char **argv)
{
        (void)argc;
        (void)argv;

        // initialization
        Logger::setLogLevel(Logger::Debug);
        LOG_INFO("Starting.");
        BluezAdapter *bluezAdapter = new BluezAdapter("hci0");
        DeviceManager *devices = new DeviceManager(bluezAdapter);
        int servicesCount = 1;
        GattService *services[1];
        services[0] = new CurrentTimeService();

        devices->addManagedDevice("FB:89:02:47:5F:C6");  // sealed PineTime
        devices->addManagedDevice("D9:C7:C5:38:D0:CB");  // development PineTime

        // set up signal handler
        LOG_DEBUG("Setting up OS signal handler.");
        struct sigaction action;
        action.sa_handler = handleSignal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);
        sigaction(SIGHUP, &action, nullptr);

        // enter the daemon's main loop
        LOG_DEBUG("Entering main loop.");
        _shutdown = false;
        while (!_shutdown)
        {
                // the bluetooth adapter has to be powered on
                if (bluezAdapter->powered())
                {
                        // find and connect devices
                        if (devices->allManagedDevicesConnected())
                                LOG_VERBOSE("All managed devices are connected, skipping scan.");
                        else
                        {
                                if (devices->scan())
                                        devices->connectedDiscoveredManagedDevices();
                        }
                        if (_shutdown)
                                break;

                        // run the services
                        for (int i = 0; i < servicesCount; i++)
                        {
                                devices->runService(services[i]);
                                if (_shutdown)
                                        break;
                        }
                }
                else
                        LOG_VERBOSE("Bluetooth adapter is powered off.");

                // sleep for a while
                for (int i = 0; i < 10; i++)
                {
                        if (_shutdown)
                                break;
                        sleep(1);
                }
        }
        LOG_DEBUG("Exited main loop.");

        // clean up
        delete devices;
        for (int i = 0; i < servicesCount; i++)
                delete services[i];
        delete bluezAdapter;

        // done
        LOG_INFO("Exiting.");
        return 0;
}
