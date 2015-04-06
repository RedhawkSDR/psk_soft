#!/usr/bin/env python
#
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of REDHAWK psk_soft.
#
# REDHAWK psk_soft is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# REDHAWK psk_soft is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/.
#
import unittest
import ossie.utils.testing
import os
from omniORB import any
import time

from ossie.utils import sb
from ossie.utils.sb import domainless
import struct
import math
import random
import cmath

import matplotlib.pyplot

from ossie.utils import bluefile
from scipy import signal

random.seed(100)

def getDelay(first,second):
    numFirst = len(first)
    numSecond = len(second)
    numPts = min(numFirst,numSecond)
    c = signal.correlate(first[:numPts],second[:numPts],mode='full')
    cLen = len(c)
    matplotlib.pyplot.plot(xrange(cLen), c)
    matplotlib.pyplot.show()
    
    z = zip(c,range(cLen))
    z.sort()
    maxIndex =z[-1][1]
    delay = maxIndex+1-numPts
    return delay

def diffDecode(data):
    last=data[0]
    out=[]
    for v in data[1:]:
        d = abs(last - v)
        #print last, v, d
        out.append(d)
        last = v
    return out

                 
def toClipboard(data):
    import pygtk
    pygtk.require('2.0')
    import gtk

    # get the clipboard
    clipboard = gtk.clipboard_get()
    txt = str(data)
    clipboard.set_text(txt)

    # make our data available to other applications
    clipboard.store()


def toCx(input):
    output =[]
    for i in xrange(len(input)/2):
        output.append(complex(input[2*i], input[2*i+1]))
    return output 

def toReal(dataCx):
    output=[]
    for val in dataCx:
        output.append(val.real)
        output.append(val.imag)
    return output

def roundCx(val):
    return complex(round(val.real), round(val.imag))                      

def genPsk(numSymbols, sampPerBaud=8,numSyms=4,differential=False):
    syms = range(numSyms)
    phase = [2*math.pi*x/numSyms for x in syms]
    cx = [complex(math.cos(x),math.sin(x)) for x in phase]
    out=[]
    inputSymbols=[]
    last=1
    for i in xrange(numSymbols):
        x = random.choice(syms)
        x_cx = cx[x]
        inputSymbols.append(x_cx)
        if differential:
            val = x_cx*last
            last = val
        else:
            val = x_cx

        for j in xrange(sampPerBaud):
              out.append(val + .0001*random.random()) #add a little noise in make the plot happy 
    return out, inputSymbols

class ComponentTests(ossie.utils.testing.ScaComponentTestCase):
    """Test for all component implementations in psk_soft"""

    def setUp(self):
        """Set up the unit test - this is run before every method that starts with test
        """
        ossie.utils.testing.ScaComponentTestCase.setUp(self)
        self.src = sb.DataSource()
        self.soft = sb.DataSink()
        self.bits = sb.DataSink()
        self.phase = sb.DataSink()
        
        #setup my components
        self.setupComponent()
        
        self.comp.start()
        self.src.start()
        self.soft.start()
        self.bits.start()
        
        #do the connections
        self.src.connect(self.comp)        
        self.comp.connect(self.soft,usesPortName='dataFloat_out')
        self.comp.connect(self.bits,usesPortName='dataShort_out')
        self.comp.connect(self.phase,usesPortName='phase_out')
        
    def tearDown(self):
        """Finish the unit test - this is run after every method that starts with test
        """
        self.comp.stop()
        #######################################################################
        # Simulate regular component shutdown
        self.src.stop()
        self.soft.stop()
        self.bits.stop()
        self.phase.stop()            
        self.comp.releaseObject()
        ossie.utils.testing.ScaComponentTestCase.tearDown(self)

    def testCase(self):
        #f = file('/home/bsg/qpsk.dat','r')
        #f = file('/home/bsg/out2','r')
        #try:
        #    s = f.read()
        #except:
        #    print "cannot read test file"
        #    return
        #data = list(struct.unpack('%sf' %(len(s)/4),s))

        h, d = bluefile.read('/nishome/bsgrego/TFDoutput.tmp')
        data=[]
        for v in d:
            data.append(float(v.real))
            data.append(float(v.imag))

        print "got %s data samples" %len(data)

        self.comp.samplesPerBaud=8
        self.comp.constelationSize=2
        print self.comp.samplesPerBaud   
        self.comp.numAvg=100   
        out = self.main(data,100)[0]
        print len(out)
         
        #         matplotlib.pyplot.plot(out[::2], out[1::2],'o')
        #         matplotlib.pyplot.show()
        
        if out:
            startSample=0
            endSample=20
            resampleFactor=1
            startIndex=0
            finalIndex=2000
            x = out[2*startIndex:finalIndex:resampleFactor*2]
            y = out[2*startIndex+1:finalIndex:resampleFactor*2]
            cx = [complex(i,j) for i,j in zip(x,y)]
            
            matplotlib.pyplot.plot(x, y,'o')
            matplotlib.pyplot.show()


#        for startIndex in xrange(10):
        if False:
            startIndex=5
            print startIndex
            resampleFactor = 8
            finalIndex = 251*resampleFactor
            x = data[2*startIndex:finalIndex:resampleFactor*2]
            y = data[2*startIndex+1:finalIndex:resampleFactor*2]
            #print x
            #print y
            #print data[:10]
            matplotlib.pyplot.plot(x,y,'o')
            matplotlib.pyplot.show()
            #matplotlib.pyplot.plot(xrange(len(x)),x,'o')
            #matplotlib.pyplot.show()

    def testDiffDecode(self):
        data, syms = genPsk(1000, sampPerBaud=8,numSyms=4,differential=True)
        
        print len(data), len(syms)
        dataReal=[]
        
        #need to rotate my generated qpsk symbols from being at 0,pi/2,pi, and 3pi/2 to +/-1,+/-j
        theta = math.pi/4
        cxScaler= complex(math.cos(theta), math.sin(theta))
        symsRotated = [cxScaler*x for x in syms]
        
        self.comp.samplesPerBaud=8
        self.comp.constelationSize=4
        self.comp.numAvg=100
        self.comp.differentialDecoding=True
        dataReal = toReal(data)
        out, bits, phase = self.main(dataReal,100)
        outCx = toCx(out)
        print len(out), len(bits), len(phase)
        
        #don't include the first output as it is relative to an arbitrary reference
        #with the differential decoding when measuring the max error
        maxError = max([abs(x-y) for x, y in zip(outCx[1:],symsRotated[1:])])
        print "found max error of %s" %maxError
        assert(maxError < 1e-3)

    def testDiffDecode2(self):
#         data, syms = genPsk(1000, sampPerBaud=8,numSyms=2,differential=False)
#         dataReal = toReal(data)
#           
#         dataStr = struct.pack('%sf' %len(dataReal),*dataReal)
#         myFile = file('test_data','w')
#         myFile.write(dataStr)
#         myFile.close()
#            
#         symsReal = [x.real for x in syms]
#            
#         dataStr = struct.pack('%sf' %len(symsReal),*symsReal)
#         myFile = file('test_syms','w')
#         myFile.write(dataStr)
#         myFile.close()
        
        f = file('test_data','r')
        s = f.read()
        dataReal = list(struct.unpack('%sf' %(len(s)/4),s))
  
        f = file('test_syms','r')
        s = f.read()
        symsReal = list(struct.unpack('%sf' %(len(s)/4),s))
         
        f = file('test_bits','r')
        s = f.read()
        myDiffDecoded = list(struct.unpack('%sh' %(len(s)/2),s))
        
        
        #print len(data), len(syms)
        self.comp.samplesPerBaud=8
        self.comp.constelationSize=2
        self.comp.numAvg=100
        self.comp.differentialDecoding=True
        
        out, bits, phase = self.main(dataReal,100)
        outCx = toCx(out)
        #print len(out), len(bits), len(phase), len(syms)
        
        dataStr = struct.pack('%sh' %len(bits),*bits)
        myFile = file('test_bits_dc','w')
        myFile.write(dataStr)
        myFile.close()
        
        
        symsRealBits = [(x+1)/2.0 for x in symsReal]
        #symsRealBits = [abs((x-1)/2.0) for x in symsReal]
        
        bits = diffDecode(bits)
        #print bits[:10]
        
#         #delay = getDelay(bits, myDiffDecoded)
#         delay = -1
#         print "delay = ",  delay
#         
#         if delay >=0:
#             bitsDel = bits[delay:]
#             myDiffDel = myDiffDecoded
#         else:
#             bitsDel = bits
#             myDiffDel = myDiffDecoded[abs(delay):]
#         
#         print bitsDel[:10]
#         print myDiffDel[:10]
#             
#         d = [abs(x-y) for x,y in zip(bitsDel, myDiffDel)]
#         print "BER = ",  sum(d)/len(d)
#         
#         matplotlib.pyplot.plot(range(len(d)), d)
#         matplotlib.pyplot.show()
        
        
        #don't include the first output as it is relative to an arbitrary reference
        #with the differential decoding when measuring the max error
        #print [x-y for x,y in zip(bits,syms[1:])]
        #print "found max error of %s" %maxError
        #assert(maxError < 1e-3)


    def testRegular(self):
        data, syms = genPsk(1000, sampPerBaud=8,numSyms=4,differential=False)
        
        print len(data), len(syms)
        dataReal=[]
        
        self.comp.samplesPerBaud=8
        self.comp.constelationSize=4
        self.comp.numAvg=100
        self.comp.differentialDecoding=False
        dataReal = toReal(data)
        out, bits, phase = self.main(dataReal,100)
        outCx = toCx(out)
        print len(out), len(bits), len(phase)
        maxError = 1e99
        #there is an arbitrary phase offset for non-differentailly decoded data
        #so we will check 4 different rotations to find which one is "correct" and use the 
        #errror associated with it to ensure that the demod got the right symbols out
        #we use pi/4 +n*pi/2 because the generator has points at +/1,0, etc but the output points are at
        #(+/1,+/-1)
        for theta in [math.pi/4, 3*math.pi/4, 5*math.pi/4, 7*math.pi/4]:
            #don't include the first output as it is relative to an arbitrary reference
            #with the differential decoding
            cxScaler= complex(math.cos(theta), math.sin(theta))
            print cxScaler
            outCxRotated = [cxScaler*x for x in outCx]
            maxError = min(maxError, max([abs(x-y) for x, y in zip(outCxRotated[1:],syms[1:])]))
            print "found max error of %s" %maxError
        assert(maxError < 1e-3)        

    def test2Case(self):
        #f = file('/home/bsg/qpsk.dat','r')
        f = file('/home/bsg/psk8.dat','r')
        try:
            s = f.read(100000)
        except:
            print "cannot read test file"
            return
        data = list(struct.unpack('%sf' %(len(s)/4),s))

        print "got %s data samples" %len(data)
        
        theta = .002
        #introduce a small frequency offset
        cxData = toCx(data)
        shiftedData = [x*cmath.rect(1,i*theta) for x, i in zip(cxData,xrange(len(cxData)))]
        data  = []
        for x in shiftedData:
            data.append(x.real)
            data.append(x.imag)

        self.comp.samplesPerBaud=8
        self.comp.constelationSize=8
        self.comp.phaseAvg = 70
        print self.comp.samplesPerBaud, self.comp.samplesPerBaud   
        self.comp.numAvg=100   
        out, bits, phase = self.main(data,100)
        print "out = ", len(out)
        if out:
            toClipboard(bits)
            print len(out), len(bits)
             
            #         matplotlib.pyplot.plot(out[::2], out[1::2],'o')
            #         matplotlib.pyplot.show()
            
            startSample=0
            endSample=20
            resampleFactor=1
            startIndex=0
            finalIndex=2000
            x = out[2*startIndex:finalIndex:resampleFactor*2]
            y = out[2*startIndex+1:finalIndex:resampleFactor*2]
            cx = [complex(i,j) for i,j in zip(x,y)]
            
            matplotlib.pyplot.plot(x, y,'o')
            matplotlib.pyplot.show()
            power = [pow(z,8) for z in cx]
            
            matplotlib.pyplot.plot(range(len(phase)), phase)
            matplotlib.pyplot.show()
            
            matplotlib.pyplot.plot(range(len(power)),[cmath.phase(z) for z in power] ,'o')
            matplotlib.pyplot.show()
            avgPhase = sum([cmath.phase(z) for z in power])/len(power)


#        for startIndex in xrange(10):
        if False:
            startIndex=5
            print startIndex
            resampleFactor = 8
            finalIndex = 251*resampleFactor
            x = data[2*startIndex:finalIndex:resampleFactor*2]
            y = data[2*startIndex+1:finalIndex:resampleFactor*2]
            #print x
            #print y
            #print data[:10]
            matplotlib.pyplot.plot(x,y,'o')
            matplotlib.pyplot.show()
            #matplotlib.pyplot.plot(xrange(len(x)),x,'o')
            #matplotlib.pyplot.show()



    def main(self,inData, sampleRate, complexData = True):
        """The main engine for all the test cases - configure the equation, push data, and get output
           As applicable
        """
        #data processing is asynchronos - so wait until the data is all processed
        count=0
        self.src.push(inData,
                      complexData = complexData, 
                      sampleRate=sampleRate)
        out=[]
        bits=[]
        phase=[]
        while True:
            newOut = self.soft.getData()
            newBits = self.bits.getData()
            newPhase = self.phase.getData()
            if newOut or newBits:
                if newOut:
                    out.extend(newOut)
                if newBits:
                    bits.extend(newBits)
                if newPhase:
                    phase.extend(newPhase)
                count=0
            elif count==100:
                break
            time.sleep(.01)
            count+=1
        return out, bits, phase

    def setupComponent(self):
        #######################################################################
        # Launch the component with the default execparams
        execparams = self.getPropertySet(kinds=("execparam",), modes=("readwrite", "writeonly"), includeNil=False)
        execparams = dict([(x.id, any.from_any(x.value)) for x in execparams])
        self.launch(execparams)
        
        #######################################################################
        # Verify the basic state of the component
        self.assertNotEqual(self.comp, None)
        self.assertEqual(self.comp.ref._non_existent(), False)
        self.assertEqual(self.comp.ref._is_a("IDL:CF/Resource:1.0"), True)
        
        #######################################################################
        # Validate that query returns all expected parameters
        # Query of '[]' should return the following set of properties
        expectedProps = []
        expectedProps.extend(self.getPropertySet(kinds=("configure", "execparam"), modes=("readwrite", "readonly"), includeNil=True))
        expectedProps.extend(self.getPropertySet(kinds=("allocate",), action="external", includeNil=True))
        props = self.comp.query([])
        props = dict((x.id, any.from_any(x.value)) for x in props)
        # Query may return more than expected, but not less
        for expectedProp in expectedProps:
            self.assertEquals(props.has_key(expectedProp.id), True)
        
        #######################################################################
        # Verify that all expected ports are available
        for port in self.scd.get_componentfeatures().get_ports().get_uses():
            port_obj = self.comp.getPort(str(port.get_usesname()))
            self.assertNotEqual(port_obj, None)
            self.assertEqual(port_obj._non_existent(), False)
            self.assertEqual(port_obj._is_a("IDL:CF/Port:1.0"),  True)
            
        for port in self.scd.get_componentfeatures().get_ports().get_provides():
            port_obj = self.comp.getPort(str(port.get_providesname()))
            self.assertNotEqual(port_obj, None)
            self.assertEqual(port_obj._non_existent(), False)
            self.assertEqual(port_obj._is_a(port.get_repid()),  True)
            
        
    # TODO Add additional tests here
    #
    # See:
    #   ossie.utils.bulkio.bulkio_helpers,
    #   ossie.utils.bluefile.bluefile_helpers
    # for modules that will assist with testing components with BULKIO ports
    
if __name__ == "__main__":
    ossie.utils.testing.main("../psk_soft.spd.xml") # By default tests all implementations
