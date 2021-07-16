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



#include "Device.h"

#include <stdlib.h>
#include <string.h>

#include "Logger.h"



Device::Device(const char *address, const char *name)
{
        // address is mandatory
        if (!address)
        {
                LOG_DEBUG("address has not been provided!");
                _address = new char[32];
                strcpy(_address, "00:00:00:00:00:00");
        }
        else
        {
                _address = new char[strlen(address) + 1];
                strcpy(_address, address);
        }

        // name is optional
        if (name)
        {
                _name = new char[strlen(name) + 1];
                strcpy(_name, name);
        }
        else
                _name = nullptr;
}


Device::~Device()
{
        delete[] _address;
        if (_name)
                delete[] _name;
}
