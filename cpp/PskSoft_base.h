/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK PskSoft.
 *
 * REDHAWK PskSoft is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK PskSoft is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
#ifndef PskSoft_BASE_IMPL_BASE_H
#define PskSoft_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Component.h>
#include <ossie/ThreadedComponent.h>

#include <bulkio/bulkio.h>

class PskSoft_base : public Component, protected ThreadedComponent
{
    public:
        PskSoft_base(const char *uuid, const char *label);
        ~PskSoft_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        /// Property: samplesPerBaud
        unsigned short samplesPerBaud;
        /// Property: numAvg
        CORBA::ULong numAvg;
        /// Property: constelationSize
        unsigned short constelationSize;
        /// Property: phaseAvg
        unsigned short phaseAvg;
        /// Property: differentialDecoding
        bool differentialDecoding;
        /// Property: resetState
        bool resetState;

        // Ports
        /// Port: dataFloat_in
        bulkio::InFloatPort *dataFloat_in;
        /// Port: softDecision_dataFloat_out
        bulkio::OutFloatPort *softDecision_dataFloat_out;
        /// Port: bits_dataShort_out
        bulkio::OutShortPort *bits_dataShort_out;
        /// Port: phase_dataFloat_out
        bulkio::OutFloatPort *phase_dataFloat_out;
        /// Port: sampleIndex_dataShort_out
        bulkio::OutShortPort *sampleIndex_dataShort_out;

    private:
};
#endif // PskSoft_BASE_IMPL_BASE_H
