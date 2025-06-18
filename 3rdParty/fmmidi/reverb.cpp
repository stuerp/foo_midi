#include <math.h>

#include "lanczos_resampler.h"

#include "reverb.hpp"

const ReverbPreset defaults[9] = {

    { "Room",
		{  4960,
		  32767, 32767, 12800, 12800,
		   0x7D,  0x5B, 28032, 21688, -16688,     0,      0, -17792,
		  22528, 21248, 0x4D6, 0x333,  0x3f0, 0x227,  0x374,  0x1EF,
		  0x334, 0x1B5,     0,     0,      0,     0,      0,      0,
		      0,     0, 0x1B4, 0x136,   0xB8,  0x5C, -32768, -32768 }
	},

    { "Studio Small",
		{ 4000,
		 32767, 32767, 12800, 12800,
		  0x33,  0x25, 28912, 20392, -17184, 17424, -16144, -25600,
		 21120, 20160, 0x3E4, 0x31B,  0x3A4, 0x2AF,  0x372,  0x266,
		 0x31C, 0x25D, 0x25C, 0x18E,  0x22F, 0x135,  0x1D2,   0xB7,
		 0x18F,  0xB5,  0xB4,  0x80,   0x4C,  0x26, -32768, -32768 }
	},

    { "Studio Medium",
		{ 9248,
		 32767, 32767, 12800, 12800,
		  0xB1,  0x7F, 28912, 20392, -17184, 17680, -16656, -19264,
		 21120, 20160, 0x904, 0x76B,  0x824, 0x65F,  0x7A2,  0x616,
		 0x76C, 0x5ED, 0x5EC, 0x42E,  0x50F, 0x305,  0x462,  0x2B7,
		 0x42F, 0x265, 0x264, 0x1B2,  0x100,  0x80, -32768, -32768 }
	},

    { "Studio Large",
		{ 14320,
		  32767, 32767, 12800, 12800,
		   0xE3,  0xA9, 28512, 20392, -17184, 17680, -16656, -22912,
		  22144, 21184, 0xDFB, 0xB58,  0xD09, 0xA3C,  0xBD9,  0x973,
		  0xB59, 0x8DA, 0x8D9, 0x5E9,  0x7EC, 0x4B0,  0x6EF,  0x3D2,
		  0x5EA, 0x31D, 0x31C, 0x238,  0x154,  0xAA, -32768, -32768 }
	},

    { "Hall",
		{  22256,
		   32767, 32767,  12800,  12800,
		   0x1A5, 0x139,  24576,  20480,  19456, -18432, -17408, -16384,
		   24576, 23552, 0x15BA, 0x11BB, 0x14C2, 0x10BD, 0x11BC,  0xDC1,
		  0x11C0, 0xDC3,  0xDC0,  0x9C1,  0xBC4,  0x7C1,  0xA00,  0x6CD,
		   0x9C2, 0x5C1,  0x5C0,  0x41A,  0x274,  0x13A, -32768, -32768 }
	},

    { "Space Echo",
		{  31584,
		   32767,  32767,  12800,  12800,
		   0x33D,  0x231,  32256,  20480, -19456, -20480,  19456, -20480,
		   24576,  21504, 0x1ED6, 0x1A31, 0x1D14, 0x183B, 0x1BC2, 0x16B2,
		  0x1A32, 0x15EF, 0x15EE, 0x1055, 0x1334,  0xF2D, 0x11F6,  0xC5D,
		  0x1056,  0xAE1,  0xAE0,  0x7A2,  0x464,  0x232, -32768, -32768 }
	},

    { "Echo",
		{ 49184,
		  32767,  32767, 12800,   12800,
		      1,      1,  32767,  32767,      0, 0,      0, -32512,
			  0,      0, 0x1FFF,  0xFFF, 0x1005, 5,      0,      0,
		 0x1005,      5,      0,      0,      0, 0,      0,      0,
		      0,      0, 0x1004, 0x1002,      4, 2, -32768, -32768 }
	},

    { "Delay",
		{ 49184,
		  32767,  32767,  12800,  12800,
		      1,      1,  32767,  32767,      0, 0,      0,      0,
			  0,      0, 0x1FFF,  0xFFF, 0x1005, 5,      0,      0,
		 0x1005,      5,      0,      0,      0, 0,      0,      0,
		      0,      0, 0x1004, 0x1002,      4, 2, -32768, -32768 }
	},

    { "Half Echo",
		{  7680,
		  32767, 32767, 12800, 12800,
		   0x17,  0x13, 28912, 20392, -17184, 17680, -16656, -31488,
		  24448, 21696, 0x371, 0x2AF,  0x2E5, 0x1DF,  0x2B0,  0x1D7,
		  0x358, 0x26A, 0x1D6, 0x11E,  0x12D,  0xB1,  0x11F,   0x59,
		  0x1A0,  0xE3,  0x58,  0x40,   0x28,  0x14, -32768, -32768 }
	}
};

float dsp_reverb::MixREVERBLeft( float INPUT_SAMPLE_L, float INPUT_SAMPLE_R, float *ptr )
{
	float ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

	const float IIR_INPUT_A0 = ( g_buffer( IIR_SRC_A0, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_L * IN_COEF_L );
	const float IIR_INPUT_A1 = ( g_buffer( IIR_SRC_A1, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_R * IN_COEF_R );
	const float IIR_INPUT_B0 = ( g_buffer( IIR_SRC_B0, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_L * IN_COEF_L );
	const float IIR_INPUT_B1 = ( g_buffer( IIR_SRC_B1, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_R * IN_COEF_R );

	const float IIR_A0 = ( IIR_INPUT_A0 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_A0, ptr ) * ( 1.f - IIR_ALPHA ) );
	const float IIR_A1 = ( IIR_INPUT_A1 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_A1, ptr ) * ( 1.f - IIR_ALPHA ) );
	const float IIR_B0 = ( IIR_INPUT_B0 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_B0, ptr ) * ( 1.f - IIR_ALPHA ) );
	const float IIR_B1 = ( IIR_INPUT_B1 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_B1, ptr ) * ( 1.f - IIR_ALPHA ) );

	s_buffer1( IIR_DEST_A0, IIR_A0, ptr );
	s_buffer1( IIR_DEST_A1, IIR_A1, ptr );
	s_buffer1( IIR_DEST_B0, IIR_B0, ptr );
	s_buffer1( IIR_DEST_B1, IIR_B1, ptr );

	ACC0 = ( g_buffer( ACC_SRC_A0, ptr ) * ACC_COEF_A ) +
		( g_buffer( ACC_SRC_B0, ptr ) * ACC_COEF_B ) +
		( g_buffer( ACC_SRC_C0, ptr ) * ACC_COEF_C ) +
		( g_buffer( ACC_SRC_D0, ptr ) * ACC_COEF_D );
	ACC1 = ( g_buffer( ACC_SRC_A1, ptr ) * ACC_COEF_A ) +
		( g_buffer( ACC_SRC_B1, ptr ) * ACC_COEF_B ) +
		( g_buffer( ACC_SRC_C1, ptr ) * ACC_COEF_C ) +
		( g_buffer( ACC_SRC_D1, ptr ) * ACC_COEF_D );

	FB_A0 = g_buffer( MIX_DEST_A0 - FB_SRC_A, ptr );
	FB_A1 = g_buffer( MIX_DEST_A1 - FB_SRC_A, ptr );
	FB_B0 = g_buffer( MIX_DEST_B0 - FB_SRC_B, ptr );
	FB_B1 = g_buffer( MIX_DEST_B1 - FB_SRC_B, ptr );

	s_buffer( MIX_DEST_A0, ACC0 - ( FB_A0 * FB_ALPHA ), ptr );
	s_buffer( MIX_DEST_A1, ACC1 - ( FB_A1 * FB_ALPHA ), ptr );

	float fb_alpha_sign = ( FB_ALPHA > 0 ) ? 1.f : -1.f;
	float fb_alpha_reversed = ( fabs( FB_ALPHA ) - 1.f ) * fb_alpha_sign;

	s_buffer( MIX_DEST_B0, ( FB_ALPHA * ACC0 ) - ( FB_A0 * fb_alpha_reversed ) - ( FB_B0 * FB_X ), ptr );
	s_buffer( MIX_DEST_B1, ( FB_ALPHA * ACC1 ) - ( FB_A1 * fb_alpha_reversed ) - ( FB_B1 * FB_X ), ptr );

	iRVBLeft  = ( g_buffer( MIX_DEST_A0, ptr ) + g_buffer( MIX_DEST_B0, ptr ) ) / 3.f;
	iRVBRight = ( g_buffer( MIX_DEST_A1, ptr ) + g_buffer( MIX_DEST_B1, ptr ) ) / 3.f;

	iRVBLeft  = ( iRVBLeft  * VolLeft );
	iRVBRight = ( iRVBRight * VolRight );

	CurrAddr++;
	if ( CurrAddr >= delay ) CurrAddr = 0;

    return iRVBLeft;
}

dsp_reverb::dsp_reverb( ReverbConfig const & in ) : r( in )
{
    resampler_in[0] = midi_lanczos_resampler_create();
    resampler_in[1] = midi_lanczos_resampler_create();
    resampler_out[0] = midi_lanczos_resampler_create();
    resampler_out[1] = midi_lanczos_resampler_create();

    while (!midi_lanczos_resampler_ready(resampler_in[0]))
    {
        midi_lanczos_resampler_write_sample(resampler_in[0], 0.0f);
        midi_lanczos_resampler_write_sample(resampler_in[1], 0.0f);
        midi_lanczos_resampler_write_sample(resampler_out[0], 0.0f);
        midi_lanczos_resampler_write_sample(resampler_out[0], 0.0f);
    }

#define cfgf(a) a = cnv_float(r.a)
#define cfgo(a) a = cnv_offset(r.a)

    {
        cfgo(delay);
        cfgf(mVolLeft);
        cfgf(mVolRight);
        cfgf(VolLeft);
        cfgf(VolRight);

        cfgo(FB_SRC_A);
        cfgo(FB_SRC_B);
        cfgf(IIR_ALPHA);
        cfgf(ACC_COEF_A);
        cfgf(ACC_COEF_B);
        cfgf(ACC_COEF_C);
        cfgf(ACC_COEF_D);
        cfgf(IIR_COEF);
        cfgf(FB_ALPHA);
        cfgf(FB_X);
        cfgo(IIR_DEST_A0);
        cfgo(IIR_DEST_A1);
        cfgo(ACC_SRC_A0);
        cfgo(ACC_SRC_A1);
        cfgo(ACC_SRC_B0);
        cfgo(ACC_SRC_B1);
        cfgo(IIR_SRC_A0);
        cfgo(IIR_SRC_A1);
        cfgo(IIR_DEST_B0);
        cfgo(IIR_DEST_B1);
        cfgo(ACC_SRC_C0);
        cfgo(ACC_SRC_C1);
        cfgo(ACC_SRC_D0);
        cfgo(ACC_SRC_D1);
        cfgo(IIR_SRC_B1);
        cfgo(IIR_SRC_B0);
        cfgo(MIX_DEST_A0);
        cfgo(MIX_DEST_A1);
        cfgo(MIX_DEST_B0);
        cfgo(MIX_DEST_B1);
        cfgf(IN_COEF_L);
        cfgf(IN_COEF_R);
    }
#undef cfgf
#undef cfgo

    CurrAddr = 0;

    reverb.resize(delay);
}

dsp_reverb::~dsp_reverb()
{
    midi_lanczos_resampler_delete(resampler_in[0]);
    midi_lanczos_resampler_delete(resampler_in[1]);
    midi_lanczos_resampler_delete(resampler_out[0]);
    midi_lanczos_resampler_delete(resampler_out[1]);
}

void dsp_reverb::on_sample( float srate, float & left, float & right )
{
    midi_lanczos_resampler_write_sample(resampler_in[0], left);
    midi_lanczos_resampler_write_sample(resampler_in[1], right);

	left = 0.0f;
	right = 0.0f;

    if (!midi_lanczos_resampler_ready(resampler_in[0]) &&
        !midi_lanczos_resampler_get_sample_count(resampler_in[0])) return;

    midi_lanczos_resampler_set_rate(resampler_in[0], srate / 22050.0f);
    midi_lanczos_resampler_set_rate(resampler_in[1], srate / 22050.0f);

    while (!midi_lanczos_resampler_get_sample_count(resampler_out[0]) &&
           (midi_lanczos_resampler_ready(resampler_in[0]) ||
           midi_lanczos_resampler_get_sample_count(resampler_in[0]) > 0))
    {
        midi_lanczos_resampler_write_sample(resampler_out[0], MixREVERBLeft(midi_lanczos_resampler_get_sample(resampler_in[0]),
                midi_lanczos_resampler_get_sample(resampler_in[1]), &reverb[0]));
        midi_lanczos_resampler_write_sample(resampler_out[1], MixREVERBRight());
        midi_lanczos_resampler_remove_sample(resampler_in[0]);
        midi_lanczos_resampler_remove_sample(resampler_in[1]);
    }

    if (!midi_lanczos_resampler_ready(resampler_out[0]) &&
        !midi_lanczos_resampler_get_sample_count(resampler_out[0])) return;

    midi_lanczos_resampler_set_rate(resampler_out[0], 22050.0f / srate);
    midi_lanczos_resampler_set_rate(resampler_out[1], 22050.0f / srate);

    left = midi_lanczos_resampler_get_sample(resampler_out[0]);
    right = midi_lanczos_resampler_get_sample(resampler_out[1]);

    midi_lanczos_resampler_remove_sample(resampler_out[0]);
    midi_lanczos_resampler_remove_sample(resampler_out[1]);
}
