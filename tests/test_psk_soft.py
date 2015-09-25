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

DISPLAY=False
if DISPLAY:
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
    if DISPLAY:
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

   

    def testDiffDecodeBPSK(self):
        self.DiffDecodeTest(2)

    def testDiffDecodeQPSK(self):
        self.DiffDecodeTest(4)
        
    def testDiffDecode8PSK(self):
        self.DiffDecodeTest(8)

    def testNonDiffDecodeBPSK(self):
        self.NonDiffDecodeTest(2)

    def testNonDiffDecodeQPSK(self):
        self.NonDiffDecodeTest(4)
        
    def testNonDiffDecode8PSK(self):
        self.NonDiffDecodeTest(8)

    def DiffDecodeTest(self,numSyms):
        data, syms = genPsk(1000, sampPerBaud=8,numSyms=numSyms,differential=True)
        
        dataReal=[]
        
        #need to rotate my generated QPSK symbols from being at 0,pi/2,pi, and 3pi/2 to +/-1,+/-j
        if numSyms==4:
            theta = math.pi/4
            cxScaler= complex(math.cos(theta), math.sin(theta))
            symsRotated = [cxScaler*x for x in syms]
        else:
            symsRotated = syms
        
        self.comp.samplesPerBaud=8
        self.comp.constelationSize=numSyms
        self.comp.numAvg=100
        self.comp.differentialDecoding=True
        dataReal = toReal(data)
        out, bits, phase = self.main(dataReal,100)
        outCx = toCx(out)
        
        #don't include the first output as it is relative to an arbitrary reference
        #with the differential decoding when measuring the max error
        
        maxError = max([abs(x-y) for x, y in zip(outCx[1:],symsRotated[1:])])
        print "found max error of %s" %maxError
        assert(maxError < 1e-3)

    def NonDiffDecodeTest(self,numSyms):
        data, syms = genPsk(1000, sampPerBaud=8,numSyms=numSyms,differential=False)

        dataReal=[]
        
        self.comp.samplesPerBaud=8
        self.comp.constelationSize=numSyms
        self.comp.numAvg=100
        self.comp.differentialDecoding=False
        dataReal = toReal(data)
        out, bits, phase = self.main(dataReal,100)
        outCx = toCx(out)
        maxError = 1e99
        
        #there is an arbitrary phase offset for non-differentially decoded data
        #so we will check  different rotations to find which one is "correct" and use the 
        #error associated with it to ensure that the demod got the right symbols out
        if numSyms ==2:
            thetas = [0, math.pi]
        if numSyms ==4:
            thetas = [math.pi/4, 3*math.pi/4, 5*math.pi/4, 7*math.pi/4]
        if numSyms ==8:
            thetas = [0, math.pi/4, math.pi/2,3*math.pi/4,math.pi, 5*math.pi/4,3*math.pi/2, 7*math.pi/4]

        for theta in thetas:
            #don't include the first output as it is relative to an arbitrary reference
            #with the differential decoding
            cxScaler= complex(math.cos(theta), math.sin(theta))
            outCxRotated = [cxScaler*x for x in outCx]
            maxError = min(maxError, max([abs(x-y) for x, y in zip(outCxRotated[1:],syms[1:])]))
            
        print "found max error of %s" %maxError
        assert(maxError < 1e-3)        


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
            
    
if __name__ == "__main__":
    ossie.utils.testing.main("../psk_soft.spd.xml") # By default tests all implementations
