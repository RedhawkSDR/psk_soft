/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK psk_soft.
 *
 * REDHAWK psk_soft is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK psk_soft is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
#include "psk_soft_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

psk_soft_base::psk_soft_base(const char *uuid, const char *label) :
    Component(uuid, label),
    ThreadedComponent()
{
    loadProperties();

    dataFloat_in = new bulkio::InFloatPort("dataFloat_in");
    addPort("dataFloat_in", "Float input for complex baseband data to be demodulated. ", dataFloat_in);
    dataFloat_out = new bulkio::OutFloatPort("dataFloat_out");
    addPort("dataFloat_out", "Complex Soft-Desition output. ", dataFloat_out);
    dataShort_out = new bulkio::OutShortPort("dataShort_out");
    addPort("dataShort_out", "Short output for bits, zero or one. Differential Decoding can be turned on with a property setting.\nSymbol to Bit Mapping as follows:\n2 Symbols: 	Phase: 0 		Bit: 0\n					Phase: pi		Bit: 1\n\n4 Symbols: \n					Phase: pi/4 		Bit: 0\n					Phase: 3pi/4	Bit: 01\n					Phase: 5pi/4	Bit: 10\n					Phase: 7pi/4	Bit: 11\n\n8 Symbols: \n					Phase: 0			Bit: 000\n					Phase: pi/4		Bit: 001\n					Phase: pi/2		Bit: 010\n					Phase: 3pi/4	Bit: 011\n					Phase: pi 		Bit: 100\n					Phase: 5pi/4	Bit: 101\n					Phase: 3pi/2	Bit: 110\n					Phase: 7pi/4	Bit: 111\n\n", dataShort_out);
    phase_out = new bulkio::OutFloatPort("phase_out");
    addPort("phase_out", "Float output containing phase estimate for debuggging. One phase estimate per symbol output. Phase is unwrapped.   \n", phase_out);
    sampleIndex_out = new bulkio::OutShortPort("sampleIndex_out");
    addPort("sampleIndex_out", "Index of sample used in timing recovery chosen for symbol output. Will range from 0 to samplesPerBaud-1.  ", sampleIndex_out);
}

psk_soft_base::~psk_soft_base()
{
    delete dataFloat_in;
    dataFloat_in = 0;
    delete dataFloat_out;
    dataFloat_out = 0;
    delete dataShort_out;
    dataShort_out = 0;
    delete phase_out;
    phase_out = 0;
    delete sampleIndex_out;
    sampleIndex_out = 0;
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void psk_soft_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    Component::start();
    ThreadedComponent::startThread();
}

void psk_soft_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    Component::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void psk_soft_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the component running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    Component::releaseObject();
}

void psk_soft_base::loadProperties()
{
    addProperty(samplesPerBaud,
                10,
                "samplesPerBaud",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(numAvg,
                100,
                "numAvg",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(constelationSize,
                4,
                "constelationSize",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(phaseAvg,
                50,
                "phaseAvg",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(differentialDecoding,
                false,
                "differentialDecoding",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(resetState,
                false,
                "resetState",
                "",
                "readwrite",
                "",
                "external",
                "property");

}


