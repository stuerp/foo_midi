#ifndef __MIDI_MODULE_HPP__
#define __MIDI_MODULE_HPP__

#include <vector>
#include <cstddef>
#include <stdint.h>
#include "ISoundDevice.hpp"
#include "structures/pl_list.hpp"

namespace dsa {

class CMIDIModule {
private:
  struct KeyInfo { int midi_ch, dev_ch, note; };
  ISoundDevice *m_device;

  int m_bank_msb[16];
  int m_bank_lsb[16];
  int m_NRPN[16], m_RPN[16];
  int m_volume[16];
  int m_bend_coarse[16];
  int m_bend_fine[16];
  int m_bend_range[16];
  int m_program[16];
  int m_pan[16];
  int m_bend[16];
  int m_drum[16];
  int m_volume7[16];
  int m_expression[16];
  // そのキーを発音しているチャンネル番号を格納する配列
  int m_keyon_table[16][128];
  double m_bendsense[16];

  typedef pl_list<KeyInfo> ChannelList;
  // MIDIチャンネルで使用しているOPLLチャンネルの集合(発音順のキュー）
  std::vector<ChannelList> m_used_channels;
  // キーオフしているOPLLチャンネルの集合
  ChannelList m_off_channels;
  // The current entry value of RPN/NRPN
  // NRPN=1, RPN=0;
  int m_entry_mode;

  void updateBanks(BYTE ch);

  void updateBendSensitivity(BYTE ch);

protected:
  virtual void ControlChange(BYTE ch, BYTE msb, BYTE lsb);
  virtual void NoteOn (BYTE ch,  BYTE note, BYTE velo);
  virtual void NoteOff(BYTE ch,  BYTE note, BYTE velo);
  virtual void AllNotesOff(BYTE ch);
  virtual void UpdatePitchBend(BYTE ch);
  virtual void PitchBend(BYTE ch, BYTE msb, BYTE lsb);
  virtual void ChannelPressure(BYTE ch, BYTE velo);
  virtual void DataEntry(BYTE midi_ch, bool is_low, BYTE data);
  virtual void DataIncrement(BYTE midi_ch, BYTE data);
  virtual void DataDecrement(BYTE midi_ch, BYTE data);
  virtual void MainVolume(BYTE midi_ch, bool is_fine, BYTE data);
  virtual void Volume7(BYTE midi_ch, bool is_fine, BYTE data);
  virtual void Expression(BYTE midi_ch, bool is_fine, BYTE data);
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

// CMIDIメッセージ形式のMIDIメッセージを処理する。
  RESULT SendNoteOn (BYTE ch,  BYTE note, BYTE velo);
  RESULT SendNoteOff(BYTE ch,  BYTE note, BYTE velo);
  RESULT SendProgramChange(BYTE ch,  BYTE program);
  RESULT SendControlChange(BYTE ch, BYTE msb, BYTE lsb);
  RESULT SendPitchBend(BYTE ch, BYTE msb, BYTE lsb);
  RESULT SendChannelPressure(BYTE ch, BYTE velo);
  RESULT SendPanic();

  bool   IsDrum(BYTE ch);

// 音声のレンダリングを行う。
  RESULT Render(INT32 buf[2]);

  RESULT SetDrumChannel(int midi_ch, int enable);
};

} // namespace dsa

#endif
