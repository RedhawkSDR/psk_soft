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

psk_soft_i::psk_soft_i(const char *uuid, const char *label) :
    psk_soft_base(uuid, label),
    symbolEnergy(samplesPerBaud,0.0),
    index(0),
    reset(false)
{
	 setPropertyChangeListener("samplesPerBaud", this, &psk_soft_i::samplesPerBaudChanged);
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
	size_t samplesPerSymbol = samplesPerBaud;
	size_t numDataPts = samplesPerSymbol*numAvg;
	if (reset)
	{
		//going to be a bit sloppy here and introudce some bit slips since this is for display purposes only
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

	}
	if (tmp->SRI.mode!=1)
	{
		std::cout<<"CANNOT work with real data"<<std::endl;
		return NORMAL;
	}
	std::cout<<"got elements"<<tmp->dataBuffer.size()<<std::endl;
	std::vector<std::complex<float> >* dataVec = (std::vector<std::complex<float> >*) &(tmp->dataBuffer);


	std::vector<std::complex<float> > out;
	out.reserve((dataVec->size()+index)/samplesPerSymbol);

	std::cout<<" samplesPerSymbol = "<<samplesPerSymbol<<", "<<"samplesPerBaud = "<<samplesPerBaud<<" numDataPts = "<<numDataPts<<std::endl;

	for (std::vector<std::complex<float> >::iterator i=dataVec->begin(); i!=dataVec->end(); i++)
	{
		//std::cout<<"AAA index = "<<index<<", samples.size() = "<<samples.size()<<std::endl;
		samples.push_back(*i);
		double sampleEnergy = norm(*i);
		energy.push_back(sampleEnergy);
		symbolEnergy[index]+=sampleEnergy;
		index++;
		//std::cout<<"BBB index = "<<index<<", samples.size() = "<<samples.size()<<std::endl;
		if (index== samplesPerSymbol)
		{
			if (samples.size()==numDataPts)
			{
				//get the index from the end for the max symbolEnergy
				size_t sampleIndex = std::distance(symbolEnergy.begin(), std::max_element(symbolEnergy.begin(),symbolEnergy.end()));
				std::cout<<"sampleIndex = "<<sampleIndex<<std::endl;
				//size_t sampleIndex=0;
				//for (int k=0; k!=samples.size(); k++)
				//	std::cout<<samples[k]<<", ";
				//std::cout<<std::endl;

				//add the output sample to the vector
				out.push_back(*(samples.begin()+sampleIndex));

				//subtract the energy for this symbol from the symbolEnergy vector
				std::vector<double>::iterator symIter = symbolEnergy.begin();
				std::deque<double>::iterator energyIterEnd = energy.begin()+samplesPerSymbol;
				for (std::deque<double>::iterator energyIter = energy.begin(); energyIter !=energyIterEnd;energyIter++)
				{
					*symIter-=*energyIter;
					symIter++;
				}
				//remove all samples from this symbol from the samples & energy deques
				energy.erase(energy.begin(), energyIterEnd);
				samples.erase(samples.begin(), samples.begin()+samplesPerSymbol);
			}
			index=0; //reset our symbolIndex back to 0
		}
		//std::cout<<"ZZZZZZZ index = "<<index<<", samples.size() = "<<samples.size()<<std::endl;
	}
	std::cout<<"AAAAA = "<<out.size()<<std::endl;

	// NOTE: You must make at least one valid pushSRI call
	if (tmp->sriChanged) {
		dataFloat_out->pushSRI(tmp->SRI);
	}
	std::cout<<"out.size() = "<<out.size()<<std::endl;
	float phase=0;
	for (std::vector<std::complex<float> >::iterator i = out.begin(); i!=out.end(); i++)
	{
		phase += arg(pow((*i),4));
	}
	float phaseAvg = float(phase)/float(out.size());
	std::complex<float> phasor = std::polar(float(1.0),-phaseAvg+float(M_PI_4));
	for (std::vector<std::complex<float> >::iterator i = out.begin(); i!=out.end(); i++)
	{
		*i=(*i)*phasor;
	}

	if (!out.empty())
	{
		std::vector<float>* output = (std::vector<float>*)&out;
		dataFloat_out->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);
	}
	delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
	return NORMAL;
}

void psk_soft_i::samplesPerBaudChanged(const std::string& id){
   std::cout << "samplesPerBaudChanged " << samplesPerBaud<< std::endl;
   reset=(samplesPerBaud!=symbolEnergy.size());
}
