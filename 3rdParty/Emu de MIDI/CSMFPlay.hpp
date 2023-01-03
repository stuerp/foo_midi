#ifndef __CSMFPLAY_HPP__
#define __CSMFPLAY_HPP__
#include <vector>

#include "CSMF.hpp"
#include "CMIDIModule.hpp"

namespace dsa {

class CSMFPlay {
  CSMF m_smf;
  CMIDIModule m_module[16];

  std::vector<CSMFEvent> m_event;
  std::vector<bool> m_end_flag;
  std::vector<int>  m_delta;
  int m_tempo;
  double m_time_rest;
  int m_playing_tracks;
  int m_mods;
  int m_rate;

  int rendered_ticks;
  int wanted_ticks;

public:  
  CSMFPlay(DWORD rate, int mods=4);
  ~CSMFPlay();
  bool Open(char *filename);
  bool Load(const void *buf, int size);
  DWORD Render(int *buf, DWORD length);
  void Start(bool reset = true);

  void SetEndPoint(int tick);
};

} // namespace dsa

#endif
