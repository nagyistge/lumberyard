/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef DRILLER_CARRIER_CARRIEROPERATIONTELEMETRYEVENT_H
#define DRILLER_CARRIER_CARRIEROPERATIONTELEMETRYEVENT_H

#include "Woodpecker/Telemetry/TelemetryEvent.h"

namespace Driller
{
    class CarrierOperationTelemetryEvent
        : public Telemetry::TelemetryEvent
    {
    public:
        CarrierOperationTelemetryEvent()
            : Telemetry::TelemetryEvent("CarrierDataViewOperation")
        {
        }
    };
}

#endif