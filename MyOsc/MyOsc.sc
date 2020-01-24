// without mul and add.
MyOsc : UGen {
    *ar { arg freq = 440.0, iphase = 0.0, mTsel = 0;
        ^this.multiNew('audio', freq, iphase, mTsel)
    }
    *kr { arg freq = 440.0, iphase = 0.0, mTsel = 0;
        ^this.multiNew('control', freq, iphase, mTsel)
    }
}
