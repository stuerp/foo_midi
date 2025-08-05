#include "CMIDIModule.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

CMIDIModule::CMIDIModule() : m_device(NULL),
    m_used_channels(16, ChannelList(128)),
    m_off_channels(128)
{}
CMIDIModule::~CMIDIModule() {}

RESULT CMIDIModule::Reset()
{

    if(m_device == NULL) return FAILURE;
    if(!m_device->Reset()) return FAILURE;

    m_off_channels.clear();

    {
        for(int i = 0; i < 16; i++)
        {
            m_used_channels[i].clear();
            m_program[i] = 3;
            m_bank_msb[i] = 0;
            m_bank_lsb[i] = 0;
            m_volume[i] = 100;
            m_bend[i] = 0;
            m_bend_coarse[i] = 0;
            m_bend_fine[i] = 0;
            m_bend_range[i] = (2 << 7);
            m_pan[i] = 64;
            m_RPN[i] = m_NRPN[i] = 0;
            m_drum[i] = 0;
            m_volume7[i] = 100;
            m_expression[i] = 127;
            for(int j = 0; j < 128; j++)
                m_keyon_table[i][j] = -1;
            updateBendSensitivity(i);
        }
    }
    m_drum[9] = 1;

    m_entry_mode = 0;

    const SoundDeviceInfo &si = m_device->GetDeviceInfo();

    {
        for(UINT i = 0; i < si.max_ch; i++)
        {
            KeyInfo ki;
            ki.midi_ch = i;
            ki.dev_ch = i;
            ki.note = 0;
            m_keyon_table[i][0] = -1;
            m_off_channels.push_back(ki);
            m_used_channels[i].push_back(ki);
        }
    }

    return SUCCESS;
}

void CMIDIModule::Panpot(BYTE midi_ch, bool is_fine, BYTE data)
{
    if(!is_fine)
    {
        m_pan[midi_ch] = data;
        ChannelList::iterator it;
        for(it = m_used_channels[midi_ch].begin(); it != m_used_channels[midi_ch].end(); it++)
            m_device->SetPan(it->value.dev_ch, m_pan[midi_ch]);
    }
}

void CMIDIModule::UpdatePitchBend(BYTE midi_ch)
{
//    int range = (m_bend_range[midi_ch] >> 7);
//    if(range != 0)
//    {
//        m_bend_coarse[midi_ch] = (m_bend[midi_ch] * range) / 8192 ; // note offset
//        m_bend_fine[midi_ch] = ((m_bend[midi_ch] % (8192 / range)) * 100 * range) / 8192; // cent offset
//    }
//    else
//    {
//        m_bend_coarse[midi_ch] = 0;
//        m_bend_fine[midi_ch] = 0;
//    }

    double bend = m_bend[midi_ch] * m_bendsense[midi_ch];
    double bendDec = bend - (int)bend;

    m_bend_coarse[midi_ch] = (int)bend;
    m_bend_fine[midi_ch] = 100 * bendDec; // cent offset

//    fprintf(stdout, "%d,%d\n", m_bend_coarse[midi_ch], m_bend_fine[midi_ch]);
//    fflush(stdout);

    ChannelList::iterator it;
    for(it = m_used_channels[midi_ch].begin(); it != m_used_channels[midi_ch].end(); it++)
        m_device->SetBend(it->value.dev_ch, m_bend_coarse[midi_ch], m_bend_fine[midi_ch]);
}

void CMIDIModule::PitchBend(BYTE midi_ch, BYTE msb, BYTE lsb)
{
    m_bend[midi_ch] = ((msb & 0x7f) | ((lsb & 0x7f) << 7)) - 8192;
    UpdatePitchBend(midi_ch);
}

void CMIDIModule::ChannelPressure(BYTE midi_ch, BYTE velo)
{
// TODO: Implement a missing TRUE vibrato (probably use a code from libOPNMIDI), Channel Pressure should be a tremolo, NOT a volume
//    std::deque<KeyInfo>::iterator it;
//    for(it = m_used_channels[midi_ch].begin(); it != m_used_channels[midi_ch].end(); it++)
//        m_device->SetVibrato((*it).dev_ch, velo);
}

void CMIDIModule::NoteOn(BYTE midi_ch, BYTE note, BYTE velo)
{
    if(m_drum[midi_ch])
    {
        m_device->PercSetVelocity(note, velo);
        m_device->PercKeyOn(note);
        return;
    }

    if(0 <= m_keyon_table[midi_ch][note]) return; //キーオン中なら無視

    KeyInfo ki;

    if(m_off_channels.empty())     // キーオフ中のデバイスチャンネルが無いとき
    {
        ki.dev_ch = -1;
        for(int i = 0; i < 16; i++) // 発音数が規定値より多いMIDIチャンネルを消音
        {
            if(m_used_channels[i].size() > 1)
            {
                ki = m_used_channels[i].front().value;
                m_device->KeyOff(ki.dev_ch);
                m_keyon_table[i][ki.note] = -1;
                m_used_channels[i].pop_front();
                break;
            }
        }
        if(ki.dev_ch == -1) // だめならどこでもいいから消音
        {
            for(int i = 0; i < 16; i++)
            {
                if(!m_used_channels[i].empty())
                {
                    ki = m_used_channels[i].front().value;
                    m_device->KeyOff(ki.dev_ch);
                    m_keyon_table[i][ki.note] = -1;
                    m_used_channels[i].pop_front();
                    break;
                }
            }
        }

    }
    else     // キーオフ中のチャンネルがあるときはそれを利用
    {
        ki = m_off_channels.front().value;
        m_off_channels.pop_front();
        ChannelList::iterator it;
        for(it = m_used_channels[ki.midi_ch].begin(); it != m_used_channels[ki.midi_ch].end(); it++)
        {
            if(it->value.dev_ch == ki.dev_ch)
            {
                m_used_channels[ki.midi_ch].erase(it);
                break;
            }
        }
    }

    m_device->SetProgram(ki.dev_ch, 0, m_program[midi_ch]);
    m_device->SetVolume(ki.dev_ch, m_volume[midi_ch]);
    m_device->SetVelocity(ki.dev_ch, velo);
    m_device->SetBend(ki.dev_ch, m_bend_coarse[midi_ch], m_bend_fine[midi_ch]);
    m_device->SetPan(ki.dev_ch, m_pan[midi_ch]);
    m_device->KeyOn(ki.dev_ch, note);
    m_keyon_table[midi_ch][note] = ki.dev_ch;
    ki.midi_ch = midi_ch;
    ki.note = note;
    m_used_channels[midi_ch].push_back(ki);
}

void CMIDIModule::NoteOff(BYTE midi_ch, BYTE note, BYTE /*velo*/)
{
    if(m_drum[midi_ch])
        m_device->PercKeyOff(note);

    int dev_ch = m_keyon_table[midi_ch][note];
    if(dev_ch < 0) return;
    m_device->KeyOff(dev_ch);
    m_keyon_table[midi_ch][note] = -1;
    ChannelList::iterator it;
    KeyInfo ki;
    ki.dev_ch = dev_ch;
    ki.midi_ch = midi_ch;
    ki.note = 0;
    m_off_channels.push_back(ki);
}

void CMIDIModule::AllNotesOff(BYTE ch)
{
    for(int i = 0; i < 127; ++i)
    {
        if(m_drum[ch])
            m_device->PercKeyOff(i);

        int dev_ch = m_keyon_table[ch][i];
        if(dev_ch < 0)
            continue;

        m_device->KeyOff(dev_ch);
        m_keyon_table[ch][i] = -1;
        ChannelList::iterator it;
        KeyInfo ki;
        ki.dev_ch = dev_ch;
        ki.midi_ch = ch;
        ki.note = 0;
        m_off_channels.push_back(ki);
    }
}

void CMIDIModule::MainVolume(BYTE midi_ch, bool is_fine, BYTE data)
{
    if(is_fine) return;

    if(m_drum[midi_ch])
    {
        m_device->PercSetVolume(data);
        return;
    }

    ChannelList::iterator it;
    for(it = m_used_channels[midi_ch].begin(); it != m_used_channels[midi_ch].end(); it++)
        m_device->SetVolume(it->value.dev_ch, data);
}

void CMIDIModule::Volume7(BYTE midi_ch, bool is_fine, BYTE data)
{
    if(is_fine) return;
    m_volume7[midi_ch] = data;
    BYTE res = (BYTE)((int(m_expression[midi_ch]) * int(m_volume7[midi_ch])) / 127);
    MainVolume(midi_ch, is_fine, res);
}

void CMIDIModule::Expression(BYTE midi_ch, bool is_fine, BYTE data)
{
    if(is_fine) return;
    m_expression[midi_ch] = data;
    BYTE res = (BYTE)((int(m_expression[midi_ch]) * int(m_volume7[midi_ch])) / 127);
    MainVolume(midi_ch, is_fine, res);
}

void CMIDIModule::LoadRPN(BYTE midi_ch, WORD data)
{
    switch(m_RPN[midi_ch])
    {
    case 0x0000:
        m_bend_range[midi_ch] = data;
        updateBendSensitivity(midi_ch);
        UpdatePitchBend(midi_ch);
        break;
    default:
        break;
    }
}

WORD CMIDIModule::SaveRPN(BYTE midi_ch)
{
    switch(m_RPN[midi_ch])
    {
    case 0x0000:
        return m_bend_range[midi_ch];
        break;
    default:
        return 0;
        break;
    }
}

void CMIDIModule::ResetRPN(BYTE midi_ch)
{
    m_bend_range[midi_ch] = (2 << 7);
}

void CMIDIModule::LoadNRPN(BYTE midi_ch, WORD data)
{
}

WORD CMIDIModule::SaveNRPN(BYTE midi_ch)
{
    return 0;
}

void CMIDIModule::ResetNRPN(BYTE midi_ch)
{
}

void CMIDIModule::DataEntry(BYTE midi_ch, bool is_fine, BYTE data)
{
    int entry = m_entry_mode ? SaveNRPN(midi_ch) : SaveRPN(midi_ch);
    if(is_fine)
        entry = (entry & 0x3F80) | (data & 0x7F);
    else
        entry = ((data & 0x7F) << 7) | (entry & 0x7F);
    m_entry_mode ? LoadNRPN(midi_ch, entry) : LoadRPN(midi_ch, entry);
}

void CMIDIModule::DataIncrement(BYTE midi_ch, BYTE data)
{
    int entry = m_entry_mode ? SaveNRPN(midi_ch) : SaveRPN(midi_ch);
    if(entry < 0x3FFF) entry++;
    m_entry_mode ? LoadNRPN(midi_ch, entry) : LoadRPN(midi_ch, entry);
}

void CMIDIModule::DataDecrement(BYTE midi_ch, BYTE data)
{
    int entry = m_entry_mode ? SaveNRPN(midi_ch) : SaveRPN(midi_ch);
    if(entry > 0) entry--;
    m_entry_mode ? LoadNRPN(midi_ch, entry) : LoadRPN(midi_ch, entry);
}

void CMIDIModule::NRPN(BYTE midi_ch, bool is_lsb, BYTE data)
{
    if(is_lsb)
        m_NRPN[midi_ch] = (m_NRPN[midi_ch] & 0x3F80) | (data & 0x7F);
    else
        m_NRPN[midi_ch] = ((data & 0x7F) << 7) | (m_NRPN[midi_ch] & 0x7F);
    if(m_NRPN[midi_ch] == 0x3FFF)   // NRPN NULL
        ResetNRPN(midi_ch);
    if(m_entry_mode == 0)
    {
        m_entry_mode = 1; // NRPN MODE
    }
}

void CMIDIModule::RPN(BYTE midi_ch, bool is_lsb, BYTE data)
{
    if(is_lsb)
    {
        m_RPN[midi_ch] = (m_RPN[midi_ch] & 0x3F80) | (data & 0x7F);
        //if(m_RPN[midi_ch] == 0x3FFF) RPN_Reset();
    }
    else
        m_RPN[midi_ch] = ((data & 0x7F) << 7) | (m_RPN[midi_ch] & 0x7F);
    if(m_RPN[midi_ch] == 0x3FFF)   // RPN NULL
        ResetRPN(midi_ch);
    if(m_entry_mode == 1)
    {
        m_entry_mode = 0; // RPN MODE
    }
}

void CMIDIModule::updateBanks(BYTE ch)
{
    if(ch != 9)
        m_drum[ch] = ((m_bank_lsb[ch] == 0) && (m_bank_msb[ch] == 127)) ? 1 : 0;
}

void CMIDIModule::updateBendSensitivity(BYTE ch)
{
    m_bendsense[ch] = m_bend_range[ch] * (1.0 / (128 * 8192));
}

void CMIDIModule::ControlChange(BYTE midi_ch, BYTE msb, BYTE lsb)
{

    if(msb < 0x40) // 14-bit
    {
        if(msb == 0x20)
        {
            m_bank_lsb[midi_ch] = lsb;
            updateBanks(midi_ch);
            return;
        }

        bool is_low = (msb & 0x20) ? true : false;
        switch(msb & 0x1F)
        {
        case 0x00:
            m_bank_msb[midi_ch] = lsb;
            updateBanks(midi_ch);
            break;
        //case 0x01: ModulationDepth(midi_ch, is_low, lsb); break;
        //case 0x02: BreathControl(midi_ch, is_low, lsb); break;
        //case 0x04: FootControl(midi_ch, is_low, lsb); break;
        //case 0x05: PortamentTime(midi_ch, is_low, lsb); break;
        case 0x06:
            DataEntry(midi_ch, is_low, lsb);
            break;
        case 0x07:
            Volume7(midi_ch, is_low, lsb);
            break;
        //case 0x08: BalanceControl(midi_ch, is_low, lsb); break;
        case 0x0A:
            Panpot(midi_ch, is_low, lsb);
            break;
        case 0x0B:
            Expression(midi_ch, is_low, lsb);
            break;
        default:
            break;
        }
    }
    else     // 7-bit
    {
        switch(msb)
        {
        case 0x40:
            break;
        case 0x60:
            DataIncrement(midi_ch, lsb);
            break;
        case 0x61:
            DataDecrement(midi_ch, lsb);
            break;
        case 0x62:
            NRPN(midi_ch, 0, lsb);
            break;
        case 0x63:
            NRPN(midi_ch, 1, lsb);
            break;
        case 0x64:
            RPN(midi_ch, 0, lsb);
            break;
        case 0x65:
            RPN(midi_ch, 1, lsb);
            break;
        case 0x79: // Reset all controllers
            break;
        case 0x78: // All sounds off
            AllNotesOff(midi_ch);
            break;
        case 0x7B: // All notes off
            AllNotesOff(midi_ch);
            break;
        default:
            break;
        }
    }

}

RESULT CMIDIModule::Render(INT32 buf[2])
{
    if(m_device == NULL)
        return FAILURE;
    else
        return m_device->Render(buf);
}

#if 0
RESULT CMIDIModule::SendMIDIMsg(const CMIDIMsg &msg)
{

    if(m_device == NULL)
        return FAILURE;

    if(msg.m_type == CMIDIMsg::NOTE_OFF)
        NoteOff(msg.m_ch, msg.m_data[0], msg.m_data[1]);
    else if(msg.m_type == CMIDIMsg::NOTE_ON)
    {
        if(msg.m_data[1] == 0)
            NoteOff(msg.m_ch, msg.m_data[0], msg.m_data[1]);
        else
            NoteOn(msg.m_ch, msg.m_data[0], msg.m_data[1]);
    }
    else if(msg.m_type == CMIDIMsg::PROGRAM_CHANGE)
        m_program[msg.m_ch] = msg.m_data[0];
    else if(msg.m_type == CMIDIMsg::CONTROL_CHANGE)
        ControlChange(msg.m_ch, msg.m_data[0], msg.m_data[1]);
    else if(msg.m_type == CMIDIMsg::PITCH_BEND_CHANGE)
        PitchBend(msg.m_ch, msg.m_data[0], msg.m_data[1]);
    else if(msg.m_type == CMIDIMsg::CHANNEL_PRESSURE)
        ChannelPressure(msg.m_ch, msg.m_data[0]);
    return SUCCESS;
}
#endif

RESULT CMIDIModule::SendNoteOn(BYTE ch, BYTE note, BYTE velo)
{
    if(m_device == NULL)
        return FAILURE;
    if(velo == 0)
        NoteOff(ch, note, velo);
    else
        NoteOn(ch, note, velo);
    return SUCCESS;
}

RESULT CMIDIModule::SendNoteOff(BYTE ch, BYTE note, BYTE velo)
{
    if(m_device == NULL)
        return FAILURE;
    NoteOff(ch, note, velo);
    return SUCCESS;
}

RESULT CMIDIModule::SendProgramChange(BYTE ch, BYTE program)
{
    if(m_device == NULL)
        return FAILURE;
    m_program[ch] = program;
    return SUCCESS;
}

RESULT CMIDIModule::SendControlChange(BYTE ch, BYTE msb, BYTE lsb)
{
    if(m_device == NULL)
        return FAILURE;
    ControlChange(ch, msb, lsb);
    return SUCCESS;
}

RESULT CMIDIModule::SendPitchBend(BYTE ch, BYTE msb, BYTE lsb)
{
    if(m_device == NULL)
        return FAILURE;
    PitchBend(ch, lsb, msb);
    return SUCCESS;
}

RESULT CMIDIModule::SendChannelPressure(BYTE ch, BYTE velo)
{
    if(m_device == NULL)
        return FAILURE;
    ChannelPressure(ch, velo);
    return SUCCESS;
}

RESULT CMIDIModule::SendPanic()
{
    if(m_device == NULL)
        return FAILURE;
    for(int i = 0; i < 16; ++i)
        SendControlChange(i, 0x7B, 0);
    return SUCCESS;
}

bool CMIDIModule::IsDrum(BYTE ch)
{
    return (m_drum[ch] != 0);
}

RESULT CMIDIModule::SetDrumChannel(int midi_ch, int enable)
{
    m_drum[midi_ch] = enable;
    return SUCCESS;
}
