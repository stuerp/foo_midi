
/** $VER: MMF.cpp (2025.04.05) Convert exclusive CHPARAM/OPPARAM (Adapted from mmftool) **/

#include "pch.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MMF.h"

#define SwapDword(data) (*((uint8_t *)(data) + 0) << 24 | *((uint8_t *)(data) + 1) << 16 | *((uint8_t *)(data) + 2) << 8 | *((uint8_t *)(data) + 3))
#define SwapWord(data)  (*((uint8_t *)(data) + 0) << 8 | *((uint8_t *)(data) + 1))

#ifdef Unused

static UINT PutSMFValueEx(uint8_t ** data, DWORD value)
{
    UINT BytesWritten = 0;

    if (0x1FFFFF < value)
        *(*data)++ = (uint8_t)(((value >> 21) & 0x7F) | 0x80), value &= 0x1FFFFF, BytesWritten++;

    if (0x3FFF < value)
        *(*data)++ = (uint8_t)(((value >> 14) & 0x7F) | 0x80), value &= 0x3FFF, BytesWritten++;

    if (0x7F < value)
        *(*data)++ = (uint8_t)(((value >> 7) & 0x7F) | 0x80), value &= 0x7F, BytesWritten++;

    *(*data)++ = (uint8_t)(value & 0x7F), BytesWritten++;

    return BytesWritten;
}

static UINT PutSMFValue(uint8_t * data, DWORD value)
{
    UINT BytesWritten = 0;

    if (0x1FFFFF < value)
        *data++ = (uint8_t)(((value >> 21) & 0x7F) | 0x80), value &= 0x1FFFFF, BytesWritten++;

    if (0x3FFF < value)
        *data++ = (uint8_t)(((value >> 14) & 0x7F) | 0x80), value &= 0x003FFF, BytesWritten++;

    if (0x7F < value)
        *data++ = (uint8_t)(((value >>  7) & 0x7F) | 0x80), value &= 0x00007F, BytesWritten++;

    *data++ = (uint8_t)(value & 0x7F), BytesWritten++;

    return BytesWritten;
}

static UINT CalcSMFValueSize(DWORD value)
{
    UINT BytesWritten = 0;

    if (0x1FFFFF < value)
        value &= 0x1FFFFF, BytesWritten++;

    if (0x3FFF < value)
        value &= 0x3FFF, BytesWritten++;

    if (0x7F < value)
        value &= 0x7F, BytesWritten++;

    BytesWritten++;

    return BytesWritten;
}

static DWORD GetSMFValueEx(uint8_t ** data)
{
    DWORD Value = 0;

    do
    {
        Value <<= 7;

        Value |= **data & 0x7F;
    }
    while (*(*data)++ & 0x80);

    return Value;
}

static DWORD GetSMFValue(const uint8_t * data)
{
    DWORD Value = 0;

    do
    {
        Value <<= 7;
        Value |= *data & 0x7F;
    }
    while (*data++ & 0x80);

    return Value;
}

static ptrdiff_t GetSMFValueSize(const uint8_t * data)
{
    const uint8_t * p = data;

    do
    {
        ;
    }
    while (*p++ & 0x80);

    return p - data;
}


static bool readMA3Exclusive(CHPARAM * chp, OPPARAM * opp, const uint8_t * data)
{
    /*UINT size =*/ GetSMFValue(data);

    data += GetSMFValueSize(data);

    // command check
    if (data[0] != 0x43 || data[1] != 0x79 || data[2] != 0x06 || data[3] != 0x7F || data[4] != 0x01)
        return true;

    chp->bm = data[5];
    chp->bl = data[6];
    chp->pc = data[7];
    chp->na = data[8];
    chp->type = data[9];

    switch (chp->type)
    {
        case VOICE_FM:
        {
            chp->dk  =   data[0x0B];
            chp->lfo = ((data[0x0A] & 0x10u) ? 2u : 0) | ((data[0x0D] & 0x40u) ? 1u : 0);
            chp->pan = ((data[0x0A] & 0x20u) ? 0x10u : 0) | ((data[0x0C] >> 3) & 0x0Fu);
            chp->pe  =  (data[0x0D] & 0x20u) ? TRUE : FALSE;
            chp->alg =   data[0x0D] & 0x07u;

            {
                const uint8_t * d = data + 0x0A;

                for (int i = 0; i < ((chp->alg < 2) ? 2 : 4); i++)
                {
                    opp[i].ar    = ((d[0] & 0x02) ? 0x08u : 0) | ((d[6] >> 4) & 0x07u);
                    opp[i].rr    = ((d[0] & 0x04) ? 0x08u : 0) | ((d[5] >> 4) & 0x07u);
                    opp[i].sr    = ((d[0] & 0x08) ? 0x08u : 0) | ((d[4] >> 4) & 0x07u);
                    opp[i].tl    = ((d[0] & 0x01) ? 0x20u : 0) | ((d[7] >> 2) & 0x1Fu);
                    opp[i].multi = ((d[8] & 0x20) ? 0x08u : 0) | ((d[0x0A] >> 4) & 0x07u);
                    opp[i].dt    = (d[0x0A])   & 0x07u;
                    opp[i].dr    = (d[5])      & 0x0Fu;
                    opp[i].sl    = (d[6])      & 0x0Fu;
                    opp[i].ksl   = (d[7])      & 0x03u;
                    opp[i].dam   = (d[9] >> 5) & 0x03u;
                    opp[i].dvb   = (d[9] >> 1) & 0x03u;
                    opp[i].fb    = (d[0x0B])   & 0x07u;
                    opp[i].ws    = ((d[8] & 0x10) ? 0x10u : 0) | ((d[0x0B] >> 3) & 0x0Fu);

                    opp[i].xof = (d[4] & 0x08) ? TRUE : FALSE;
                    opp[i].sus = (d[4] & 0x02) ? TRUE : FALSE;
                    opp[i].ksr = (d[4] & 0x01) ? TRUE : FALSE;
                    opp[i].eam = (d[9] & 0x10) ? TRUE : FALSE;
                    opp[i].evb = (d[9] & 0x01) ? TRUE : FALSE;

                    d += 8;
                }
            }

            return true;
        }

        case VOICE_PCM:
        {
            chp->fs    = ((data[0x0A] & 0x40u) ? 0x8000 : 0) | ((data[0x0B] & 0x7Fu) << 8) | ((data[0x0A] & 0x20u) ? 0x80u : 0) | (data[0x0C] & 0x7Fu);
            chp->lfo   = ((data[0x0A] & 0x08u) ? 2u : 0) | ((data[0x0E] & 0x40u) ? 1u : 0);
            chp->pan   = ((data[0x0A] & 0x10) ? 0x10u : 0) | ((data[0x0D] >> 3) & 0x0Fu);
            chp->pe    =  (data[0x0D] & 0x01) ? TRUE : FALSE;

            opp->ar    = ((data[0x0A] & 0x01) ? 0x08u : 0) | ((data[0x11] >> 4) & 0x07u);
            opp->dr    =  (data[0x10]) & 0x0Fu;
            opp->sr    = ((data[0x0A] & 0x04) ? 0x08u : 0) | ((data[0x0F] >> 4) & 0x07u);
            opp->rr    = ((data[0x0A] & 0x02) ? 0x08u : 0) | ((data[0x10] >> 4) & 0x07u);
            opp->sl    =  (data[0x11]) & 0x0Fu;
            opp->tl    = ((data[0x12] & 0x40) ? 0x20u : 0) | ((data[0x13] >> 2) & 0x1Fu);
            opp->dam   =  (data[0x14] >> 5) & 0x03u;
            opp->dvb   =  (data[0x14] >> 1) & 0x03u;
            opp->xof   =  (data[0x0F] & 0x08u) ? TRUE : FALSE;
            opp->sus   =  (data[0x0F] & 0x02u) ? TRUE : FALSE;
            opp->eam   =  (data[0x14] & 0x10u) ? TRUE : FALSE;
            opp->evb   =  (data[0x14] & 0x01u) ? TRUE : FALSE;
            chp->lp    =  (data[0x17] << 8) | ((data[0x12] & 0x02u) ? 0x80u : 0) | (data[0x18]);
            chp->ep    =  (data[0x19] << 8) | ((data[0x1A] & 0x40u) ? 0x80u : 0) | (data[0x1B]);
            chp->wavno =  (data[0x1C]) & 0x7Fu;
            chp->rm    =  (data[0x1A] & 0x20) ? TRUE : FALSE;
    
            return true;
        }
    }

    return false;
}

static int readMA5FMParam(OPPARAM * opp, const uint8_t * data)
{
    opp->sr    = (data[0] >> 4) & 0x0Fu;
    opp->xof   = (data[0] & 0x08u) ? TRUE : FALSE;
    opp->sus   = (data[0] & 0x02u) ? TRUE : FALSE;
    opp->ksr   = (data[0] & 0x01u) ? TRUE : FALSE;

    opp->rr    = (data[1] >> 4) & 0x0Fu;
    opp->dr    =  data[1]       & 0x0Fu;

    opp->ar    = (data[2] >> 4) & 0x0Fu;
    opp->sl    =  data[2]       & 0x0Fu;

    opp->tl    = (data[3] >> 2) & 0x3Fu;
    opp->ksl   =  data[3]       & 0x03u;

    opp->dam   = (data[4] >> 5) & 0x03u;
    opp->eam   = (data[4] & 0x10) ? TRUE : FALSE;
    opp->dvb   = (data[4] >> 1) & 0x03u;
    opp->evb   = (data[4] & 0x01) ? TRUE : FALSE;

    opp->multi = (data[5] >> 4) & 0x0Fu;
    opp->dt    =  data[5]       & 0x07u;

    opp->ws    = (data[6] >> 3) & 0x1Fu;
    opp->fb    = data[6]        & 0x07u;

    return 7;
}

static int readMA5PCMParam(OPPARAM * opp, const uint8_t * data)
{
    opp->sr  = (data[0] >> 4) & 0x0Fu;
    opp->xof = (data[0]       & 0x08u) ? TRUE : FALSE;
    opp->sus = (data[0]       & 0x02u) ? TRUE : FALSE;

    opp->rr  = (data[1] >> 4) & 0x0Fu;
    opp->dr  =  data[1]       & 0x0Fu;

    opp->ar  = (data[2] >> 4) & 0x0Fu;
    opp->sl  =  data[2]       & 0x0Fu;

    opp->tl  = (data[3] >> 2) & 0x3Fu;

    opp->dam = (data[4] >> 5) & 0x03u;
    opp->eam = (data[4]       & 0x10u) ? TRUE : FALSE;
    opp->dvb = (data[4] >> 1) & 0x03u;
    opp->evb = (data[4]       & 0x01u) ? TRUE : FALSE;

    return 5;
}

static bool readMA5Exclusive(CHPARAM * chp, OPPARAM * opp, const uint8_t * exdata)
{
    const uint8_t * data = exdata + GetSMFValueSize(exdata);

    UINT size = GetSMFValue(exdata);

    chp->bm = data[5];
    chp->bl = data[6];
    chp->pc = data[7];
    chp->na = data[8];

    // FM
    if (data[9] == 0x00)
    {
        chp->alg = data[0x0C] & 0x07u;
        chp->lfo = (data[0x0C] >> 6) & 0x03u;
        chp->pan = (data[0x0B] >> 2) & 0x3Fu;
        chp->pe = (data[0x0C] & 0x20) ? TRUE : FALSE;
        chp->dk = data[10];

        chp->type = VOICE_FM;

        readMA5FMParam(&opp[0], data + 13);
        readMA5FMParam(&opp[1], data + 13 + 7);

        if (size == 0x2A)
        {
            readMA5FMParam(&opp[2], data + 13 + 7 + 7);
            readMA5FMParam(&opp[3], data + 13 + 7 + 7 + 7);
        }

        return true;
    }

    // PCM
    if (data[9] == 0x01)
    {   // Problems?
        chp->lfo   = (data[0x0D] >> 6) & 0x03u;
        chp->pan   = (data[0x0C] >> 3) & 0x1Fu;
        chp->pe    = (data[0x0C]        & 0x01) ? TRUE : FALSE;

        chp->fs    = (DWORD) SwapWord(data + 0x0A);
        chp->rm    = (data[0x19] & 0x80u) ? TRUE : FALSE;
        chp->wavno =  data[0x19] & 0x7Fu;
        chp->lp    = (DWORD) SwapWord(data + 0x15u);
        chp->ep    = (DWORD) SwapWord(data + 0x17u);

        chp->type  = VOICE_PCM;

        readMA5PCMParam(opp, data + 0x0E);

        return true;
    }

    return false;
}

#endif

static void setExclusiveFMCh(uint8_t * data, CHPARAM * chp)
{
    data[ 0]  = 0xF0;
    data[ 1]  = 0x43;
    data[ 2]  = 0x79;
    data[ 3]  = 0x07;
    data[ 4]  = 0x7F;
    data[ 5]  = 0x01;
    data[ 6]  = chp->bm;
    data[ 7]  = chp->bl;
    data[ 8]  = chp->pc;
    data[ 9]  = chp->na;
    data[12]  = chp->dk;
    data[11] &= ~0x30;
    data[11] |= ((chp->lfo & 0x02) ? 0x10 : 0) | ((chp->pan & 0x10) ? 0x20 : 0);
    data[13] &= ~0x78;
    data[13] |= ((chp->pan & 0x0F) << 3);
    data[14] |= ((chp->lfo & 0x01) ? 0x40 : 0) | ((chp->pe) ? 0x20 : 0) | (chp->alg);
}

static void setExclusiveFMOp(uint8_t * data, OPPARAM * opp)
{
    // X1 XX XX XX 23 45 67 89 AX BC DE FG

    data[ 0] &= ~0x0F;
    data[ 0] |= ((opp->sr & 0x08) ? 8 : 0) | ((opp->rr & 0x08) ? 4 : 0) | ((opp->ar & 0x08) ? 2 : 0) | ((opp->tl & 0x20) ? 1 : 0);

    data[ 4]  = ((opp->sr & 0x07u) << 4) | (opp->xof ? 8 : 0) | (opp->sus ? 2 : 0) | (opp->ksr ? 1 : 0);
    data[ 5]  = ((opp->rr & 0x07u) << 4) | (opp->dr);
    data[ 6]  = ((opp->ar & 0x07u) << 4) | (opp->sl);
    data[ 7]  = ((opp->tl & 0x1Fu) << 2) | (opp->ksl);
    data[ 8] &= ~0x30;
    data[ 8] |= ((opp->multi & 0x08u) ? 0x20 : 0) | ((opp->ws & 0x10u) ? 0x10 : 0);
    data[ 9]  = ((opp->dam) << 5) | ((opp->eam) ? 0x10u : 0) | ((opp->dvb) << 1) | ((opp->evb) ? 1 : 0);
    data[10]  = ((opp->multi & 0x07u) << 4) | (opp->dt);
    data[11]  = ((opp->ws & 0x0Fu) << 3) | opp->fb;
}

static size_t setExclusiveFMAll(uint8_t * data, CHPARAM * chp, OPPARAM * opps)
{
    setExclusiveFMCh(data, chp);

    setExclusiveFMOp(data + 11, opps + 0);
    setExclusiveFMOp(data + 19, opps + 1);

    size_t size;

    if (chp->alg <= 1)
    {
        size = 0x20;
    }
    else
    {
        setExclusiveFMOp(data + 27, opps + 2);
        setExclusiveFMOp(data + 35, opps + 3);

        size = 0x30;
    }

    data[size - 1] = 0xF7;

    return size;
}

static size_t setExclusivePCMAll(uint8_t * data, CHPARAM * chp, OPPARAM * opp)
{
    data[ 0]  = 0xF0;   // Exclusive event

    data[ 1]  = 0x43;
    data[ 2]  = 0x79;
    data[ 3]  = 0x07;
    data[ 4]  = 0x7F;
    data[ 5]  = 0x01;
    data[ 6]  = chp->bm;
    data[ 7]  = chp->bl;
    data[ 8]  = chp->pc;
    data[ 9]  = chp->na;
    data[10]  = 0x01;
    data[11] |= ((chp->pan & 0x10) ? 0x10 : 0) | ((chp->fs & 0x80) ? 0x20 : 0) | ((chp->fs & 0x8000) ? 0x40 : 0);
    data[11] |= ((opp->ar & 0x08) ? 1 : 0) | ((opp->rr & 0x08) ? 2 : 0) | ((opp->sr & 0x08) ? 4 : 0) | ((chp->lfo & 0x02) ? 8 : 0);
    data[12]  = (uint8_t)((chp->fs >> 8) & 0x7F);
    data[13]  = (uint8_t)(chp->fs & 0x7F);
    data[14]  = ((chp->pan & 0x0Fu) << 3) | ((chp->pe) ? 1 : 0);
    data[15]  = (chp->lfo & 0x01u) ? 0x40u : 0;

    data[16]  = ((opp->sr & 0x07u) << 4) | ((opp->xof) ? 8 : 0) | ((opp->sus) ? 2 : 0);
    data[17]  = ((opp->rr & 0x07u) << 4) | (opp->dr);
    data[18]  = ((opp->ar & 0x07u) << 4) | (opp->sl);
    data[19] |= ((opp->tl & 0x20u) ? 0x40 : 0);
    data[19] |= ((chp->lp & 0x80u) ? 2 : 0);
    data[20]  = ((opp->tl) & 0x1Fu) << 2;
    data[21]  = ((opp->dam) << 5) | ((opp->eam) ? 0x10u : 0) | ((opp->dvb) << 1) | ((opp->evb) ? 1 : 0);
    data[24]  = (uint8_t)((chp->lp >> 8) & 0x7F);
    data[25]  = (uint8_t)(chp->lp & 0x7F);
    data[26]  = (uint8_t)((chp->ep >> 8) & 0x7F);
    data[27]  = ((chp->rm) ? 0x20u : 0) | ((chp->ep & 0x80u) ? 0x40u : 0);
    data[28]  = (uint8_t)(chp->ep & 0x7F);
    data[29]  = chp->wavno;

    data[30]  = 0xF7;   // SysEx End

    return 31;
}

static void GetHPSExclusiveFMOp(const uint8_t * data, OPPARAM * opp)
{
    opp->multi = (data[0] >> 4) & 0x0Fu;
    opp->ksl   =  data[3]       & 0x03u;
    opp->tl    = (data[3] >> 2) & 0x3Fu;
    opp->ar    = (data[2] >> 4) & 0x0Fu;
    opp->dr    =  data[1]       & 0x0Fu;
    opp->sl    =  data[2]       & 0x0Fu;
    opp->rr    = (data[1] >> 4) & 0x0Fu;
    opp->ws    =  data[4]       & 0x07u;
    opp->dvb   = (data[4] >> 6) & 0x03u;
    opp->dam   = (data[4] >> 4) & 0x03u;
    opp->evb   = (data[0] & 0x08) ? TRUE : FALSE;
    opp->xof   = 0; //(data[0] & 0x04) ? TRUE : FALSE; // EGT
    opp->sus   = (data[0]       & 0x02u) ? TRUE : FALSE;
    opp->ksr   = (data[0]       & 0x01u) ? TRUE : FALSE;
    opp->eam   = (data[4]       & 0x08u) ? TRUE : FALSE;
    opp->fb    = 0;
    opp->dt    = 0;
    opp->sr    = 0;
}

/// <summary>
/// Gets an HPS exclusive FM message.
/// </summary>
bool GetHPSExclusiveFMMessage(const uint8_t * data, CHPARAM * chp, OPPARAM * opp)
{
    // Maker ID
    if (data[0] != 0x12 && data[0] != 0x1C)
        return false;

    // Format ID
    if (data[1] != 0x43 || data[2] != 0x03 || data[7] != 0x01)
        return false;

    if (data[4] & 0x80)
        return false; // Drums are not supported yet.

    chp->lfo  = (data[6] >> 6) & 0x03u;
    chp->alg  =  data[6]       & 0x07u;
    chp->bm   = (data[4]       & 0x80u) ? 0x7Du : 0x7Cu;
    chp->bl   =  data[4]       & 0x7Fu; // TODO: Fix drums
    chp->pc   =  data[5];
    chp->type = VOICE_FM;
    chp->pan  = 16;
    chp->pe   = FALSE;

    // TODO: Drum support
    chp->na = 0;
    chp->dk = 0;

    // TODO: EGT support
    GetHPSExclusiveFMOp(data + 8, &opp[0]);

    opp[0].fb = (data[6] >> 3) & 0x07u;

    GetHPSExclusiveFMOp(data + 8 + 5, &opp[1]);

    if (data[0] == 0x1C)
    {
        GetHPSExclusiveFMOp(data + 8 + 5 + 5,     &opp[2]);
        GetHPSExclusiveFMOp(data + 8 + 5 + 5 + 5, &opp[3]);
    }

    return true;
}

size_t SetMA3ExclusiveMessage(uint8_t * data, CHPARAM * chp, OPPARAM * opp)
{
    switch (chp->type)
    {
        case VOICE_FM:
            return setExclusiveFMAll(data, chp, opp);

        case VOICE_PCM:
            return setExclusivePCMAll(data, chp, opp);
    }

    return 0;
}
