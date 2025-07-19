#include <cstdio>
#include "CSMFPlay.hpp"
#include "COpllDevice.hpp"
#include "CSccDevice.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

CSMFPlay::CSMFPlay(DWORD rate, int mods) {
  m_rate = rate;
  m_mods = mods;
  rendered_ticks = 0;
  wanted_ticks = 0;
  for(int i=0;i<m_mods;i++){
    if(i&1)
      m_module[i].AttachDevice(new CSccDevice(rate,2));
    else
      m_module[i].AttachDevice(new COpllDevice(rate,2));
  }
}

CSMFPlay::~CSMFPlay() {
  for(int i=0;i<m_mods;i++) delete m_module[i].DetachDevice();
}

bool CSMFPlay::Load(const void *buf, int size) {
  if(!m_smf.Load((const char *)buf,size)) { 
    return false; 
  }
  return true;
}

bool CSMFPlay::Open(char *filename) {

  FILE *fp;
  DWORD length;
  char *data;

  if((fp=fopen(filename,"rb"))==NULL) {
    // printf("Can't open %s\n",filename);
    return false;
  };

  fseek(fp,0L,SEEK_END);
  length = ftell(fp);
  fseek(fp,0L,SEEK_SET);
  data = new char [length];
  if(fread(data,sizeof(char),length,fp)!=length) {
    // printf("Fatal Error.\n");
    return false;
  };
  fclose(fp);

  if(!m_smf.Load(data,length)) { 
    // printf("Invalid SMF.\n"); 
    delete [] data;
    return false; 
  }

  /*
  printf("[MThd]\n");
  printf("MThd Length: %d\n", m_smf.m_MThd_length);
  printf("Format     : %2d\n", m_smf.m_format);
  printf("TrackNum   : %2d\n",   m_smf.m_track_num);
  printf("TimeBase   : %2d\n\n", m_smf.m_time_base); 
  for(int trk=0; trk<m_smf.m_track_num; trk++) {
    printf("MTrk[%03d]: %d bytes at %p\n",trk,m_smf.m_MTrk_length[trk],m_smf.m_MTrk_data[trk]);
  }
  */

  delete [] data;
  return true;
}

void CSMFPlay::Start(bool reset) {

  if (reset) for(int i=0;i<m_mods;i++) m_module[i].Reset();

  m_tempo = (60*1000000)/120;
  m_time_rest = 0.0;

  rendered_ticks = 0;
  wanted_ticks = 0;

  m_delta.clear();
  m_end_flag.clear();
  m_event.clear();

  m_playing_tracks = m_smf.m_track_num;

  for(int trk=0; trk<m_smf.m_track_num; trk++) {
    m_smf.Seek(trk,0);
    m_delta.push_back(0);
    m_end_flag.push_back(false);
    try {
      m_event.push_back(m_smf.ReadNextEvent(trk));
	  m_delta[trk] = m_event[trk].m_delta;
    } catch (CSMF_Exception) {
      m_end_flag[trk] = true;
    }
  }
}

void CSMFPlay::SetEndPoint(int tick) {
	wanted_ticks = tick;
}

DWORD CSMFPlay::Render(int *buf, DWORD length) {
  
  const double sample_in_us = 1.0E06 / (double)m_rate;
  CMIDIMsg msg;
  CMIDIMsgInterpreter mi;
  DWORD idx = 0;
  int trk;

  if( m_playing_tracks <= 0 || (wanted_ticks && wanted_ticks <= rendered_ticks)) return 0;

  while( idx < length ) {

    for(trk=0; trk<m_smf.m_track_num; trk ++) {

      while ( !m_end_flag[trk] && m_delta[trk] == 0 ) { 

        if(m_event[trk].m_type == CSMFEvent::MIDI_EVENT) {
          if (buf) {
            mi.Interpret((BYTE)m_event[trk].m_status);
            for(DWORD i=0;i<m_event[trk].m_length;i++) mi.Interpret(m_event[trk].m_data[i]);
            while(mi.GetMsgCount()) {
              const CMIDIMsg &msg = mi.GetMsg();
              m_module[(msg.m_ch*2)%m_mods].SendMIDIMsg(msg);
              if(msg.m_ch!=9)
                m_module[(msg.m_ch*2+1)%m_mods].SendMIDIMsg(msg);
              mi.PopMsg();
            }
          }
        } else if(m_event[trk].m_type == CSMFEvent::META_EVENT) {
          if(m_event[trk].m_status == 0xFF2F) { // ENDOFTRACK
            m_end_flag[trk] = true;
            m_playing_tracks--;
            /*
            printf("<META>EndOfTrack</META>\n");
            */
            break;
          } else if(m_event[trk].m_status == 0xFF01) {
            /*
            printf("<META><TEXT>\n");
            for(DWORD i=0;i<m_event[trk].m_length;i++) putchar(m_event[trk].m_data[i]);
            printf("\n</TEXT></META>\n");
            */
          } else if(m_event[trk].m_status == 0xFF51) { // TEMPO
            m_tempo = (m_event[trk].m_data[0]<<16)|(m_event[trk].m_data[1]<<8)|(m_event[trk].m_data[2]);
            //printf("<META>Tempo %06x</META>\n",m_tempo);
          } else {
            //printf("META: %04x\n",m_event[trk].m_status);
          }
        }
        
        try {
          m_event[trk] = m_smf.ReadNextEvent(trk);
          m_delta[trk] = m_event[trk].m_delta;
        } catch (CSMF_Exception e) {
          //printf("*%s\n",e.c_str());
          m_end_flag[trk] = true;
          m_playing_tracks--;
        }
      }
    } // endwhile;

    if (buf) {
      bool done = false;
      double tick_time = (double)m_tempo/m_smf.m_time_base; 

      while(!done) {
        if (m_time_rest < sample_in_us) m_time_rest += tick_time;
        while ( sample_in_us < m_time_rest) {
          m_time_rest -= sample_in_us;
          buf[idx*2] = buf[idx*2+1] = 0;
          for(int i=0; i<m_mods; i++) {
            INT32 b[2];
            m_module[i].Render(b);
            buf[idx*2] += b[0]; 
            buf[idx*2+1] += b[1];
          }
          idx++;
          if( length <= idx ) {
            done = true;
            break;
          }
        }
        if(!done) {
			if (wanted_ticks && wanted_ticks <= ++rendered_ticks) {
				length = idx;
				done = true;
			}
			else
          for(trk=0; trk<m_smf.m_track_num; trk++) {
			  if((0<m_delta[trk])&&(--m_delta[trk]==0)) done = true;
          }
        }
      }
    }
    else {
		int least = INT_MAX;
		for (trk = 0; trk < m_smf.m_track_num; trk++) {
			if ((0<m_delta[trk])&&(m_delta[trk] < least)) least = m_delta[trk];
		}

		if (least < INT_MAX) {
			rendered_ticks += least;
			idx += least;
			for (trk = 0; trk < m_smf.m_track_num; trk++) {
				if (0<m_delta[trk]) m_delta[trk] -= least;
			}
		}
		else break;
	}

  } // end while (idx < length )

  return idx;
}

