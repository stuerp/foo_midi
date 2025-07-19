#ifndef __MIDI_MODULE_HPP__
#define __MIDI_MODULE_HPP__

#include "ISoundDevice.hpp"
#include "CMIDIMessage.hpp"

namespace dsa {

class CMIDIModule {
private:
  struct KeyInfo { int midi_ch, dev_ch, note; };
  ISoundDevice *m_device;

  int m_NRPN[16], m_RPN[16];
  int m_volume[16];
  int m_bend_coarse[16];
  int m_bend_fine[16];
  int m_bend_range[16];
  int m_program[16];
  int m_pan[16];
  int m_bend[16];
  int m_drum[16];
  // ���̃L�[�𔭉����Ă���`�����l���ԍ����i�[����z��
  int m_keyon_table[16][128];
  // MIDI�`�����l���Ŏg�p���Ă���OPLL�`�����l���̏W��(�������̃L���[�j
  std::deque<KeyInfo> m_used_channels[16];
  // �L�[�I�t���Ă���OPLL�`�����l���̏W��
  std::deque<KeyInfo> m_off_channels; 
  // The current entry value of RPN/NRPN
  // NRPN=1, RPN=0;
  int m_entry_mode;

protected:
  virtual void ControlChange(BYTE ch, BYTE msb, BYTE lsb);
  virtual void NoteOn (BYTE ch,  BYTE note, BYTE velo);
  virtual void NoteOff(BYTE ch,  BYTE note, BYTE velo);
  virtual void UpdatePitchBend(BYTE ch);
  virtual void PitchBend(BYTE ch, BYTE msb, BYTE lsb);
  virtual void ChannelPressure(BYTE ch, BYTE velo);
  virtual void DataEntry(BYTE midi_ch, bool is_low, BYTE data);
  virtual void DataIncrement(BYTE midi_ch, BYTE data);
  virtual void DataDecrement(BYTE midi_ch, BYTE data);
  virtual void MainVolume(BYTE midi_ch, bool is_fine, BYTE data);
  virtual void NRPN(BYTE midi_ch, bool is_fine, BYTE data);
  virtual void RPN(BYTE midi_ch, bool is_fine, BYTE data);
  virtual void LoadRPN(BYTE midi_ch, WORD data);
  virtual void LoadNRPN(BYTE midi_ch, WORD data);
  virtual WORD SaveRPN(BYTE midi_ch);
  virtual WORD SaveNRPN(BYTE midi_ch);
  virtual void ResetRPN(BYTE midi_ch);
  virtual void ResetNRPN(BYTE midi_ch);
  virtual void Panpot(BYTE ch, bool is_fine, BYTE data);

public:
  CMIDIModule();
  virtual ~CMIDIModule();
  void AttachDevice(ISoundDevice *device){ m_device = device; }
  ISoundDevice *DetachDevice(){ ISoundDevice *tmp=m_device; m_device = NULL; return tmp; }
  RESULT Reset();
// CMIDI���b�Z�[�W�`����MIDI���b�Z�[�W����������B
  RESULT SendMIDIMsg(const CMIDIMsg &mes);
// �����̃����_�����O���s���B
  RESULT Render(INT32 buf[2]);

  RESULT SetDrumChannel(int midi_ch, int enable);
};

} // namespace dsa

#endif