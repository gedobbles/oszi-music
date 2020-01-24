#include "SC_PlugIn.h"
#include "waves.h"

// InterfaceTable contains pointers to functions in the host (server).
static InterfaceTable *ft;

// declare struct to hold unit generator state
struct MyOsc : public Unit
{
    double mPhase; // phase of the oscillator, from -1 to 1.
    float mFreqMul; // a constant for multiplying frequency
    int mTsel;
    double* mTable;
};

// declare unit generator functions
static void MyOsc_next_a(MyOsc *unit, int inNumSamples);
static void MyOsc_next_k(MyOsc *unit, int inNumSamples);
static void MyOsc_Ctor(MyOsc* unit);
static void MyOsc_Dtor(MyOsc* unit);
static void MyOsc_fillTable(int mTsel, double table[]);


//////////////////////////////////////////////////////////////////

// Ctor is called to initialize the unit generator.
// It only executes once.

// A Ctor usually does 3 things.
// 1. set the calculation function.
// 2. initialize the unit generator state variables.
// 3. calculate one sample of output.
void MyOsc_Ctor(MyOsc* unit)
{
    // 1. set the calculation function.
    if (INRATE(0) == calc_FullRate) {
        // if the frequency argument is audio rate
        SETCALC(MyOsc_next_a);
    } else {
        // if the frequency argument is control rate (or a scalar).
        SETCALC(MyOsc_next_k);
    }

    // 2. initialize the unit generator state variables.
    // initialize a constant for multiplying the frequency
    unit->mFreqMul = 2.0 * SAMPLEDUR;
    // get initial phase of oscillator
    unit->mPhase = IN0(1);
    unit->mTsel = (int) IN0(2);
    unit->mTable = (double*)RTAlloc(unit->mWorld, 64 * sizeof(double));
    MyOsc_fillTable(unit->mTsel, unit->mTable);
    // 3. calculate one sample of output.
    MyOsc_next_k(unit, 1);
}

void MyOsc_Dtor(MyOsc* unit) {
    // Free the memory.
    RTFree(unit->mWorld, unit->mTable);
}


void MyOsc_fillTable(int mTsel, double table[])
{
    switch(mTsel)
    {
      case 0:
        tBaumL(table);
      break;
      case 1:
        tBaumR(table);
      break;
    }
}

//////////////////////////////////////////////////////////////////

// The calculation function executes once per control period
// which is typically 64 samples.

// calculation function for an audio rate frequency argument
void MyOsc_next_a(MyOsc *unit, int inNumSamples)
{
    // get the pointer to the output buffer
    float *out = OUT(0);

    // get the pointer to the input buffer
    float *freq = IN(0);

    // get phase and freqmul constant from struct and store it in a
    // local variable.
    // The optimizer will cause them to be loaded it into a register.
    float freqmul = unit->mFreqMul;
    double phase = unit->mPhase;
    double* table = unit->mTable;
    int pf, pc;
    double dp, phaseN;
    float z;

    // perform a loop for the number of samples in the control period.
    // If this unit is audio rate then inNumSamples will be 64 or whatever
    // the block size is. If this unit is control rate then inNumSamples will
    // be 1.
    for (int i=0; i < inNumSamples; ++i)
    {
        // out must be written last for in place operation
        phaseN = (phase * 32.0) + 32;
        pf = floor(phaseN);
        pc = ceil(phaseN);
        if(pc >= 63)
          pc = 0;
        dp = phaseN - pf;
        z = table[pf] + (table[pc] - table[pf]) * dp;
        phase += freq[i] * freqmul;

        // these if statements wrap the phase a +1 or -1.
        if (phase >= 1.f) phase -= 2.f;
        else if (phase <= -1.f) phase += 2.f;

        // write the output
        out[i] = z;
    }

    // store the phase back to the struct
    unit->mPhase = phase;
}

//////////////////////////////////////////////////////////////////

// calculation function for a control rate frequency argument
void MyOsc_next_k(MyOsc *unit, int inNumSamples)
{
    // get the pointer to the output buffer
    float *out = OUT(0);

    // freq is control rate, so calculate it once.
    float freq = IN0(0) * unit->mFreqMul;

    // get phase from struct and store it in a local variable.
    // The optimizer will cause it to be loaded it into a register.
    double phase = unit->mPhase;
    double* table = unit->mTable;
    int pf, pc;
    double dp, phaseN;
    float z;

    // since the frequency is not changing then we can simplify the loops
    // by separating the cases of positive or negative frequencies.
    // This will make them run faster because there is less code inside the loop.
    if (freq >= 0.f) {
        // positive frequencies
        for (int i=0; i < inNumSamples; ++i)
        {
            phaseN = (phase * 32.0) + 32;
            pf = floor(phaseN);
            pc = ceil(phaseN);
            if(pc >= 63)
              pc = 0;
            dp = phaseN - pf;
            z = table[pf] + (table[pc] - table[pf]) * dp;
            out[i] = z;
            phase += freq;
            if (phase >= 1.f) phase -= 2.f;
        }
    } else {
        // negative frequencies
        for (int i=0; i < inNumSamples; ++i)
        {
            phaseN = (phase * 32.0) + 32;
            pf = floor(phaseN);
            pc = ceil(phaseN);
            if(pc >= 63)
              pc = 0;
            dp = phaseN - pf;
            z = table[pf] + (table[pc] - table[pf]) * dp;
            out[i] = z;
            phase += freq;
            if (phase <= -1.f) phase += 2.f;
        }
    }

    // store the phase back to the struct
    unit->mPhase = phase;
}


// the entry point is called by the host when the plug-in is loaded
PluginLoad(MyOsc)
{
    // InterfaceTable *inTable implicitly given as argument to the load function
    ft = inTable; // store pointer to InterfaceTable

    DefineSimpleUnit(MyOsc);
}
