#include "oplmidiPlayer.h"

#include <midisynth.hpp>
#include <oplmidi.hpp>

#include "adldata.h"

#include "shared.h"

oplmidiPlayer::oplmidiPlayer() : MIDIPlayer()
{
	factory = 0;
	for ( unsigned i = 0; i < 4; i++ )
	{
		synthesizers[ i ] = 0;
	}
}

oplmidiPlayer::~oplmidiPlayer()
{
	shutdown();
}

void oplmidiPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned port = (b >> 24) & 0x7F;
		synthesizers[ port & 3 ]->midi_event( b );
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		synthesizers[ port & 3 ]->sysex_message( data, size );
	}
}

void oplmidiPlayer::render( float * out, unsigned long count )
{
	int_least32_t buffer[ 512 ];

	while ( count )
	{
		unsigned todo = count;
		if ( todo > 256 ) todo = 256;

		memset( buffer, 0, todo * sizeof(*buffer) * 2 );

		for ( unsigned i = 0; i < 4; i++ )
		{
			synthesizers[ i ]->synthesize_mixing( buffer, todo, uSampleRate );
		}

		audio_math::convert_from_int32( (const t_int32 *) buffer, todo * 2, out, 65536.0 );

		out += todo * 2;
		count -= todo;
	}
}

void oplmidiPlayer::setBank( unsigned bank )
{
	uBankNumber = bank;
}

void oplmidiPlayer::shutdown()
{
	for ( unsigned i = 0; i < 4; i++ )
	{
		delete synthesizers[ i ];
		synthesizers[ i ] = NULL;
	}
	delete factory;
	factory = NULL;
}

bool oplmidiPlayer::startup()
{
	if ( factory ) return true;

	factory = new midisynth::opl::fm_note_factory;

	for ( unsigned i = 0; i < 256; ++i )
	{
		struct midisynth::opl::FMPARAMETER param;
		const struct adlinsdata * insdata = &adlins[ banks[uBankNumber][i] ];
		param.mode = (insdata->adlno1 == insdata->adlno2) ? midisynth::opl::FMPARAMETER::mode_single :
			(insdata->flags & adlinsdata::Flag_Pseudo4op) ? midisynth::opl::FMPARAMETER::mode_double :
			midisynth::opl::FMPARAMETER::mode_fourop;
		param.tone = insdata->tone;
		param.key_on_ms = insdata->ms_sound_kon;
		param.key_off_ms = insdata->ms_sound_koff;
		const struct adldata * ins = &adl[insdata->adlno1];
		param.ops[0].modulator_E862 = ins->modulator_E862;
		param.ops[0].carrier_E862 = ins->carrier_E862;
		param.ops[0].modulator_40 = ins->modulator_40;
		param.ops[0].carrier_40 = ins->carrier_40;
		param.ops[0].feedconn = ins->feedconn;
		param.ops[0].finetune = ins->finetune;
		ins = &adl[insdata->adlno2];
		param.ops[1].modulator_E862 = ins->modulator_E862;
		param.ops[1].carrier_E862 = ins->carrier_E862;
		param.ops[1].modulator_40 = ins->modulator_40;
		param.ops[1].carrier_40 = ins->carrier_40;
		param.ops[1].feedconn = ins->feedconn;
		param.ops[1].finetune = ins->finetune;

		if (i < 128) factory->set_program(i, param);
		else factory->set_drum_program(i - 128, param);
	}

	for ( unsigned i = 0; i < 4; i++ )
	{
		synthesizers[ i ] = new midisynth::synthesizer( factory );
	}

	return true;
}
