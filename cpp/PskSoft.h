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
#ifndef PskSoft_IMPL_H
#define PskSoft_IMPL_H

#include "PskSoft_base.h"

/* Class for calculating a linear fit for uniformly sampled data.
 * To use this class do the following:
 * 1. Specify the number of points to use in the fit (the history) and sample rate of the data.
 * 2. Pass in one data point at a time. The point which is the linear best fit given the current history is returned.
 */
class LinearFit
{
public:
	LinearFit(size_t numPts, float sampleRate);
	float next(float yval);
	float reset(size_t* numPts=NULL, float* sampleRate=NULL, bool forceHistoryClear=false);
	float subtractConst(float yval);
private:
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


class PskSoft_i : public PskSoft_base
{
    ENABLE_LOGGING
    public:
        PskSoft_i(const char *uuid, const char *label);
        ~PskSoft_i();
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
