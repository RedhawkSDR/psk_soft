#ifndef PSK_SOFT_IMPL_BASE_H
#define PSK_SOFT_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Resource_impl.h>
#include <ossie/ThreadedComponent.h>

#include <bulkio/bulkio.h>

class psk_soft_base : public Resource_impl, protected ThreadedComponent
{
    public:
        psk_soft_base(const char *uuid, const char *label);
        ~psk_soft_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        unsigned short samplesPerBaud;
        CORBA::ULong numAvg;
        unsigned short constelationSize;
        unsigned short phaseAvg;
        bool differentialDecoding;
        bool resetState;

        // Ports
        bulkio::InFloatPort *dataFloat_in;
        bulkio::OutFloatPort *dataFloat_out;
        bulkio::OutShortPort *dataShort_out;
        bulkio::OutFloatPort *phase_out;
        bulkio::OutShortPort *sampleIndex_out;

    private:
};
#endif // PSK_SOFT_IMPL_BASE_H
