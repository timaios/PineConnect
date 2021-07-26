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
#include "AlertNotificationService.h"
#include "DBusEventWatcher.h"
#include "NotificationEventSink.h"



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
        DBusEventWatcher *sessionBusWatcher = new DBusEventWatcher(true);
        NotificationEventSink *notificationEventSink = new NotificationEventSink();
        sessionBusWatcher->registerSink(notificationEventSink);
        BluezAdapter *bluezAdapter = new BluezAdapter("hci0");
        DeviceManager *devices = new DeviceManager(bluezAdapter);
        int servicesCount = 2;
        GattService *services[2];
        services[0] = new CurrentTimeService();
        services[1] = new AlertNotificationService(notificationEventSink);

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
                // check message queue for ten seconds at most
                sessionBusWatcher->checkQueue(10);

                // the bluetooth adapter has to be powered on
                if (bluezAdapter->powered())
                {
                        // find and connect devices
                        if (devices->allManagedDevicesConnected())
                                LOG_VERBOSE("All managed devices are connected, skipping scan.");
                        else
                        {
                                // start the scan
                                if (devices->startScan())
                                {
                                        // while waiting for the scan to complete, check the message queue
                                        sessionBusWatcher->checkQueue(4);

                                        // stop scan and connect devices
                                        if (devices->stopScan())
                                                devices->connectedDiscoveredManagedDevices();
                                }
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

                        // the AlertNotification service should have sent all pending alerts
                        notificationEventSink->clearNotificationQueue();
                }
                else
                        LOG_VERBOSE("Bluetooth adapter is powered off.");
        }
        LOG_DEBUG("Exited main loop.");

        // clean up
        delete devices;
        for (int i = 0; i < servicesCount; i++)
                delete services[i];
        delete bluezAdapter;
        delete sessionBusWatcher;
        delete notificationEventSink;

        // done
        LOG_INFO("Exiting.");
        return 0;
}
