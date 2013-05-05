#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "lanczos_resampler.h"

enum { LANCZOS_RESOLUTION = 8192 };
enum { LANCZOS_WIDTH = 8 };
enum { LANCZOS_SAMPLES = LANCZOS_RESOLUTION * LANCZOS_WIDTH };

static double lanczos_lut[LANCZOS_SAMPLES + 1];

enum { lanczos_buffer_size = LANCZOS_WIDTH * 4 };

int my_abs(int in)
{
	int const mask = in >> sizeof(int) * CHAR_BIT - 1;
	return (in + mask) ^ mask;
}

int fEqual(const double b, const double a)
{
	return fabs(a - b) < 1.0e-6;
}

static double sinc(double x)
{
	return fEqual(x, 0.0) ? 1.0 : sin(x * M_PI) / (x * M_PI);
}

void lanczos_init()
{
	unsigned i;
	double dx = (double)(LANCZOS_WIDTH) / LANCZOS_SAMPLES, x = 0.0;
	for (i = 0; i < LANCZOS_SAMPLES + 1; ++i, x += dx)
		lanczos_lut[i] = my_abs(x) < LANCZOS_WIDTH ? sinc(x) * sinc(x / LANCZOS_WIDTH) : 0.0;
}

typedef struct lanczos_resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    unsigned short phase;
    unsigned int phase_inc;
    float buffer_in[2][lanczos_buffer_size * 2];
    int buffer_out[lanczos_buffer_size * 2];
} lanczos_resampler;

void * lanczos_resampler_create()
{
	lanczos_resampler * r = ( lanczos_resampler * ) malloc( sizeof(lanczos_resampler) );
    if ( !r ) return 0;

	r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->phase_inc = 0;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
    memset( r->buffer_out, 0, sizeof(r->buffer_out) );

	return r;
}

void lanczos_resampler_delete(void * _r)
{
	free( _r );
}

void * lanczos_resampler_dup(void * _r)
{
    lanczos_resampler * r_in = ( lanczos_resampler * ) _r;
    lanczos_resampler * r_out = ( lanczos_resampler * ) malloc( sizeof(lanczos_resampler) );
    if ( !r_out ) return 0;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );

    return r_out;
}

int lanczos_resampler_get_free_count(void *_r)
{
	lanczos_resampler * r = ( lanczos_resampler * ) _r;
    return lanczos_buffer_size - r->write_filled;
}

int lanczos_resampler_ready(void *_r)
{
    lanczos_resampler * r = ( lanczos_resampler * ) _r;
    return r->write_filled > (LANCZOS_WIDTH * 2);
}

void lanczos_resampler_clear(void *_r)
{
    lanczos_resampler * r = ( lanczos_resampler * ) _r;
    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
}

void lanczos_resampler_set_rate(void *_r, double new_factor)
{
    lanczos_resampler * r = ( lanczos_resampler * ) _r;
    r->phase_inc = (int)( new_factor * LANCZOS_RESOLUTION );
}

void lanczos_resampler_write_sample(void *_r, short ls, short rs)
{
	lanczos_resampler * r = ( lanczos_resampler * ) _r;

    if ( r->write_filled < lanczos_buffer_size )
	{
        float s32 = ls;

        r->buffer_in[ 0 ][ r->write_pos ] = s32;
        r->buffer_in[ 0 ][ r->write_pos + lanczos_buffer_size ] = s32;

		s32 = rs;

		r->buffer_in[ 1 ][ r->write_pos ] = s32;
		r->buffer_in[ 1 ][ r->write_pos + lanczos_buffer_size ] = s32;

        ++r->write_filled;

		r->write_pos = ( r->write_pos + 1 ) % lanczos_buffer_size;
	}
}

int lanczos_resampler_run(void *_r, int ** out_, int * out_end)
{
	lanczos_resampler * r = ( lanczos_resampler * ) _r;
    int in_size = r->write_filled;
	int in_offset = lanczos_buffer_size + r->write_pos - r->write_filled;
    float const* inl_ = r->buffer_in[0] + in_offset;
	float const* inr_ = r->buffer_in[1] + in_offset;
	int used = 0;
	in_size -= LANCZOS_WIDTH * 2;
	if ( in_size > 0 )
	{
		int* out = *out_;
		float const* inl = inl_;
		float const* inr = inr_;
		float const* const in_end = inl + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;

		int step = phase_inc > LANCZOS_RESOLUTION ? LANCZOS_RESOLUTION * LANCZOS_RESOLUTION / phase_inc : LANCZOS_RESOLUTION;
		
		do
		{
			// accumulate in extended precision
            double kernel[LANCZOS_WIDTH * 2], kernel_sum = 0.0;
			int i = LANCZOS_WIDTH;
			int phase_adj = phase * step / LANCZOS_RESOLUTION;
			double samplel, sampler;
			int in_inc;

			if ( out >= out_end )
				break;

			for (; i >= -LANCZOS_WIDTH + 1; --i)
			{
				int pos = i * step;
				kernel_sum += kernel[i + LANCZOS_WIDTH - 1] = lanczos_lut[my_abs(phase_adj - pos)];
			}
			for (samplel = 0, sampler = 0, i = 0; i < LANCZOS_WIDTH * 2; ++i)
			{
				samplel += inl[i] * kernel[i];
				sampler += inr[i] * kernel[i];
			}
			kernel_sum = 1 / kernel_sum * 256.;
			*out++ = (int) (samplel * kernel_sum);
			*out++ = (int) (sampler * kernel_sum);

            phase += phase_inc;

			in_inc = phase >> 13;
            inl += in_inc;
			inr += in_inc;

            phase &= 8191;
		}
		while ( inl < in_end );
		
        r->phase = phase;
		*out_ = out;

		used = inl - inl_;

        r->write_filled -= used;
	}
	
	return used;
}

void lanczos_resampler_get_sample(void *_r, int * ls, int * rs)
{
    lanczos_resampler * r = ( lanczos_resampler * ) _r;
    if ( r->read_filled < 1 )
    {
        int write_pos = ( r->read_pos + r->read_filled ) % lanczos_buffer_size;
        int write_size = lanczos_buffer_size - write_pos;
        int * out = r->buffer_out + write_pos * 2;
        if ( write_size > ( lanczos_buffer_size - r->read_filled ) )
            write_size = lanczos_buffer_size - r->read_filled;
        lanczos_resampler_run( r, &out, out + write_size * 2 );
        r->read_filled += ( out - r->buffer_out ) / 2 - write_pos;
    }
    if ( r->read_filled < 1 )
	{
		*ls = 0;
		*rs = 0;
	}
	else
	{
		*ls = r->buffer_out[ r->read_pos * 2 ];
		*rs = r->buffer_out[ r->read_pos * 2 + 1 ];
	}
}

void lanczos_resampler_remove_sample(void *_r)
{
    lanczos_resampler * r = ( lanczos_resampler * ) _r;
    if ( r->read_filled > 0 )
    {
        --r->read_filled;
        r->read_pos = ( r->read_pos + 1 ) % lanczos_buffer_size;
    }
}
