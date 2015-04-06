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
	float reset(size_t* numPts=NULL, float* sampleRate=NULL, bool forceHistoryClear=false);
	float subtractConst(float yval);
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
	size_t count;
};


class psk_soft_i : public psk_soft_base
{
    ENABLE_LOGGING
    public:
        psk_soft_i(const char *uuid, const char *label);
        ~psk_soft_i();
        int serviceFunction();
    private:
        static const double M_2PI = 2*M_PI;
        std::deque<std::complex<float> > samples;
        std::deque<double> energy;
        std::vector<double> symbolEnergy;
        size_t index;
        std::complex<float> last;
        void samplesPerBaudChanged(const std::string& id);
        void constelationSizeChanged(const std::string& id);
        void phaseAvgChanged(const std::string& id);

        bool resetSamplesPerBaud;
        bool resetNumSymbols;
        bool resetPhaseAvg;

        float phaseEstimate;
        float sampleRate;

        size_t count;

        void resyncEnergy(const size_t& samplesPerSymbol, const size_t& numDataPts);

        LinearFit phaseEstimator;
};

#endif
