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



#ifndef CURRENTTIMESERVICE_H
#define CURRENTTIMESERVICE_H


#include "GattService.h"



class CurrentTimeService : public GattService
{
public:

        CurrentTimeService();

        bool run(ManagedDevice *device) override;

protected:

        double getComparableTimestamp(int year, int month, int day, int hour, int minute, int second) const;
};

#endif // CURRENTTIMESERVICE_H
