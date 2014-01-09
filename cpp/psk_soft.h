#ifndef PSK_SOFT_IMPL_H
#define PSK_SOFT_IMPL_H

#include "psk_soft_base.h"

class psk_soft_i;

class psk_soft_i : public psk_soft_base
{
    ENABLE_LOGGING
    public:
        psk_soft_i(const char *uuid, const char *label);
        ~psk_soft_i();
        int serviceFunction();
    private:
        std::deque<std::complex<float> > samples;
        std::deque<double> energy;
        std::deque<float>  phase;
        std::vector<double> symbolEnergy;
        size_t index;
        double phaseSum;
        void samplesPerBaudChanged(const std::string& id);
        void constelationSizeChanged(const std::string& id);

        bool resetSamplesPerBaud;
        bool resetNumSymbols;

};

#endif
