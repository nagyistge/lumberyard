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

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "Wavelet.h"


namespace MCore
{
    /**
     *
     *
     *
     */
    class MCORE_API CDF97Wavelet
        : public Wavelet
    {
    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        CDF97Wavelet();
        ~CDF97Wavelet();    // automatically releases the temp buffer

        uint32 GetType() const override                 { return CDF97Wavelet::TYPE_ID; }
        const char* GetName() const override            { return "CDF97Wavelet"; }

    private:
        void PartialTransform(float* signal, uint32 signalLength) override;
        void PartialInverseTransform(float* coefficients, uint32 signalLength) override;
    };
}   // namespace MCore
