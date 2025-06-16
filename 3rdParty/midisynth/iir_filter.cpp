// Copyright(c)2013 Chris Moeller

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "iir_filter.hpp"

namespace midisynth{
	iir_filter::iir_filter()
	{
		b02 = 0.0f; b1 = 0.0f; a1 = 0.0f; a2 = 0.0f;
		reset();
	}

	void iir_filter::reset()
	{
		hl0 = 0.0f; hl1 = 0.0f;
		hr0 = 0.0f; hr1 = 0.0f;
		last_fres = -1.0f;
		startup = 1;
	}

	void iir_filter::set_q_dB(float q_dB)
	{
		q_lin = (float) (std::pow(10.0f, q_dB / 20.0f));
		filter_gain = (float) (1.0 / std::sqrt(q_lin));
		last_fres = -1.0f;
	}

	void iir_filter::set_fres(float fres)
	{
		this->fres = fres;
		last_fres = -1.0f;
	}

	void iir_filter::calculate(float output_rate, float fres_mod)
	{
		float fres = midi_ct2hz(this->fres + fres_mod);

		if (fres > 0.45f * output_rate)
			fres = 0.45f * output_rate;
		else if (fres < 5.0f)
			fres = 5.0f;

		if (fabs(fres - last_fres) > 0.01)
		{
			last_fres = fres;

			float omega = (float) (2.0f * M_PI * (fres / output_rate));
			float sin_coeff = (float) std::sin(omega);
			float cos_coeff = (float) std::cos(omega);
			float alpha_coeff = sin_coeff / (2.0f * q_lin);
			float a0_inv = 1.0f / (1.0f + alpha_coeff);

			float a1 = -2.0f * cos_coeff * a0_inv;
			float a2 = (1.0f - alpha_coeff) * a0_inv;
			float b1 = (1.0f - cos_coeff) * a0_inv * filter_gain;
			float b02 = b1 * 0.5f;

			if (startup)
			{
				this->a1 = a1;
				this->a2 = a2;
				this->b1 = b1;
				this->b02 = b02;
				incr_count = 0;
				startup = 0;
			}
			else
			{
				const float transition_scale = 1.0f / 128.0f;

				this->a1i = (a1 - this->a1) * transition_scale;
				this->a2i = (a2 - this->a2) * transition_scale;
				this->b02i = (b02 - this->b02) * transition_scale;
				this->b1i = (b1 - this->b1) * transition_scale;
				incr_count = 128;
			}
		}
	}

    void iir_filter::apply(int_least32_t* buf, int pair_count)
	{
		float hl0 = this->hl0, hl1 = this->hl1, hr0 = this->hr0, hr1 = this->hr1;

		float a1 = this->a1, a2 = this->a2, b02 = this->b02, b1 = this->b1;

		float lcenternode, rcenternode;

		int i;

		hl0 += 0.00000000001;
		hr0 += 0.00000000001;

		if (incr_count)
		{
			float a1i = this->a1i, a2i = this->a2i, b02i = this->b02i, b1i = this->b1i;

			for (i = 0; i < pair_count; ++i)
			{
                lcenternode = (float)buf[i * 2 + 0] - a1 * hl0 - a2 * hl1;
                rcenternode = (float)buf[i * 2 + 1] - a1 * hr0 - a2 * hr1;
                buf[i * 2 + 0] = (int_least32_t)(b02 * (lcenternode + hl1) + b1 * hl0);
                buf[i * 2 + 1] = (int_least32_t)(b02 * (rcenternode + hr1) + b1 * hr0);
				hl1 = hl0;
				hl0 = lcenternode;
				hr1 = hr0;
				hr0 = rcenternode;

				if (incr_count-- > 0)
				{
					a1 += a1i;
					a2 += a2i;
					b02 += b02i;
					b1 += b1i;
				}
			}

			this->a1 = a1; this->a2 = a2; this->b02 = b02; this->b1 = b1;
		}
		else
		{
			for (i = 0; i < pair_count; ++i)
			{
                lcenternode = (float)buf[i * 2 + 0] - a1 * hl0 - a2 * hl1;
                rcenternode = (float)buf[i * 2 + 1] - a1 * hr0 - a2 * hr1;
                buf[i * 2 + 0] = (int_least32_t)(b02 * (lcenternode + hl1) + b1 * hl0);
                buf[i * 2 + 1] = (int_least32_t)(b02 * (rcenternode + hr1) + b1 * hr0);
				hl1 = hl0;
				hl0 = lcenternode;
				hr1 = hr0;
				hr0 = rcenternode;
			}
		}

		this->hl0 = hl0; this->hl1 = hl1; this->hr0 = hr0; this->hr1 = hr1;
	}
}
