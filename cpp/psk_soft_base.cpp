#include "psk_soft_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

psk_soft_base::psk_soft_base(const char *uuid, const char *label) :
    Resource_impl(uuid, label),
    ThreadedComponent()
{
    loadProperties();

    dataFloat_in = new bulkio::InFloatPort("dataFloat_in");
    addPort("dataFloat_in", dataFloat_in);
    dataFloat_out = new bulkio::OutFloatPort("dataFloat_out");
    addPort("dataFloat_out", dataFloat_out);
    dataShort_out = new bulkio::OutShortPort("dataShort_out");
    addPort("dataShort_out", dataShort_out);
    phase_out = new bulkio::OutFloatPort("phase_out");
    addPort("phase_out", phase_out);
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
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void psk_soft_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    Resource_impl::start();
    ThreadedComponent::startThread();
}

void psk_soft_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    Resource_impl::stop();
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

    Resource_impl::releaseObject();
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
                "configure");

    addProperty(numAvg,
                100,
                "numAvg",
                "",
                "readwrite",
                "",
                "external",
                "configure");

    addProperty(constelationSize,
                4,
                "constelationSize",
                "",
                "readwrite",
                "",
                "external",
                "configure");

    addProperty(phaseAvg,
                50,
                "phaseAvg",
                "",
                "readwrite",
                "",
                "external",
                "configure");

}


