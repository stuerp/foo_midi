
/** $VER: API.cpp (2023.01.09) P. Stuer **/

#include "pch.h"

#include "MusicKeyboard.h"
#include "API.h"

#pragma hdrstop

namespace foo_vis_midi
{
    class API : public IAPI
    {
    public:
        explicit API() { }

        API(const API &) = delete;
        API & operator=(const API &) = delete;
        API(API &&) = delete;
        API & operator=(API &&) = delete;

        virtual ~API() { }

        IMusicKeyboard::ptr GetMusicKeyboard() const override
        {
            return fb2k::service_new<MusicKeyboard>();
        }
    };

    service_factory_t<API> _APIFactory;
}
