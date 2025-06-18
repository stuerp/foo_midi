#ifndef _REVERB_HPP_
#define _REVERB_HPP_

#include <vector>

struct ReverbConfig
{
	unsigned delay;

	int      mVolLeft;
	int      mVolRight;

	int      VolLeft;
	int      VolRight;

	int      FB_SRC_A;
	int      FB_SRC_B;

	int      IIR_ALPHA;
	int      ACC_COEF_A;
	int      ACC_COEF_B;
	int      ACC_COEF_C;
	int      ACC_COEF_D;
	int      IIR_COEF;
	int      FB_ALPHA;
	int      FB_X;

	int      IIR_DEST_A0;
	int      IIR_DEST_A1;
	int      ACC_SRC_A0;
	int      ACC_SRC_A1;
	int      ACC_SRC_B0;
	int      ACC_SRC_B1;
	int      IIR_SRC_A0;
	int      IIR_SRC_A1;
	int      IIR_DEST_B0;
	int      IIR_DEST_B1;
	int      ACC_SRC_C0;
	int      ACC_SRC_C1;
	int      ACC_SRC_D0;
	int      ACC_SRC_D1;
	int      IIR_SRC_B1;
	int      IIR_SRC_B0;
	int      MIX_DEST_A0;
	int      MIX_DEST_A1;
	int      MIX_DEST_B0;
	int      MIX_DEST_B1;

	int      IN_COEF_L;
	int      IN_COEF_R;
};

struct ReverbPreset
{
    const char * name;
	ReverbConfig config;
};

extern const ReverbPreset defaults[9];

class dsp_reverb
{
protected:
    std::vector<float> reverb;

	ReverbConfig r;

	int delay, CurrAddr;

	int FB_SRC_A, FB_SRC_B, IIR_DEST_A0, IIR_DEST_A1, ACC_SRC_A0, ACC_SRC_A1, ACC_SRC_B0,
		ACC_SRC_B1, IIR_SRC_A0, IIR_SRC_A1, IIR_DEST_B0, IIR_DEST_B1, ACC_SRC_C0,
		ACC_SRC_C1, ACC_SRC_D0, ACC_SRC_D1, IIR_SRC_B1, IIR_SRC_B0, MIX_DEST_A0,
		MIX_DEST_A1, MIX_DEST_B0, MIX_DEST_B1;

	float IIR_ALPHA, ACC_COEF_A, ACC_COEF_B, ACC_COEF_C, ACC_COEF_D, IIR_COEF, FB_ALPHA, FB_X,
		IN_COEF_L, IN_COEF_R;

	float iRVBLeft, iRVBRight, VolLeft, VolRight, mVolLeft, mVolRight;

    void * resampler_in[2];
    void * resampler_out[2];

    inline float g_buffer( int iOff, float *ptr )                          // get_buffer content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		return ( float ) *( ptr + iOff );
	}

	inline void s_buffer( int iOff, float iVal, float *ptr )                // set_buffer content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

	inline void s_buffer1( int iOff, float iVal, float *ptr )                // set_buffer (+1 sample) content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr + 1;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

    float MixREVERBLeft( float INPUT_SAMPLE_L, float INPUT_SAMPLE_R, float *ptr );
    inline float MixREVERBRight() const { return iRVBRight; }

    inline int cnv_offset( int in ) const { return in; }
    inline float cnv_float( int in ) const { return ( float ) in * ( 1.f / 32768.f ); }

public:
    dsp_reverb( ReverbConfig const & in );
    ~dsp_reverb();

    void on_sample( float srate, float & left, float & right );
};

#endif
