#ifndef __CPSG_DRUM_HPP__
#include "structures/pl_list.hpp"
#include <vector>

namespace dsa {
    namespace C {
#include "device/emu2149.h"
    }
}
#include "DsaCommon.hpp"
#include "ISoundDevice.hpp"
#include "CEnvelope.hpp"

namespace dsa {

class CPSGDrum : public ISoundDevice {
public:
  struct Instrument {
    UINT8 note;
    INT8 vol;
    UINT8 noise;
    CEnvelope::Param param;
  };

  struct ChannelInfo {
    UINT8 note;
    INT8 vol;
    UINT8 noise;
    bool keyon;
  };

private:
  DWORD m_rate;
  UINT m_nch;
  C::PSG *m_psg[2];
  BYTE m_reg_cache[2][0x10]; 
  UINT8 m_noise_mode[2];
  UINT16 m_note2freq[128];
  struct KeyInfo { UINT ch; UINT8 note; };
  typedef pl_list<KeyInfo> OnChannelsQ;
  OnChannelsQ m_on_channels;
  typedef pl_list<UINT> OffChannelsQ;
  OffChannelsQ m_off_channels;
  ChannelInfo m_ci[6];
  CEnvelope m_env;
  UINT8 m_volume;
  UINT8 m_velocity[128];
  INT m_keytable[128];
  typedef pl_list<INT32> RBuf;
  std::vector<RBuf> m_rbuf; // The rendering buffer
  void _UpdateMode(UINT ch);
  void _UpdateVolume(UINT ch);
  void _UpdateFreq(UINT ch);
  void _UpdateProgram(UINT ch);
  void _WriteReg(BYTE reg, BYTE val, UINT id);
public:
  explicit CPSGDrum(DWORD rate=44100, UINT m_nch=1);
  virtual ~CPSGDrum();
  const SoundDeviceInfo &GetDeviceInfo(void) const;
  RESULT Reset(void);
  RESULT Render(INT32 buf[2]);

  void PercKeyOn(UINT8 note);
  void PercKeyOff(UINT8 note);
  void PercSetVolume(UINT8 vol);
  void PercSetVelocity(UINT8 note, UINT8 velo);
  void PercSetProgram(UINT8 bank, UINT8 prog);

  void SetProgram(UINT ch, UINT8 bank, UINT8 prog){};
  void SetVelocity(UINT ch, UINT8 vel){};
  void SetPan(UINT ch, UINT8 pan){};
  void SetVolume(UINT ch, UINT8 vol){};
  void SetBend(UINT ch, INT8 coarse, INT8 fine){};
  void KeyOn(UINT ch, UINT8 note){};
  void KeyOff(UINT ch){};
};


} // namespace 

#endif
