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



#ifndef ALERTNOTIFICATIONSERVICE_H
#define ALERTNOTIFICATIONSERVICE_H


#include "GattService.h"

class NotificationEventSink;



class AlertNotificationService : public GattService
{
public:

        AlertNotificationService(NotificationEventSink *eventSink);

        bool run(ManagedDevice *device) override;

private:

        NotificationEventSink *_eventSink;
};

#endif // ALERTNOTIFICATIONSERVICE_H
