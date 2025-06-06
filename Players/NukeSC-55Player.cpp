
/** $VER: NukeSC-55Player.cpp (2025.01.12) - MCU code based Nuked SC-55 mcu.cpp **/

#include "framework.h"

#include "NukeSC-55Player.h"
#include "Configuration.h"

#include <PathCch.h>

#pragma comment(lib, "pathcch")

#include <pfc/string-conv-lite.h>

#include <stdio.h>
#include <string.h>

#include <mcu.h>
#include <mcu_opcodes.h>
#include <mcu_interrupt.h>
#include <mcu_timer.h>
#include <pcm.h>
#include <lcd.h>
#include <submcu.h>

#include <midi.h>

#pragma hdrstop

static bool work_thread_run = false;

NukeSC55Player::NukeSC55Player()
{
    _Thread = nullptr;
}

NukeSC55Player::~NukeSC55Player()
{
    SDL_Event Event = { SDL_QUIT };

    ::SDL_PushEvent(&Event);

    Shutdown();
}

void NukeSC55Player::SetBasePath(const std::wstring & basePathName)
{
    _BasePathName = basePathName;
}

#pragma region MCU

mcu_t mcu;

const char * rs_name[ROM_SET_COUNT] =
{
    "SC-55mk2",
    "SC-55st",
    "SC-55mk1",
    "CM-300/SCC-1",
    "JV-880",
    "SCB-55",
    "RLP-3237",
    "SC-155",
    "SC-155mk2"
};

int romset = ROM_SET_MK2;

int mcu_mk1   = false; // 0 - SC-55mkII, SC-55ST. 1 - SC-55, CM-300/SCC-1
int mcu_cm300 = false; // 0 - SC-55, 1 - CM-300/SCC-1
int mcu_st    = false; // 0 - SC-55mk2, 1 - SC-55ST
int mcu_jv880 = false; // 0 - SC-55, 1 - JV880
int mcu_scb55 = false; // 0 - sub mcu (e.g SC-55mk2), 1 - no sub mcu (e.g SCB-55)
int mcu_sc155 = false; // 0 - SC-55(MK2), 1 - SC-155(MK2)

uint8_t dev_register[0x80];

int adf_rd = 0;

uint64_t analog_end_time;

int ssr_rd = 0;

uint32_t uart_write_ptr;
uint32_t uart_read_ptr;
uint8_t uart_buffer[uart_buffer_size];

const int ROM1_SIZE      =  0x8000;
const int ROM2_SIZE      = 0x80000;
const int RAM_SIZE       =   0x400;
const int SRAM_SIZE      =  0x8000;
const int NVRAM_SIZE     =  0x8000; // JV880 only
const int CARDRAM_SIZE   =  0x8000; // JV880 only
const int ROMSM_SIZE     =  0x1000;

uint8_t rom1[ROM1_SIZE];
uint8_t rom2[ROM2_SIZE];
uint8_t ram[RAM_SIZE];
uint8_t sram[SRAM_SIZE];
uint8_t nvram[NVRAM_SIZE];
uint8_t cardram[CARDRAM_SIZE];

int rom2_mask = ROM2_SIZE - 1;

uint8_t mcu_p0_data = 0x00;
uint8_t mcu_p1_data = 0x00;

uint8_t tempbuf[0x800000];

SDL_atomic_t mcu_button_pressed = { 0 };

static const int ROM_SET_N_FILES = 6;

static const wchar_t * roms[ROM_SET_COUNT][ROM_SET_N_FILES] =
{
    {
        L"rom1.bin",
        L"rom2.bin",
        L"waverom1.bin",
        L"waverom2.bin",
        L"rom_sm.bin",
        L"",
    },
    {
        L"rom1.bin",
        L"rom2_st.bin",
        L"waverom1.bin",
        L"waverom2.bin",
        L"rom_sm.bin",
        L"",
    },
    {
        L"sc55_rom1.bin",
        L"sc55_rom2.bin",
        L"sc55_waverom1.bin",
        L"sc55_waverom2.bin",
        L"sc55_waverom3.bin",
        L"",
    },
    {
        L"cm300_rom1.bin",
        L"cm300_rom2.bin",
        L"cm300_waverom1.bin",
        L"cm300_waverom2.bin",
        L"cm300_waverom3.bin",
        L"",
    },
    {
        L"jv880_rom1.bin",
        L"jv880_rom2.bin",
        L"jv880_waverom1.bin",
        L"jv880_waverom2.bin",
        L"jv880_waverom_expansion.bin",
        L"jv880_waverom_pcmcard.bin",
    },
    {
        L"scb55_rom1.bin",
        L"scb55_rom2.bin",
        L"scb55_waverom1.bin",
        L"scb55_waverom2.bin",
        L"",
        L"",
    },
    {
        L"rlp3237_rom1.bin",
        L"rlp3237_rom2.bin",
        L"rlp3237_waverom1.bin",
        L"",
        L"",
        L"",
    },
    {
        L"sc155_rom1.bin",
        L"sc155_rom2.bin",
        L"sc155_waverom1.bin",
        L"sc155_waverom2.bin",
        L"sc155_waverom3.bin",
        L"",
    },
    {
        L"rom1.bin",
        L"rom2.bin",
        L"waverom1.bin",
        L"waverom2.bin",
        L"rom_sm.bin",
        L"",
    },
};

static int audio_buffer_size;
static int audio_page_size;
static short * sample_buffer;

static int sample_read_ptr;
static int sample_write_ptr;

static SDL_mutex * work_thread_lock;
static SDL_AudioDeviceID sdl_audio;

static int ga_int[8];
static int ga_int_enable = 0;
static int ga_int_trigger = 0;
static int ga_lcd_counter = 0;

static uint16_t ad_val[4];
static uint8_t ad_nibble = 0x00;
static uint8_t sw_pos = 3;
static uint8_t io_sd = 0x00;

static uint8_t uart_rx_byte;
static uint64_t uart_rx_delay;
static uint64_t uart_tx_delay;

enum
{
    ANALOG_LEVEL_RCU_LOW = 0,
    ANALOG_LEVEL_RCU_HIGH = 0,
    ANALOG_LEVEL_SW_0 = 0,
    ANALOG_LEVEL_SW_1 = 0x155,
    ANALOG_LEVEL_SW_2 = 0x2aa,
    ANALOG_LEVEL_SW_3 = 0x3ff,
    ANALOG_LEVEL_BATTERY = 0x2a0,
};

static const size_t rf_num = ROM_SET_N_FILES;

static FILE * s_rf[rf_num] =
{
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

enum class ResetType
{
    NONE,
    GS_RESET,
    GM_RESET,
};

void MCU_ErrorTrap(void)
{
    console::printf("%.2x %.4x\n", mcu.cp, mcu.pc);
}

static uint8_t MCU_DeviceRead(uint32_t address)
{
    address &= 0x7f;

    if (address >= 0x10 && address < 0x40)
    {
        return TIMER_Read(address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return TIMER_Read2(address);
    }

    switch (address)
    {
        case DEV_ADDRAH:
        case DEV_ADDRAL:
        case DEV_ADDRBH:
        case DEV_ADDRBL:
        case DEV_ADDRCH:
        case DEV_ADDRCL:
        case DEV_ADDRDH:
        case DEV_ADDRDL:
            return dev_register[address];

        case DEV_ADCSR:
            adf_rd = (dev_register[address] & 0x80) != 0;
            return dev_register[address];

        case DEV_SSR:
            ssr_rd = dev_register[address];
            return dev_register[address];

        case DEV_RDR:
            return uart_rx_byte;

        case 0x00:
            return 0xff;

        case DEV_P7DR:
        {
            if (!mcu_jv880) return 0xff;

            uint8_t data = 0xff;
            uint32_t button_pressed = (uint32_t) ::SDL_AtomicGet(&mcu_button_pressed);

            if (io_sd == 0b11111011)
                data &= ((button_pressed >> 0) & 0b11111) ^ 0xFF;

            if (io_sd == 0b11110111)
                data &= ((button_pressed >> 5) & 0b11111) ^ 0xFF;

            if (io_sd == 0b11101111)
                data &= ((button_pressed >> 10) & 0b1111) ^ 0xFF;

            data |= 0b10000000;

            return data;
        }

        case DEV_P9DR:
        {
            int cfg = 0;

            if (!mcu_mk1)
                cfg = mcu_sc155 ? 0 : 2; // bit 1: 0 - SC-155mk2 (???), 1 - SC-55mk2

            int dir = dev_register[DEV_P9DDR];

            int val = cfg & (dir ^ 0xff);

            val |= dev_register[DEV_P9DR] & dir;

            return (uint8_t) val;
        }
        case DEV_SCR:
        case DEV_TDR:
        case DEV_SMR:
            return dev_register[address];

        case DEV_IPRC:
        case DEV_IPRD:
        case DEV_DTEC:
        case DEV_DTED:
        case DEV_FRT2_TCSR:
        case DEV_FRT1_TCSR:
        case DEV_FRT1_TCR:
        case DEV_FRT1_FRCH:
        case DEV_FRT1_FRCL:
        case DEV_FRT3_TCSR:
        case DEV_FRT3_OCRAH:
        case DEV_FRT3_OCRAL:
            return dev_register[address];
    }

    return dev_register[address];
}

static void MCU_DeviceWrite(uint32_t address, uint8_t data)
{
    address &= 0x7f;

    if (address >= 0x10 && address < 0x40)
    {
        TIMER_Write(address, data);
        return;
    }

    if (address >= 0x50 && address < 0x55)
    {
        TIMER2_Write(address, data);
        return;
    }

    switch (address)
    {
        case DEV_P1DDR: // P1DDR
            break;
        case DEV_P5DDR:
            break;
        case DEV_P6DDR:
            break;
        case DEV_P7DDR:
            break;
        case DEV_SCR:
            break;
        case DEV_WCR:
            break;
        case DEV_P9DDR:
            break;
        case DEV_RAME: // RAME
            break;
        case DEV_P1CR: // P1CR
            break;
        case DEV_DTEA:
            break;
        case DEV_DTEB:
            break;
        case DEV_DTEC:
            break;
        case DEV_DTED:
            break;
        case DEV_SMR:
            break;
        case DEV_BRR:
            break;
        case DEV_IPRA:
            break;
        case DEV_IPRB:
            break;
        case DEV_IPRC:
            break;
        case DEV_IPRD:
            break;
        case DEV_PWM1_DTR:
            break;
        case DEV_PWM1_TCR:
            break;
        case DEV_PWM2_DTR:
            break;
        case DEV_PWM2_TCR:
            break;
        case DEV_PWM3_DTR:
            break;
        case DEV_PWM3_TCR:
            break;
        case DEV_P7DR:
            break;
        case DEV_TMR_TCNT:
            break;
        case DEV_TMR_TCR:
            break;
        case DEV_TMR_TCSR:
            break;
        case DEV_TMR_TCORA:
            break;
        case DEV_TDR:
            break;
        case DEV_ADCSR:
        {
            dev_register[address] &= ~0x7f;
            dev_register[address] |= data & 0x7f;
            if ((data & 0x80) == 0 && adf_rd)
            {
                dev_register[address] &= ~0x80;
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
            }
            if ((data & 0x40) == 0)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 0);
            return;
        }
        case DEV_SSR:
        {
            if ((data & 0x80) == 0 && (ssr_rd & 0x80) != 0)
            {
                dev_register[address] &= ~0x80;
                uart_tx_delay = mcu.cycles + 3000;
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_TX, 0);
            }

            if ((data & 0x40) == 0 && (ssr_rd & 0x40) != 0)
            {
                uart_rx_delay = mcu.cycles + 3000;
                dev_register[address] &= ~0x40;
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_RX, 0);
            }

            if ((data & 0x20) == 0 && (ssr_rd & 0x20) != 0)
            {
                dev_register[address] &= ~0x20;
            }

            if ((data & 0x10) == 0 && (ssr_rd & 0x10) != 0)
            {
                dev_register[address] &= ~0x10;
            }
            break;
        }
        default:
            address += 0;
            break;
    }

    dev_register[address] = data;
}

uint8_t MCU_Read(uint32_t address)
{
    uint32_t address_rom = address & 0x3ffff;

    if (address & 0x80000 && !mcu_jv880)
        address_rom |= 0x40000;

    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    uint8_t ret = 0xff;

    switch (page)
    {
        case 0:
            if (!(address & 0x8000))
                ret = rom1[address & 0x7fff];
            else
            {
                if (!mcu_mk1)
                {
                    uint16_t base = mcu_jv880 ? (uint16_t) 0xf000 : (uint16_t) 0xe000;

                    if ((address >= base) && (address < (base | 0x400U)))
                    {
                        ret = PCM_Read(address & 0x3f);
                    }
                    else
                    if (!mcu_scb55 && address >= 0xec00 && address < 0xf000)
                    {
                        ret = SM_SysRead(address & 0xff);
                    }
                    else
                    if (address >= 0xff80)
                    {
                        ret = MCU_DeviceRead(address & 0x7f);
                    }
                    else
                    if (address >= 0xfb80 && address < 0xff80 && (dev_register[DEV_RAME] & 0x80) != 0)
                        ret = ram[(address - 0xfb80) & 0x3ff];
                    else
                    if (address >= 0x8000 && address < 0xe000)
                    {
                        ret = sram[address & 0x7fff];
                    }
                    else
                    if (address == (base | 0x402U))
                    {
                        ret = (uint8_t) ga_int_trigger;
                        ga_int_trigger = 0;

                        MCU_Interrupt_SetRequest(mcu_jv880 ? INTERRUPT_SOURCE_IRQ0 : INTERRUPT_SOURCE_IRQ1, 0);
                    }
                    else
                    {
                        console::printf("Unknown read %x", address);
                        ret = 0xff;
                    }
                    //
                    // e402:2-0 irq source
                    //
                }
                else
                {
                    if (address >= 0xe000 && address < 0xe040)
                    {
                        ret = PCM_Read(address & 0x3f);
                    }
                    else if (address >= 0xff80)
                    {
                        ret = MCU_DeviceRead(address & 0x7f);
                    }
                    else if (address >= 0xfb80 && address < 0xff80
                        && (dev_register[DEV_RAME] & 0x80) != 0)
                    {
                        ret = ram[(address - 0xfb80) & 0x3ff];
                    }
                    else if (address >= 0x8000 && address < 0xe000)
                    {
                        ret = sram[address & 0x7fff];
                    }
                    else if (address >= 0xf000 && address < 0xf100)
                    {
                        io_sd = address & 0xff;

                        if (mcu_cm300)
                            return 0xff;

                        LCD_Enable((io_sd & 8) != 0);

                        uint8_t data = 0xff;
                        uint32_t button_pressed = (uint32_t) ::SDL_AtomicGet(&mcu_button_pressed);

                        if ((io_sd & 1) == 0)
                            data &= ((button_pressed >> 0) & 255) ^ 255;
                        if ((io_sd & 2) == 0)
                            data &= ((button_pressed >> 8) & 255) ^ 255;
                        if ((io_sd & 4) == 0)
                            data &= ((button_pressed >> 16) & 255) ^ 255;
                        if ((io_sd & 8) == 0)
                            data &= ((button_pressed >> 24) & 255) ^ 255;
                        return data;
                    }
                    else if (address == 0xf106)
                    {
                        ret = (uint8_t) ga_int_trigger;
                        ga_int_trigger = 0;
                        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, 0);
                    }
                    else
                    {
                        console::printf("Unknown read %x", address);
                        ret = 0xff;
                    }
                    //
                    // f106:2-0 irq source
                    //
                }
            }
            break;
    #if 0
        case 3:
            ret = rom2[address | 0x30000];
            break;
        case 4:
            ret = rom2[address];
            break;
        case 10:
            ret = rom2[address | 0x60000]; // FIXME
            break;
        case 1:
            ret = rom2[address | 0x10000];
            break;
    #endif
        case 1:
            ret = rom2[address_rom & rom2_mask];
            break;
        case 2:
            ret = rom2[address_rom & rom2_mask];
            break;
        case 3:
            ret = rom2[address_rom & rom2_mask];
            break;
        case 4:
            ret = rom2[address_rom & rom2_mask];
            break;
        case 8:
            if (!mcu_jv880)
                ret = rom2[address_rom & rom2_mask];
            else
                ret = 0xff;
            break;
        case 9:
            if (!mcu_jv880)
                ret = rom2[address_rom & rom2_mask];
            else
                ret = 0xff;
            break;
        case 14:
        case 15:
            if (!mcu_jv880)
                ret = rom2[address_rom & rom2_mask];
            else
                ret = cardram[address & 0x7fff]; // FIXME
            break;
        case 10:
        case 11:
            if (!mcu_mk1)
                ret = sram[address & 0x7fff]; // FIXME
            else
                ret = 0xff;
            break;

        case 12:
        case 13:
            if (mcu_jv880)
                ret = nvram[address & 0x7fff]; // FIXME
            else
                ret = 0xff;
            break;

        case 5:
            if (mcu_mk1)
                ret = sram[address & 0x7fff]; // FIXME
            else
                ret = 0xff;
            break;

        default:
            ret = 0x00;
            break;
    }

    return ret;
}

uint16_t MCU_Read16(uint32_t address)
{
    address &= ~1;

    uint8_t b0 = MCU_Read(address);
    uint8_t b1 = MCU_Read(address + 1);

    return (uint16_t) ((b0 << 8) + b1);
}

uint32_t MCU_Read32(uint32_t address)
{
    address &= ~3;

    uint8_t b0 = MCU_Read(address);
    uint8_t b1 = MCU_Read(address + 1);
    uint8_t b2 = MCU_Read(address + 2);
    uint8_t b3 = MCU_Read(address + 3);

    return (uint32_t) ((b0 << 24) + (b1 << 16) + (b2 << 8) + b3);
}

void MCU_Write(uint32_t address, uint8_t value)
{
    uint8_t page = (address >> 16) & 0xf;

    address &= 0xffff;

    if (page == 0)
    {
        if (address & 0x8000)
        {
            if (!mcu_mk1)
            {
                uint16_t base = mcu_jv880 ? 0xF000U : 0xE000U;

                if (address >= (base | 0x400U) && address < (base | 0x800U))
                {
                    if (address == (base | 0x404U) || address == (base | 0x405U))
                        LCD_Write(address & 1, value);
                    else
                    if (address == (base | 0x401U))
                    {
                        io_sd = value;
                        LCD_Enable((value & 1) == 0);
                    }
                    else
                    if (address == (base | 0x402U))
                        ga_int_enable = (value << 1);
                    else
                        console::printf("Unknown write %x %x", address, value);
                    //
                    // e400: always 4?
                    // e401: SC0-6?
                    // e402: enable/disable IRQ?
                    // e403: always 1?
                    // e404: LCD
                    // e405: LCD
                    // e406: 0 or 40
                    // e407: 0, e406 continuation?
                    //
                }
                else
                if (address >= (base | 0x000U) && address < (base | 0x400U))
                {
                    PCM_Write(address & 0x3f, value);
                }
                else
                if (!mcu_scb55 && address >= 0xec00U && address < 0xf000U)
                {
                    SM_SysWrite(address & 0xff, value);
                }
                else
                if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    sram[address & 0x7fff] = value;
                }
                else
                {
                    console::printf("Unknown write %x %x", address, value);
                }
            }
            else
            {
                if (address >= 0xe000 && address < 0xe040)
                {
                    PCM_Write(address & 0x3f, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    sram[address & 0x7fff] = value;
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    io_sd = address & 0xff;
                    LCD_Enable((io_sd & 8) != 0);
                }
                else if (address == 0xf105)
                {
                    LCD_Write(0, value);
                    ga_lcd_counter = 500;
                }
                else if (address == 0xf104)
                {
                    LCD_Write(1, value);
                    ga_lcd_counter = 500;
                }
                else if (address == 0xf107)
                {
                    io_sd = value;
                }
                else
                {
                    console::printf("Unknown write %x %x", address, value);
                }
            }
        }
        else
        if (mcu_jv880 && address >= 0x6196 && address <= 0x6199)
        {
            // nop: the jv880 rom writes into the rom at 002E77-002E7D
        }
        else
        {
            console::printf("Unknown write %x %x", address, value);
        }
    }
    else
    if (page == 5 && mcu_mk1)
    {
        sram[address & 0x7fff] = value; // FIXME
    }
    else
    if (page == 10 && !mcu_mk1)
    {
        sram[address & 0x7fff] = value; // FIXME
    }
    else
    if (page == 12 && mcu_jv880)
    {
        nvram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 14 && mcu_jv880)
    {
        cardram[address & 0x7fff] = value; // FIXME
    }
    else
    {
        console::printf("Unknown write %x %x", (page << 16) | address, value);
    }
}

void MCU_Write16(uint32_t address, uint16_t value)
{
    address &= ~1;

    MCU_Write(address,     (uint8_t) (value >> 8));
    MCU_Write(address + 1, (uint8_t) (value & 0xFF));
}

void MCU_PostUART(uint8_t data)
{
    uart_buffer[uart_write_ptr] = data;
    uart_write_ptr = (uart_write_ptr + 1) % uart_buffer_size;
}

void MCU_WorkThread_Lock(void)
{
    ::SDL_LockMutex(work_thread_lock);
}

void MCU_WorkThread_Unlock(void)
{
    ::SDL_UnlockMutex(work_thread_lock);
}

uint8_t MCU_ReadP0(void)
{
    return 0xff;
}

uint8_t MCU_ReadP1(void)
{
    uint8_t data = 0xff;
    uint32_t button_pressed = (uint32_t) ::SDL_AtomicGet(&mcu_button_pressed);

    if ((mcu_p0_data & 1) == 0)
        data &= ((button_pressed >> 0) & 255) ^ 255;
    if ((mcu_p0_data & 2) == 0)
        data &= ((button_pressed >> 8) & 255) ^ 255;
    if ((mcu_p0_data & 4) == 0)
        data &= ((button_pressed >> 16) & 255) ^ 255;
    if ((mcu_p0_data & 8) == 0)
        data &= ((button_pressed >> 24) & 255) ^ 255;

    return data;
}

void MCU_WriteP0(uint8_t data)
{
    mcu_p0_data = data;
}

void MCU_WriteP1(uint8_t data)
{
    mcu_p1_data = data;
}

void MCU_PostSample(int * sample)
{
    sample[0] >>= 15;

    if (sample[0] > INT16_MAX)
        sample[0] = INT16_MAX;
    else
    if (sample[0] < INT16_MIN)
        sample[0] = INT16_MIN;

    sample[1] >>= 15;

    if (sample[1] > INT16_MAX)
        sample[1] = INT16_MAX;
    else
    if (sample[1] < INT16_MIN)
        sample[1] = INT16_MIN;

    sample_buffer[sample_write_ptr + 0] = (short) sample[0];
    sample_buffer[sample_write_ptr + 1] = (short) sample[1];

    sample_write_ptr = (sample_write_ptr + 2) % audio_buffer_size;
}

void MCU_GA_SetGAInt(int line, int value)
{
    // guesswork
    if (value && !ga_int[line] && (ga_int_enable & (1 << line)) != 0)
        ga_int_trigger = line;

    ga_int[line] = value;

    if (mcu_jv880)
        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ0, ga_int_trigger != 0);
    else
        MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_IRQ1, ga_int_trigger != 0);
}

void MCU_EncoderTrigger(int dir)
{
    if (!mcu_jv880) return;
    MCU_GA_SetGAInt(dir == 0 ? 3 : 4, 0);
    MCU_GA_SetGAInt(dir == 0 ? 3 : 4, 1);
}

static uint8_t RCU_Read(void)
{
    return 0;
}

static uint16_t MCU_SC155Sliders(uint32_t index)
{
    // 0 - 1/9
    // 1 - 2/10
    // 2 - 3/11
    // 3 - 4/12
    // 4 - 5/13
    // 5 - 6/14
    // 6 - 7/15
    // 7 - 8/16
    // 8 - ALL
    return 0x0;
}

static uint16_t MCU_AnalogReadPin(uint32_t pin)
{
    if (mcu_cm300)
        return 0;

    if (mcu_jv880)
    {
        if (pin == 1)
            return ANALOG_LEVEL_BATTERY;

        return 0x3ff;
    }

    if (0)
    {
READ_RCU:
        uint8_t rcu = RCU_Read();

        if (rcu & (1 << pin))
            return ANALOG_LEVEL_RCU_HIGH;
        else
            return ANALOG_LEVEL_RCU_LOW;
    }

    if (mcu_mk1)
    {
        if (mcu_sc155 && (dev_register[DEV_P9DR] & 1) != 0)
        {
            return MCU_SC155Sliders(pin);
        }

        if (pin == 7)
        {
            if (mcu_sc155 && (dev_register[DEV_P9DR] & 2) != 0)
                return MCU_SC155Sliders(8);
            else
                return ANALOG_LEVEL_BATTERY;
        }
        else
            goto READ_RCU;
    }
    else
    {
        if (mcu_sc155 && (io_sd & 16) != 0)
        {
            return MCU_SC155Sliders(pin);
        }

        if (pin == 7)
        {
            if (mcu_mk1)
                return ANALOG_LEVEL_BATTERY;

            switch ((io_sd >> 2) & 3)
            {
                case 0: // Battery voltage
                    return ANALOG_LEVEL_BATTERY;

                case 1: // NC
                    if (mcu_sc155)
                        return MCU_SC155Sliders(8);

                    return 0;

                case 2: // SW
                    switch (sw_pos)
                    {
                        default:
                        case 0:
                            return ANALOG_LEVEL_SW_0;

                        case 1:
                            return ANALOG_LEVEL_SW_1;

                        case 2:
                            return ANALOG_LEVEL_SW_2;

                        case 3:
                            return ANALOG_LEVEL_SW_3;
                    }

                case 3: // RCU
                    goto READ_RCU;
            }
        }
        else
            goto READ_RCU;
    }
}

static void MCU_AnalogSample(int channel)
{
    int value = MCU_AnalogReadPin((uint32_t) channel);
    int dest = (channel << 1) & 6;

    dev_register[DEV_ADDRAH + dest] = (uint8_t) (value >> 2);
    dev_register[DEV_ADDRAL + dest] = (uint8_t) ((value << 6) & 0xc0);
}

static void MCU_DeviceReset(void)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
    dev_register[DEV_RAME] = 0x80;
    dev_register[DEV_SSR] = 0x80;
}

static void MCU_UpdateAnalog(uint64_t cycles)
{
    int ctrl = dev_register[DEV_ADCSR];
    int isscan = (ctrl & 16) != 0;

    if (ctrl & 0x20)
    {
        if (analog_end_time == 0)
            analog_end_time = cycles + 200;
        else if (analog_end_time < cycles)
        {
            if (isscan)
            {
                int base = ctrl & 4;
                for (int i = 0; i <= (ctrl & 3); i++)
                    MCU_AnalogSample(base + i);
                analog_end_time = cycles + 200;
            }
            else
            {
                MCU_AnalogSample(ctrl & 7);
                dev_register[DEV_ADCSR] &= ~0x20;
                analog_end_time = 0;
            }
            dev_register[DEV_ADCSR] |= 0x80;
            if (ctrl & 0x40)
                MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        analog_end_time = 0;
}

static void MCU_ReadInstruction(void)
{
    uint8_t operand = MCU_ReadCodeAdvance();

    MCU_Operand_Table[operand](operand);

    if (mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(EXCEPTION_SOURCE_TRACE);
    }
}

static void MCU_Init(void)
{
    memset(&mcu, 0, sizeof(mcu_t));
}

static void MCU_Reset(void)
{
    mcu.r[0] = 0;
    mcu.r[1] = 0;
    mcu.r[2] = 0;
    mcu.r[3] = 0;
    mcu.r[4] = 0;
    mcu.r[5] = 0;
    mcu.r[6] = 0;
    mcu.r[7] = 0;

    mcu.pc = 0;

    mcu.sr = 0x700;

    mcu.cp = 0;
    mcu.dp = 0;
    mcu.ep = 0;
    mcu.tp = 0;
    mcu.br = 0;

    uint32_t reset_address = MCU_GetVectorAddress(VECTOR_RESET);
    mcu.cp = (reset_address >> 16) & 0xff;
    mcu.pc = reset_address & 0xffff;

    mcu.exception_pending = -1;

    MCU_DeviceReset();

    if (mcu_mk1)
        ga_int_enable = 255;
}

static void MCU_UpdateUART_RX(void)
{
    if ((dev_register[DEV_SCR] & 16) == 0) // RX disabled
        return;
    if (uart_write_ptr == uart_read_ptr) // no byte
        return;

    if (dev_register[DEV_SSR] & 0x40)
        return;

    if (mcu.cycles < uart_rx_delay)
        return;

    uart_rx_byte = uart_buffer[uart_read_ptr];
    uart_read_ptr = (uart_read_ptr + 1) % uart_buffer_size;
    dev_register[DEV_SSR] |= 0x40;
    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_RX, (dev_register[DEV_SCR] & 0x40) != 0);
}

static void MCU_UpdateUART_TX(void)
{
    if ((dev_register[DEV_SCR] & 32) == 0) // TX disabled
        return;

    if (dev_register[DEV_SSR] & 0x80)
        return;

    if (mcu.cycles < uart_tx_delay)
        return;

    dev_register[DEV_SSR] |= 0x80;
    MCU_Interrupt_SetRequest(INTERRUPT_SOURCE_UART_TX, (dev_register[DEV_SCR] & 0x80) != 0);

    // printf("tx:%x\n", dev_register[DEV_TDR]);
}

static int SDLCALL work_thread(void * data) noexcept
{
    work_thread_lock = ::SDL_CreateMutex();

    MCU_WorkThread_Lock();

    while (work_thread_run)
    {
        if (pcm.config_reg_3c & 0x40)
            sample_write_ptr &= ~3;
        else
            sample_write_ptr &= ~1;

        if (sample_read_ptr == sample_write_ptr)
        {
            MCU_WorkThread_Unlock();

            while (sample_read_ptr == sample_write_ptr)
            {
                ::SDL_Delay(1);
            }

            MCU_WorkThread_Lock();
        }

        if (!mcu.ex_ignore)
            MCU_Interrupt_Handle();
        else
            mcu.ex_ignore = 0;

        if (!mcu.sleep)
            MCU_ReadInstruction();

        mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

//      if (mcu.cycles % 24000000 == 0)
//          printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

        PCM_Update(mcu.cycles);

        TIMER_Clock(mcu.cycles);

        if (!mcu_mk1 && !mcu_jv880 && !mcu_scb55)
            SM_Update(mcu.cycles);
        else
        {
            MCU_UpdateUART_RX();
            MCU_UpdateUART_TX();
        }

        MCU_UpdateAnalog(mcu.cycles);

        if (mcu_mk1)
        {
            if (ga_lcd_counter != 0)
            {
                ga_lcd_counter--;

                if (ga_lcd_counter == 0)
                {
                    MCU_GA_SetGAInt(1, 0);
                    MCU_GA_SetGAInt(1, 1);
                }
            }
        }
    }

    MCU_WorkThread_Unlock();

    ::SDL_DestroyMutex(work_thread_lock);

    return 0;
}

static void MCU_PatchROM(void)
{
    //rom2[0x1333] = 0x11;
    //rom2[0x1334] = 0x19;
    //rom1[0x622d] = 0x19;
}

static void unscramble(uint8_t * src, uint8_t * dst, int len)
{
    for (int i = 0; i < len; i++)
    {
        int address = i & ~0xfffff;
        static const int aa[] = { 2, 0, 3, 4, 1, 9, 13, 10, 18, 17, 6, 15, 11, 16, 8, 5, 12, 7, 14, 19 };

        for (int j = 0; j < 20; j++)
        {
            if (i & (1 << j))
                address |= 1 << aa[j];
        }

        uint8_t srcdata = src[address];
        uint8_t data = 0;
        static const int dd[] = {
            2, 0, 4, 5, 7, 6, 3, 1
        };

        for (int j = 0; j < 8; j++)
        {
            if (srcdata & (1 << dd[j]))
                data |= 1 << j;
        }
        dst[i] = data;
    }
}

static void audio_callback(void * /*userdata*/, Uint8 * stream, int len)
{
    len /= 2;

    ::memcpy(stream, &sample_buffer[sample_read_ptr], (size_t) len * sizeof(*sample_buffer));
    ::memset(&sample_buffer[sample_read_ptr], 0,      (size_t) len * sizeof(*sample_buffer));

    sample_read_ptr += len;
    sample_read_ptr %= audio_buffer_size;
}

static const char * audio_format_to_str(int format)
{
    switch (format)
    {
        case AUDIO_S8:
            return "S8";

        case AUDIO_U8:
            return "U8";

        case AUDIO_S16MSB:
            return "S16MSB";

        case AUDIO_S16LSB:
            return "S16LSB";

        case AUDIO_U16MSB:
            return "U16MSB";

        case AUDIO_U16LSB:
            return "U16LSB";

        case AUDIO_S32MSB:
            return "S32MSB";

        case AUDIO_S32LSB:
            return "S32LSB";

        case AUDIO_F32MSB:
            return "F32MSB";

        case AUDIO_F32LSB:
            return "F32LSB";
    }

    return "UNK";
}

static int MCU_OpenAudio(int deviceIndex, int pageSize, int pageNum)
{
    audio_page_size = (pageSize / 2) * 2; // must be even
    audio_buffer_size = audio_page_size * pageNum;

    sample_buffer = (short *) ::calloc((size_t) audio_buffer_size, sizeof(short));

    if (sample_buffer == nullptr)
        return 0; // "Cannot allocate audio buffer."

    sample_read_ptr  = 0;
    sample_write_ptr = 0;
/*
    SDL_AudioSpec AudioSpecReq = {};

    AudioSpecReq.format   = AUDIO_S16SYS;
    AudioSpecReq.freq     = (mcu_mk1 || mcu_jv880) ? 64000 : 66207;
    AudioSpecReq.channels = 2;
    AudioSpecReq.callback = audio_callback;
    AudioSpecReq.samples  = (Uint16) (audio_page_size / 4);

    int AudioDeviceCount = ::SDL_GetNumAudioDevices(0);

    if (AudioDeviceCount == 0)
        return 0; // "No audio output device found."

    if (deviceIndex < -1 || deviceIndex >= AudioDeviceCount)
    {
        console::printf("Out of range audio device index is requested. Default audio output device is selected.");
        deviceIndex = -1;
    }

    const char * audioDevicename = (deviceIndex == -1) ? "Default device" : SDL_GetAudioDeviceName(deviceIndex, 0);

    SDL_AudioSpec AudioSpecAct = {};

    sdl_audio = ::SDL_OpenAudioDevice(deviceIndex == -1 ? NULL : audioDevicename, 0, &AudioSpecReq, &AudioSpecAct, 0);

    if (!sdl_audio)
        return 0;

    console::printf("Audio device   : %s", audioDevicename);
    console::printf("Audio Requested: Format: %s, No. of channels: %d, Frequency: %d, No. of samples: %d", audio_format_to_str(AudioSpecReq.format), AudioSpecReq.channels, AudioSpecReq.freq, AudioSpecReq.samples);
    console::printf("Audio Actual   : Format: %s, No. of channels: %d, Frequency: %d, No. of samples: %d", audio_format_to_str(AudioSpecAct.format), AudioSpecAct.channels, AudioSpecAct.freq, AudioSpecAct.samples);

    ::SDL_PauseAudioDevice(sdl_audio, 0);
*/
    return 1;
}

static void MCU_CloseAudio(void)
{
/*
    ::SDL_CloseAudio();
*/
    if (sample_buffer)
    {
        ::free(sample_buffer);
        sample_buffer = nullptr;
    }
}

static void MIDI_Reset(ResetType resetType)
{
    if (resetType == ResetType::GS_RESET)
    {
        const unsigned char gsReset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };

        for (size_t i = 0; i < sizeof(gsReset); ++i)
            MCU_PostUART(gsReset[i]);
    }
    else
    if (resetType == ResetType::GM_RESET)
    {
        const unsigned char gmReset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };

        for (size_t i = 0; i < sizeof(gmReset); ++i)
            MCU_PostUART(gmReset[i]);
    }
}

/// <summary>
/// Initializes the MCU.
/// </summary>
void NukeSC55Player::Initialize()
{
    ResetType resetType = ResetType::NONE;

    romset = ROM_SET_MK2;

    int audioDeviceIndex = -1;
    int pageSize = 512;
    int pageNum = 32;

//  int MIDIPort = 0;

#ifdef later
    bool autodetect = true;

    {
        for (int i = 1; i < argc; ++i)
        {
            if (!strncmp(argv[i], "-p:", 3))
            {
                MIDIPort = atoi(argv[i] + 3);
            }
            else
            if (!strncmp(argv[i], "-a:", 3))
            {
                audioDeviceIndex = atoi(argv[i] + 3);
            }
            else
            if (!strncmp(argv[i], "-ab:", 4))
            {
                char * pColon = argv[i] + 3;

                if (pColon[1] != 0)
                {
                    pageSize = atoi(++pColon);
                    pColon = strchr(pColon, ':');
                    if (pColon && pColon[1] != 0)
                    {
                        pageNum = atoi(++pColon);
                    }
                }

                // reset both if either is invalid
                if (pageSize <= 0 || pageNum <= 0)
                {
                    pageSize = 512;
                    pageNum = 32;
                }
            }
            else
            if (!strcmp(argv[i], "-mk2"))
            {
                romset = ROM_SET_MK2;
                autodetect = false;
            }
            else
            if (!strcmp(argv[i], "-st"))
            {
                romset = ROM_SET_ST;
                autodetect = false;
            }
            else
            if (!strcmp(argv[i], "-mk1"))
            {
                romset = ROM_SET_MK1;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-cm300"))
            {
                romset = ROM_SET_CM300;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-jv880"))
            {
                romset = ROM_SET_JV880;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-scb55"))
            {
                romset = ROM_SET_SCB55;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-rlp3237"))
            {
                romset = ROM_SET_RLP3237;
                autodetect = false;
            }
            else if (!strcmp(argv[i], "-gs"))
            {
                resetType = ResetType::GS_RESET;
            }
            else if (!strcmp(argv[i], "-gm"))
            {
                resetType = ResetType::GM_RESET;
            }
            else
            if (!strcmp(argv[i], "-sc155"))
            {
                romset = ROM_SET_SC155;
                autodetect = false;
            }
            else
            if (!strcmp(argv[i], "-sc155mk2"))
            {
                romset = ROM_SET_SC155MK2;
                autodetect = false;
            }
            else
            if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
            {
                // TODO: Might want to try to find a way to print out the executable's actual name (without any full paths).
                printf("Usage: nuked-sc55 [options]\n");
                printf("Options:\n");
                printf("  -h, -help, --help              Display this information.\n");
                printf("\n");
                printf("  -p:<port_number>               Set MIDI port.\n");
                printf("  -a:<device_number>             Set Audio Device index.\n");
                printf("  -ab:<page_size>:[page_count]   Set Audio Buffer size.\n");
                printf("\n");
                printf("  -mk2                           Use SC-55mk2 ROM set.\n");
                printf("  -st                            Use SC-55st ROM set.\n");
                printf("  -mk1                           Use SC-55mk1 ROM set.\n");
                printf("  -cm300                         Use CM-300/SCC-1 ROM set.\n");
                printf("  -jv880                         Use JV-880 ROM set.\n");
                printf("  -scb55                         Use SCB-55 ROM set.\n");
                printf("  -rlp3237                       Use RLP-3237 ROM set.\n");
                printf("\n");
                printf("  -gs                            Reset system in GS mode.\n");
                printf("  -gm                            Reset system in GM mode.\n");
                return 0;
            }
        }
    }

    std::string basePath = Files::real_dirname(argv[0]);

    printf("Base path is: %s\n", argv[0]);

    if (Files::dirExists(basePath + "/../share/nuked-sc55"))
        basePath += "/../share/nuked-sc55";

    if (autodetect)
    {
        for (size_t i = 0; i < ROM_SET_COUNT; i++)
        {
            bool FoundAllFiles = true;

            for (size_t j = 0; j < 5; j++)
            {
                if (roms[i][j][0] == '\0')
                    continue;

                std::string path = basePath + "/" + roms[i][j];

                auto h = Files::utf8_fopen(path.c_str(), "rb");

                if (!h)
                {
                    FoundAllFiles = false;
                    break;
                }

                fclose(h);
            }

            if (FoundAllFiles)
            {
                romset = i;
                break;
            }
        }

        printf("ROM set autodetect: %s\n", rs_name[romset]);
    }
#endif

    mcu_mk1   = false;
    mcu_cm300 = false;
    mcu_st    = false;
    mcu_jv880 = false;
    mcu_scb55 = false;
    mcu_sc155 = false;

    switch (romset)
    {
        case ROM_SET_MK2:
        case ROM_SET_SC155MK2:
            if (romset == ROM_SET_SC155MK2)
                mcu_sc155 = true;
            break;

        case ROM_SET_ST:
            mcu_st = true;
            break;

        case ROM_SET_MK1:
        case ROM_SET_SC155:
            mcu_mk1 = true;
            mcu_st = false;

            if (romset == ROM_SET_SC155)
                mcu_sc155 = true;
            break;

        case ROM_SET_CM300:
            mcu_mk1 = true;
            mcu_cm300 = true;
            break;

        case ROM_SET_JV880:
            mcu_jv880 = true;
            rom2_mask /= 2; // rom is half the size

            lcd_width  = 820;
            lcd_height = 100;

            lcd_col1 = 0x000000;
            lcd_col2 = 0x78b500;
            break;

        case ROM_SET_SCB55:
        case ROM_SET_RLP3237:
            mcu_scb55 = true;
            break;
    }

#ifdef later
    std::string rpaths[ROM_SET_N_FILES];

    bool r_ok = true;
    std::string errors_list;

    for (size_t i = 0; i < ROM_SET_N_FILES; ++i)
    {
        if (roms[romset][i][0] == '\0')
        {
            rpaths[i] = "";
            continue;
        }

        rpaths[i] = basePath + "/" + roms[romset][i];

        s_rf[i] = Files::utf8_fopen(rpaths[i].c_str(), "rb");

        bool optional = mcu_jv880 && i >= 4;
        r_ok &= optional || (s_rf[i] != nullptr);

        if (!s_rf[i])
        {
            if (!errors_list.empty())
                errors_list.append(", ");

            errors_list.append(rpaths[i]);
        }
    }

    if (!r_ok)
    {
        fprintf(stderr, "FATAL ERROR: One of required data ROM files is missing: %s.\n", errors_list.c_str());
        fflush(stderr);
        closeAllR();

        return 1;
    }
#endif

    LCD_SetBackPath(pfc::utf8FromWide((_BasePathName + L"/back.data").c_str()).c_str());

    mcu = {};

    size_t Size = ReadROM(roms[romset][0], rom1, ROM1_SIZE);

    Size = ReadROM(roms[romset][1], rom2, ROM2_SIZE);

    if (Size == ROM2_SIZE || Size == ROM2_SIZE / 2)
        rom2_mask = (int) (Size - 1);

    if (mcu_mk1)
    {
        Size = ReadROM(roms[romset][2], tempbuf, 0x100000);

        unscramble(tempbuf, waverom1, 0x100000);

        Size = ReadROM(roms[romset][3], tempbuf, 0x100000);

        unscramble(tempbuf, waverom2, 0x100000);

        Size = ReadROM(roms[romset][4], tempbuf, 0x100000);

        unscramble(tempbuf, waverom3, 0x100000);
    }
    else
    if (mcu_jv880)
    {
        Size = ReadROM(roms[romset][2], tempbuf, 0x200000);

        unscramble(tempbuf, waverom1, 0x200000);

        Size = ReadROM(roms[romset][3], tempbuf, 0x200000);

        unscramble(tempbuf, waverom2, 0x200000);

        Size = ReadROM(roms[romset][4], tempbuf, 0x800000); // Optional

        unscramble(tempbuf, waverom_exp, 0x800000);

        Size = ReadROM(roms[romset][5], tempbuf, 0x200000); // Optional

        unscramble(tempbuf, waverom_exp, 0x200000);
    }
    else
    {
        Size = ReadROM(roms[romset][2], tempbuf, 0x200000);

        unscramble(tempbuf, waverom1, 0x200000);

        Size = ReadROM(roms[romset][3], tempbuf, 0x100000);

        unscramble(tempbuf, mcu_scb55 ? waverom3 : waverom2, 0x100000);

        Size = ReadROM(roms[romset][4], sm_rom, ROMSM_SIZE);
    }

    if (::SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
        return; // fprintf(stderr, "FATAL ERROR: Failed to initialize the SDL2: %s.\n", SDL_GetError());

    if (!MCU_OpenAudio(audioDeviceIndex, pageSize, pageNum))
        return; // fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
/*
    if (!MIDI_Init(MIDIPort))
    {
        fprintf(stderr, "ERROR: Failed to initialize the MIDI Input.\nWARNING: Continuing without MIDI Input...\n");
        fflush(stderr);
    }
*/
    LCD_Init();

    MCU_Init();
    MCU_PatchROM();
    MCU_Reset();
    SM_Reset();
    PCM_Reset();

    if (resetType != ResetType::NONE)
        MIDI_Reset(resetType);

    work_thread_run = true;

    _Thread = SDL_CreateThread(work_thread, "work thread", 0);
}

/// <summary>
/// Terminates the MCU.
/// </summary>
void NukeSC55Player::Terminate()
{
    work_thread_run = false;

    ::SDL_WaitThread(_Thread, 0);
    _Thread = nullptr;

    LCD_UnInit();

//  MIDI_Quit();

    MCU_CloseAudio();

    ::SDL_Quit();
}

/// <summary>
/// Reads the specified ROM.
/// </summary>
size_t NukeSC55Player::ReadROM(const std::wstring & fileName, uint8_t * data, size_t size) const noexcept
{
    std::wstring FilePath;

    FilePath.resize(32768);

    HRESULT hResult = ::PathCchCombine((WCHAR *) FilePath.c_str(), FilePath.size(), _BasePathName.c_str(), fileName.c_str());

    if (!SUCCEEDED(hResult))
        return 0;

    HANDLE hFile = ::CreateFileW(FilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD BytesRead;

    if (!(::ReadFile(hFile, data, (DWORD) size, &BytesRead, nullptr) && ((DWORD) size == BytesRead)))
        BytesRead = 0;

    ::CloseHandle(hFile);

    return BytesRead;
}

#pragma endregion

#pragma region player_t

bool NukeSC55Player::Startup()
{
    if (work_thread_run)
        return true;

    Initialize();

    return true;
}

void NukeSC55Player::Shutdown()
{
    Terminate();
}

void NukeSC55Player::Render(audio_sample * sampleData, uint32_t sampleSize)
{
    static const uint32_t MaxSamples = (sample_write_ptr + audio_buffer_size) - sample_read_ptr;

    while (sampleSize != 0)
    {
        const size_t ToDo = std::min(sampleSize, MaxSamples);
        const size_t SampleCount = ToDo * 2;

        const short * Src = &sample_buffer[sample_read_ptr];

        for (size_t i = 0; i < SampleCount; ++i)
            sampleData[i] = (audio_sample) *Src++ * (1.0f / 32768.0f);

        sampleData += SampleCount;
        sampleSize -= (uint32_t) ToDo;

        sample_read_ptr = (sample_read_ptr + (int) SampleCount) % audio_buffer_size;

console::printf("%8d/%8d-%8d/%8d", sample_read_ptr, sample_write_ptr, ToDo, MaxSamples);
    }
}

/// <summary>
/// Sends a message to the emulator.
/// </summary>
void NukeSC55Player::SendEvent(uint32_t data)
{
//  uint8_t PortNumber = (uint8_t) ((data >> 24) & 0x7F);
    uint8_t Param2     = (uint8_t) ((data >> 16) & 0xFF);
    uint8_t Param1     = (uint8_t) ((data >>  8) & 0xFF);
    uint8_t Status     = (uint8_t)  (data        & 0xFF);

    MCU_PostUART(Status);

    switch (Status & 0xF0)
    {
        case StatusCodes::NoteOff:
            MCU_PostUART(Param1);
            break;

        case StatusCodes::NoteOn:
            MCU_PostUART(Param1);
            MCU_PostUART(Param2);
            break;

        case StatusCodes::KeyPressure:
            break;

        case StatusCodes::ControlChange:
            MCU_PostUART(Param1);
            MCU_PostUART(Param2);
            break;

        case StatusCodes::ProgramChange:
            MCU_PostUART(Param1);
            break;

        case StatusCodes::ChannelPressure:
            MCU_PostUART(Param1);
            break;

        case StatusCodes::PitchBendChange:
            MCU_PostUART(Param1);
            MCU_PostUART(Param2);
            break;
    }
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void NukeSC55Player::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    for (; size != 0; --size)
        MCU_PostUART(*data++);
}

#pragma endregion
