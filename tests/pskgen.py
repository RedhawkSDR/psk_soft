import math
try:
  import matplotlib.pyplot
  import scipy.fftpack
  enablePlotting = True
except:
  print "plotitng not enabled - missing packages"
  enablePlotting = False

def pskGen(symbols, baudRate, sampleRate, numSyms, fc, differential=False):
  syms = range(numSyms)
  twoPi = 2*math.pi
  phase = [twoPi*x/numSyms for x in syms]
  
  out=[]
  last = 0
  t = 0
  sampleTime = 1.0/sampleRate
  symbolTime = 1.0/baudRate
  tSym = symbolTime
  fNorm = twoPi*fc
  tNext = tSym
  for sym in symbols:
    ph = phase[sym]
    if differential:
      ph += last
      if ph >twoPi :
        ph-=twoPi
      last = ph
    
    while t < tNext:
      val = math.cos(fNorm*t+ph)
      out.append(val)
      t+=sampleTime
    tNext+=tSym
  return out        

def freqPlot(data,fs=1.0, fftNum=None):
    if enablePlotting:
        if not fftNum:
          fftNum = len(data)
        freqResponse = [abs(x) for x in scipy.fftpack.fftshift(scipy.fftpack.fft(data,fftNum))]
        freqAxis =  scipy.fftpack.fftshift(scipy.fftpack.fftfreq(fftNum,1.0/fs))
        matplotlib.pyplot.plot(freqAxis, freqResponse)
        matplotlib.pyplot.show()


def genData(filename,baudRate,sampleRate,totalTime,numSyms,complex=False):
    import random
    from ossie.utils import bluefile
    
    bfFormat = 'CF' if complex else 'SF'
    totalSymbols = int(totalTime*baudRate)
    symChoices = range(numSyms)
    symbols = [random.choice(symChoices) for _ in xrange(totalSymbols)]
    data = pskGen(symbols,baudRate, sampleRate, numSyms, sampleRate/4.0, False)
    #freqPlot(data,sampleRate,128*1024)  
    hdr = bluefile.header(format=bfFormat)
    hdr['xunits']=1
    hdr['xdelta']=1.0/sampleRate
    bluefile.write(filename, hdr, data)
    

def genTC1Data():
    filename = 'TFDoutput.tmp'
    numSyms = 2 # BPSK
    sampleRate = 100.0
    samplesPerBaud = 8
    baudRate = sampleRate/samplesPerBaud
    totalTime = 10.0
    genData(filename,baudRate,sampleRate,totalTime,numSyms,True)
    
    
def genTC2Data():
    filename = 'TFDoutput.tmp'
    numSyms = 2 # BPSK
    sampleRate = 100.0
    samplesPerBaud = 8
    baudRate = sampleRate/samplesPerBaud
    totalTime = 10.0
    genData(filename,baudRate,sampleRate,totalTime,numSyms)
    

if __name__=="__main__":
    genTC1Data()

