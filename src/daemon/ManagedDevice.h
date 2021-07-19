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



#ifndef MANAGEDDEVICE_H
#define MANAGEDDEVICE_H


#include <stdlib.h>
#include <mutex>

#include "Device.h"

class BluezAdapter;



class ManagedDevice : public Device
{
public:

        ManagedDevice(BluezAdapter *bluezAdapter, const char *address);
        ~ManagedDevice() override;

        void setName(const char *value);

        bool isConnected();
        bool connect();
        bool disconnect();

        int readCharacteristic(const char *charGuid, uint8_t *buffer, int bufferSize);
        bool writeCharacteristic(const char *charGuid, uint8_t *buffer, int length);

private:

        BluezAdapter *_bluezAdapter;
};

#endif // MANAGEDDEVICE_H
