// Copyright(c)2013 Chris Moeller
#ifndef iir_filter_hpp
#define iir_filter_hpp

#include "midisynth_common.hpp"

namespace midisynth{
	class iir_filter:uncopyable{
		float b02, b1, a1, a2;
		float b02i, b1i, a1i, a2i;

		float hl0, hl1, hr0, hr1;

		float fres, last_fres;
		float q_lin, last_q_lin, filter_gain;

		int incr_count, startup;

		float sample_rate;

	public:
		iir_filter();

		void reset();

		void set_q_dB(float q_dB);
		void set_fres(float fres);

		void calculate(float output_rate, float fres_mod);
        void apply(int_least32_t* buf, int pair_count);
	};
}

#endif
