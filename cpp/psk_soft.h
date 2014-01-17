#ifndef PSK_SOFT_IMPL_H
#define PSK_SOFT_IMPL_H

#include "psk_soft_base.h"

class psk_soft_i;


class LinearFit
{
// class for calculating a linear fit for uniformly sampled data
// to use this class do the following:
// you tell it the number of points to use in your fit (the history) and sample rate of the data
//
// you then pass in one data point at a time and it returns the point which is the linear best fit given our current history
public:
	LinearFit(size_t numPts, float sampleRate);
	float next(float yval);
	void reset(size_t* numPts=NULL, float* sampleRate=NULL, bool forceHistoryClear=false);
private:
	//here are the two equations which do the best fit
	float calculateFit();
	void calculateDenominator();
	std::deque<float> yvals;
	float m;
	float b;

	double ySum;
	double xySum;
	size_t n;
	float xdelta;
	float denominator;
	float xAvg;
};


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
        std::vector<double> symbolEnergy;
        size_t index;
        void samplesPerBaudChanged(const std::string& id);
        void constelationSizeChanged(const std::string& id);
        void phaseAvgChanged(const std::string& id);

        bool resetSamplesPerBaud;
        bool resetNumSymbols;
        bool resetPhaseAvg;

        float phaseEstimate;
        float sampleRate;

        LinearFit phaseEstimator;
};

#endif
