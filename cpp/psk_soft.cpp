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
/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "psk_soft.h"
#include "complex"
#include <cmath>

PREPARE_LOGGING(psk_soft_i)

LinearFit::LinearFit (size_t numPts, float sampleRate):
	ySum(0),
	xySum(0),
	n(numPts),
	xdelta(1.0/sampleRate),
	count(0)
{
}

float LinearFit::next(float yval)
{
	//try to cope with systemic floating point math errors
	if (count==1048576)
		reset();
	//are we currently in steady state
	bool steadyState =  yvals.size()==n;
	if (steadyState)
	{
		//update our state given our x-axis shift and
		//our loss of the last point

		// here is a bit of the magic for the xySum update equqtion:
		// xySum = sum(xi*yi) = y0*0+y1*xdelta + y2*2*xdelta + ... yn-1*(n-1)*xdelta
		// for the next value for xySum - we time shift our xaxis (to keep the earliest point at time 0)
		// xySumNext = y1*0 + y2*xdelta +y3*2*xdelta + ... yn-1*(n-2)*xdelta + newYval*(n-1)*xdelta
		// xySumNext - xySum = -xdelta*(y1+y2+...yn-1) + newYval*(n-1)*xdelta
		// but ySumNext = ySum-y0 = y1+y2+...yn-1
		// xySumNext - xySum = -xdelta*ySumNext + newYval*(n-1)*xdelta
		// and the final update equation becomes xySum = -xdelta*newYSum+newYval*(n-1)*xdelta

		//we take care of the last term providing the new value update outside of the steady state check

		ySum-=yvals.front();
		yvals.pop_front();
		xySum-=xdelta*ySum;
	}
	//update our state for our new point acording to the update equations
	ySum+=yval;
	//this is actually multiplying by yval*(n-1)*xdelta
	//because we haven't yet pushed_back the new value.  This is intenional
	xySum+=yval*yvals.size()*xdelta;
	yvals.push_back(yval);

	if (! steadyState)
		//if the size of our data vector has changed we need to recalculate the denominator
		calculateDenominator();
	//calculate the best fit given our state
	count++;
	return calculateFit();
}

float LinearFit::reset(size_t* numPts, float* sampleRate, bool forceHistoryClear)
{
	if (sampleRate!=NULL)
	{
		//can't use history once we reset the sample rate
		float newXdelta = 1.0/(*sampleRate);
		if (xdelta!=newXdelta)
		{
			xdelta = newXdelta;
			forceHistoryClear=true;
		}
	}
	if (forceHistoryClear)
		yvals.clear();

	if (numPts !=NULL && *numPts != n)
	{
		n = *numPts;
		while (yvals.size() >n)
			yvals.pop_front();
	}
	//now update our internal state given our history and sample rate
	unsigned int j=0;
	ySum=0;
	xySum=0;
	//recalculate or yVal state directly from the yvals deque
	for (std::deque<float>::iterator i =yvals.begin(); i!=yvals.end(); i++, j++)
	{
		ySum+=*i;
		xySum+=j*xdelta*(*i);
	}
	calculateDenominator();
	count=0;
    return calculateFit();

}

float LinearFit::subtractConst(float yval)
{
	for (std::deque<float>::iterator i =yvals.begin(); i!=yvals.end(); i++)
	{
		*i-=yval;
	}
	return reset();
}

float LinearFit::calculateFit()
{
	// The General best fit for a linear fit (to minimize avg error is as follows:

	// y = mx+b
	// numerator = sum(xi*yi) -1/n*sum(xi)*sum(yi);
	// denominator = sum(xi^2) - 1/n(sum(x))^2
	// m = numerator / denominator
	// b = sum(y)/n - m sum(x) / n

	// we simplify this equation for regularly sampled data: xi = i*xdelta
	// (xvals = 0, xdelta, 2*xdelta, ... (n-1)*xdelta)

	// Thus - the numerator reduces to the following equation:
	// numerator  = sum(xi*yi) -xdelta*(n-1)/2*sum(yi);
	// the denominator simplifies to a constant for a given xdelta and n:
	// denominator =xdelta^2*((n-1)^3/3 + (n-1)^2/2 +(n-1)/6 - 1/4*(n-1)^2*n))

	size_t pts = yvals.size();
	if (pts>1)
	{
		size_t pts_m_1 = pts-1;
		m = (xySum-xdelta*pts_m_1/2*ySum)/denominator;
		b = ySum/pts-m*xAvg;

		//calculate the best fit point for our new data point
		float xVal = xdelta*pts_m_1;
		return m*xVal + b;
	}
	else
	{
		m=0;
		if (pts==0)
			b=0;
		else
			b=yvals.back();
		return b;
	}

}

void LinearFit::calculateDenominator()
{
	//update the denominator for the current sample rate and number of points we are fitting
	size_t pts = yvals.size();
	if (pts<=1)
		return;
	size_t pts_m_1 = pts -1;
	denominator = pow(xdelta,2)*(pow(pts_m_1,3)/3.0+pow(pts_m_1,2)/2.0+(pts_m_1)/6.0- pow(pts_m_1,2)*pts/4.0);
	xAvg = xdelta*(pts_m_1)/2;
}

psk_soft_i::psk_soft_i(const char *uuid, const char *label) :
    psk_soft_base(uuid, label),
    symbolEnergy(samplesPerBaud,0.0),
    index(0),
    resetSamplesPerBaud(false),
    resetNumSymbols(false),
    resetPhaseAvg(false),
    phaseEstimate(0.0),
    sampleRate(1.0), //put in a bogus sample rate we will update it later
    count(0),
    phaseEstimator(phaseAvg,sampleRate)
{
	 setPropertyChangeListener("samplesPerBaud", this, &psk_soft_i::samplesPerBaudChanged);
	 setPropertyChangeListener("constelationSize", this, &psk_soft_i::constelationSizeChanged);
	 setPropertyChangeListener("phaseAvg", this, &psk_soft_i::phaseAvgChanged);

}

psk_soft_i::~psk_soft_i()
{
}

/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
                std::string stream_id = "testStream";
                BULKIO::StreamSRI sri = bulkio::sri::create(stream_id);

	Time:
	    To create a PrecisionUTCTime object, use the following code:
                BULKIO::PrecisionUTCTime tstamp = bulkio::time::utils::now();

        
    Ports:

        Data is passed to the serviceFunction through the getPacket call (BULKIO only).
        The dataTransfer class is a port-specific class, so each port implementing the
        BULKIO interface will have its own type-specific dataTransfer.

        The argument to the getPacket function is a floating point number that specifies
        the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.  Constants have been defined for these values, bulkio::Const::BLOCKING and
        bulkio::Const::NON_BLOCKING.

        Each received dataTransfer is owned by serviceFunction and *MUST* be
        explicitly deallocated.

        To send data using a BULKIO interface, a convenience interface has been added 
        that takes a std::vector as the data input

        NOTE: If you have a BULKIO dataSDDS port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // this example assumes that the component has two ports:
            //  A provides (input) port of type bulkio::InShortPort called short_in
            //  A uses (output) port of type bulkio::OutFloatPort called float_out
            // The mapping between the port and the class is found
            // in the component base class header file

            bulkio::InShortPort::dataTransfer *tmp = short_in->getPacket(bulkio::Const::BLOCKING);
            if (not tmp) { // No data is available
                return NOOP;
            }
	energy(samplesPerBaud*numAvg);
            std::vector<float> outputData;
            outputData.resize(tmp->dataBuffer.size());
            for (unsigned int i=0; i<tmp->dataBuffer.size(); i++) {
                outputData[i] = (float)tmp->dataBuffer[i];
            }

            // NOTE: You must make at least one valid pushSRI call
            if (tmp->sriChanged) {
                float_out->pushSRI(tmp->SRI);
            }
            float_out->pushPacket(outputData, tmp->T, tmp->EOS, tmp->streamID);

            delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
            return NORMAL;

        If working with complex data (i.e., the "mode" on the SRI is set to
        true), the std::vector passed from/to BulkIO can be typecast to/from
        std::vector< std::complex<dataType> >.  For example, for short data:

            bulkio::InShortPort::dataTransfer *tmp = myInput->getPacket(bulkio::Const::BLOCKING);
            std::vector<std::complex<short> >* intermediate = (std::vector<std::complex<short> >*) &(tmp->dataBuffer);
            // do work here
            std::vector<short>* output = (std::vector<short>*) intermediate;
            myOutput->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);

        Interactions with non-BULKIO ports are left up to the component developer's discretion

    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given a generated name of the form
        "prop_n", where "n" is the ordinal number of the property in the PRF file.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (psk_soft_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
              
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        A callback method can be associated with a property so that the method is
        called each time the property value changes.  This is done by calling 
        setPropertyChangeListener(<property name>, this, &psk_soft_i::<callback method>)
        in the constructor.
            
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            
        //Add to psk_soft.cpp
        psk_soft_i::psk_soft_i(const char *uuid, const char *label) :
            psk_soft_base(uuid, label)
        {
            setPropertyChangeListener("scaleValue", this, &psk_soft_i::scaleChanged);
        }

        void psk_soft_i::scaleChanged(const std::string& id){
            std::cout << "scaleChanged scaleValue " << scaleValue << std::endl;
        }
            
        //Add to psk_soft.h
        void scaleChanged(const std::string&);
        
        
************************************************************************************************/
int psk_soft_i::serviceFunction()
{

	bulkio::InFloatPort::dataTransfer *tmp = dataFloat_in->getPacket(bulkio::Const::BLOCKING);
	if (not tmp) { // No data is available
		return NOOP;
	}

	if (tmp->SRI.mode!=1)
	{
		std::cout<<"CANNOT work with real data"<<std::endl;
		return NORMAL;
	}

	if (resetState)
	{
		LOG_DEBUG(psk_soft_i, "psk_soft_i reset state");
		resetSamplesPerBaud=true;
		resetNumSymbols=true;
		resetPhaseAvg=true;
		resetState = false;
	}

	//cash off local values in case user configures properties during our processing loop

	const size_t samplesPerSymbol = samplesPerBaud;
	const size_t numDataPts = samplesPerSymbol*numAvg;
	const size_t numSyms = constelationSize;
	//this should only happen if numAvg or samplesPerBaud has shrunk
	if (numDataPts > samples.size())
	{
		resetSamplesPerBaud = true;
	}
	size_t bitsPerBaud=0;
	if (numSyms==2)
		bitsPerBaud=1;
	else if (numSyms==4)
		bitsPerBaud=2;
	else if (numSyms==8)
		bitsPerBaud=3;

	// NOTE: You must make at least one valid pushSRI call
	if (tmp->sriChanged || resetNumSymbols|| resetSamplesPerBaud) {
		if (tmp->SRI.xdelta != sampleRate)
		{
			sampleRate = 1.0/tmp->SRI.xdelta;
			phaseEstimator.reset(NULL,&sampleRate);
		}
		tmp->SRI.xdelta*=samplesPerSymbol;
		dataFloat_out->pushSRI(tmp->SRI);
		tmp->SRI.mode=0;
		phase_out->pushSRI(tmp->SRI);
		tmp->SRI.xdelta/=bitsPerBaud;
		dataShort_out->pushSRI(tmp->SRI);
	}

	//user has changed our oversample factor - resize the symbolEnergy vector and repopulate it with the energy samples
	if (resetSamplesPerBaud)
	{
		resyncEnergy(samplesPerSymbol, numDataPts);
		resetSamplesPerBaud=false;
	}

	//all our phase caluclations are invalid if we change our constelation size
	//just clear the phaseSum and the phaseVec and start with new estimates
	if (resetNumSymbols)
	{
		phaseEstimator.reset(NULL,NULL,true);
		resetNumSymbols=false;
	}
	if (resetPhaseAvg)
	{
		size_t numPts(phaseAvg);
		phaseEstimator.reset(&numPts);
		resetPhaseAvg=false;
	}

	std::vector<std::complex<float> >* dataVec = (std::vector<std::complex<float> >*) &(tmp->dataBuffer);

	std::vector<std::complex<float> > out;
	std::vector<short> bits;
	std::vector<float> phase_vec;
	//reserve data for the output
	out.reserve((dataVec->size()+index)/samplesPerSymbol);
	phase_vec.reserve(out.size());
	bits.reserve(out.size()*bitsPerBaud);
    std::vector<short> sampleIndexOut;
    sampleIndexOut.reserve(out.size());

	std::complex<float> sample;
	const size_t lastSample = samplesPerSymbol-1;
	for (std::vector<std::complex<float> >::iterator i=dataVec->begin(); i!=dataVec->end(); i++)
	{
		//push back the sample and its energy
		if (samplesPerSymbol >1)
		{
			samples.push_back(*i);
			double sampleEnergy = norm(*i);
			energy.push_back(sampleEnergy);
			//add energy to the symbolEnergy vector
			symbolEnergy[index]+=sampleEnergy;
		}
		//when we reach the end of the next symbol
		if (index== lastSample)
		{
			//if we have enough samples to get meaningful averages we start outputing data
			if (samples.size()==numDataPts)
			{
				if (samplesPerSymbol>1)
				{
					//get the index from the end for the max symbolEnergy
					size_t sampleIndex = std::distance(symbolEnergy.begin(), std::max_element(symbolEnergy.begin(),symbolEnergy.end()));

					//this is the sample we need to output
					sample= *(samples.begin()+sampleIndex);
					sampleIndexOut.push_back(sampleIndex);
				}
				else
					sample = *i;

				//algorthim to compensate for phase offset
				//note this isn't needed for differential decoding
				//but since we have phase as a debug float out we do the calculations regardless
				double thisPhase = arg(pow(sample,numSyms));
//				std::cout<<"thisPhase = "<<thisPhase<<std::endl;

				//do phase unwrapping here with previous phase estimates
				long numWraps = round((phaseEstimate-thisPhase)/M_2PI);
				thisPhase += +numWraps*M_2PI;

				//compute the average phase
				phaseEstimate = phaseEstimator.next(thisPhase);
				phase_vec.push_back(phaseEstimate);

				float phaseCorrection=0;

				if (differentialDecoding)
				{
					std::complex<float> decoded = sample/last;
					last = sample;
					sample = decoded;
				}
				else
				{
					phaseCorrection = -phaseEstimate/numSyms;
				}
				//compute the phase offset - we add PI/4 so that we get samples at (+/- 1, +/-j) instead of 0,1,-1,,-j
				if (numSyms==4)
					phaseCorrection+=M_PI_4;
				std::complex<float> phaseCorrectionPhasor= std::polar(float(1.0),phaseCorrection);
				std::complex<float> corrected(sample*phaseCorrectionPhasor);
				out.push_back(corrected);
				//do conversion to bits
				if (bitsPerBaud==1)
				{
					//
					//                  |             // A -> 0
					//                  |             // B -> 1
					//             B---------A
					//                  |
					//                  |

					bits.push_back((out.back().real()<0));
				}
				else if (bitsPerBaud==2)
				{
					//
					//             B    |    A         // A -> 00 (0)
					//                  |              // B -> 01 (1)
					//              ---------          // C -> 10 (2)
					//                  |              // D -> 11 (3)
					//             C    |    D

					bool real = out.back().real();
					bool imag = out.back().imag();
					bits.push_back(real ^ imag);
					bits.push_back(not imag);
				}
				else if (bitsPerBaud==3)
				{

					//                  C
					//             D    |    B         // A -> 000 (0)   E -> 100 (4)
					//                  |              // B -> 001 (1)   F -> 101 (5)
					//            E  --------- A       // C -> 010 (2)   G -> 110 (6)
					//                  |              // D -> 011 (3)   H -> 111 (7)
					//             F    |    H
                    //                  G

					//TIME to map the 8 psk constelation into bits
					//this was harder then it should have been

					// we have clusters arround theta 0, pi/4, pi/2, 3pi/4, pi, 5pi/4, 3pi/2, 7pi/8
					// however, arg returns a number between -pi and pi.  Those phases near -pi AND near pi map to the same cluster

					//this is what we do to provide some rudamentary mapping

					//get the phase -pi<theta<pi
					float theta = arg(out.back());
					//convert the phase to soft symobols -4 <=softsym < 4
					float softsym = theta/M_PI*4;
					//now wrap the negative numbers over to positive numbers -.5<=softsym<7.5
					if (softsym <-.5)
						softsym +=8;
					//now round to the closest integer
					//This gives symbols betweeen 0 <=sym<= 7
					unsigned short sym = round(softsym);
					//std::cout<<"got phase "<<theta<<", "<<softsym<<", sym "<<sym <<" , ";
					//now unpack the bits
					// the trick is that both 0 and 8 have the same values for 3 least significant bits (0,0,0) - so they will produce the same bits;
					for (size_t j=0; j!=3; j++)
					{
						bits.push_back(sym&1);
						sym=sym>>1;
						//std::cout<<bits.back();
					}
					//std::cout<<std::endl;
				}
				else
					std::cout<<"numSyms "<<numSyms<< " not supported - no bits out"<<std::endl;

				if (samplesPerSymbol>1)
				{

					//subtract the energy for this symbol from the symbolEnergy vector
					std::vector<double>::iterator symIter = symbolEnergy.begin();
					std::deque<double>::iterator energyIterEnd = energy.begin()+samplesPerSymbol;
					for (std::deque<double>::iterator energyIter = energy.begin(); energyIter !=energyIterEnd;energyIter++, symIter++)
					{
						*symIter-=*energyIter;
					}
					//remove all samples from this symbol from the samples & energy containers
					energy.erase(energy.begin(), energyIterEnd);
					samples.erase(samples.begin(), samples.begin()+samplesPerSymbol);
					count++;
					if (count==1048576)
						resyncEnergy(samplesPerSymbol, numDataPts);
				}
			}
			//reset our symbolIndex back to 0
			index=0;
		}
		else
			index++;
	}
	//wrap phase estimate back to a reasonable value to keep it from going to infinitie
	//we wrap about numSyms*2pi and NOT 2PI or we introduce phase offsets
	//since phaseEstimate is the estiamte of the numSyms power of the phase
	float wrapValue = M_2PI*numSyms;
	if (abs(phaseEstimate)> wrapValue)
	{
		long numWraps = round(phaseEstimate/wrapValue);
		//we subtract the phaseOffset from the estiamtor.  This takes care of doing it for all the history
		//and reseting the state
		float newPhaseEstimate = phaseEstimator.subtractConst(numWraps*wrapValue);
		//assert(abs((phaseEstimate-numWraps*M_2PI)-out)<.01);
		phaseEstimate = newPhaseEstimate;
	}

	if (!out.empty())
	{
		std::vector<float>* output = (std::vector<float>*)&out;
		dataFloat_out->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);
	}
	if (!bits.empty())
		dataShort_out->pushPacket(bits, tmp->T, tmp->EOS, tmp->streamID);
	if (!phase_vec.empty())
		phase_out->pushPacket(phase_vec, tmp->T, tmp->EOS, tmp->streamID);
	if (!sampleIndexOut.empty())
		sampleIndex_out->pushPacket(sampleIndexOut, tmp->T, tmp->EOS, tmp->streamID);
	delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
	return NORMAL;
}
void psk_soft_i::resyncEnergy(const size_t& samplesPerSymbol, const size_t& numDataPts)
{
	symbolEnergy.assign(samplesPerSymbol,0.0);
	if (samples.size()> numDataPts)
	{
		samples.erase(samples.begin()+numDataPts,samples.end());
		energy.erase(energy.begin()+numDataPts,energy.end());
	}
	index=0;
	for (std::deque<double>::iterator i = energy.begin(); i!= energy.end(); i++)
	{
		symbolEnergy[index]+=*i;
		index++;
		if (index==samplesPerSymbol)
			index=0;
	}
	count=0;
}

void psk_soft_i::samplesPerBaudChanged(const std::string& id){
   std::cout << "samplesPerBaudChanged " << samplesPerBaud<< std::endl;
   resetSamplesPerBaud=(samplesPerBaud!=symbolEnergy.size());
}

void psk_soft_i::constelationSizeChanged(const std::string& id){
   std::cout << "numSymbolsChanged " << samplesPerBaud<< std::endl;
   resetNumSymbols=true;
}

void psk_soft_i::phaseAvgChanged(const std::string& id){
   std::cout << "phaseAvgChanged " << phaseAvg<< std::endl;
   resetPhaseAvg=true;
}
