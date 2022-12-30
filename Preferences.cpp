
/** $VER: Preferences.cpp (2022.12.30) **/

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include <map>

#include <sdk/foobar2000.h>
#include <sdk/preferences_page.h>
#include <sdk/coreDarkMode.h>

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include "resource.h"

#include "Configuration.h"
#include "Fields.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "VSTiPlayer.h"

extern char _DLLFileName[];
extern volatile int g_running;

static const GUID guid_cfg_history_rate = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate, 16);

static const char * click_to_set = "Click to set.";

static const int SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

static const char * loop_txt[] =
{
    "Never loop",
    "Never, add 1s decay time",
    "Loop and fade when detected",
    "Always loop and fade",
    "Play indefinitely when detected",
    "Play indefinitely"
};

#ifdef FLUIDSYNTHSUPPORT
static const char * interp_txt[] = { "None", "Linear", "Cubic", "7th Order Sinc" };
static int interp_method[] = { FLUID_INTERP_NONE, FLUID_INTERP_LINEAR, FLUID_INTERP_4THORDER, FLUID_INTERP_7THORDER };
enum
{
    interp_method_default = 2
};
#endif

class Preferences : public CDialogImpl<Preferences>, public preferences_page_instance
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    Preferences(preferences_page_callback::ptr callback) : m_callback(callback), busy(false)
    {
    }

    // Note that we don't bother doing anything regarding destruction of our class.
    // The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.

    // dialog resource ID
    enum
    {
        IDD = IDD_CONFIG
    };
    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state();
    void apply();
    void reset();

    enum
    {
        ID_REFRESH = 1000
    };

    // WTL message map
    BEGIN_MSG_MAP(Preferences)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_PLUGIN, CBN_SELCHANGE, OnPluginChange)
        COMMAND_HANDLER_EX(IDC_SOUNDFONT, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_MUNT, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, cfg_history_rate)
        COMMAND_HANDLER_EX(IDC_LOOP, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_CLOOP, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_RESAMPLING, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_RPGMLOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_PLUGIN_CONFIGURE, BN_CLICKED, OnButtonConfig)
        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_PANNING, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MUNT_GM, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MS_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_FILTER_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MS_PANNING, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_EFFECTS, BN_CLICKED, OnButtonClick)
        MSG_WM_TIMER(OnTimer)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
    void OnPluginChange(UINT, int, CWindow);
    void OnButtonClick(UINT, int, CWindow);
    void OnButtonConfig(UINT, int, CWindow);
    void OnSetFocus(UINT, int, CWindow);
    void OnTimer(UINT_PTR nIDEvent);
    bool HasChanged();
    void OnChanged();

    void enum_vsti_plugins(const char * _path = 0, puFindFile _find = 0);

    static bool check_secret_sauce();

    const preferences_page_callback::ptr m_callback;

    bool busy, secret_sauce_found;

#ifdef DXISUPPORT
    pfc::array_t<CLSID> dxi_plugins;
#endif

    struct vsti_info
    {
        std::string path, display_name;
        uint32_t unique_id;
        bool has_editor;
    };

    pfc::array_t<vsti_info> vsti_plugins;

    std::vector<uint8_t> vsti_config;

    pfc::string8 m_soundfont, m_munt_path;

#ifdef BASSMIDISUPPORT
    pfc::string8_fast m_cached, m_cached_current;
#endif

    struct adl_bank
    {
        int number;
        const char * name;

        adl_bank()
            : number(-1), name("")
        {
        }
        adl_bank(const adl_bank & b)
            : number(b.number), name(b.name)
        {
        }
        adl_bank(int _number, const char * _name)
            : number(_number), name(_name)
        {
        }

        adl_bank & operator=(const adl_bank & b)
        {
            number = b.number;
            name = b.name;
            return *this;
        }

        bool operator==(const adl_bank & b) const
        {
            return number == b.number;
        }
        bool operator<(const adl_bank & b) const
        {
            int c = stricmp_utf8(name, b.name);
            if (c) return c < 0;
            return 0;
        }
        bool operator>(const adl_bank & b) const
        {
            int c = stricmp_utf8(name, b.name);
            if (c) return c > 0;
            return 0;
        }
        bool operator!=(const adl_bank & b) const
        {
            return !operator==(b);
        }
    };

    pfc::list_t<adl_bank> m_bank_list;

    struct plugin_names_ids
    {
        const char * name;
        int plugin_number;
        int plugin_number_alt;
        bool (*plugin_present)(Preferences *);
    };

    static bool plugin_always_present(Preferences *)
    {
        return true;
    }

    static bool plugin_never_present(Preferences *)
    {
        return false;
    }

    static bool plugin_sauce_present(Preferences * pthis)
    {
        return pthis->secret_sauce_found;
    }

    int plugins_reported;

    static const plugin_names_ids Plugins[];
    std::map<int, int> plugins_reverse_map, plugins_present_map, plugins_present_reverse_map;
    fb2k::CCoreDarkModeHooks m_hooks;
};

void Preferences::enum_vsti_plugins(const char * _path, puFindFile _find)
{
    pfc::string8 ppath;
    if (!_find)
    {
        vsti_plugins.set_size(0);

        cfg_vsti_search_path.get(ppath);
        if (ppath.is_empty())
        {
            GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_SHOW);
            GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
            return;
        }

        console::print("Enumerating VSTi Plug-ins...");

        if (ppath[ppath.length() - 1] != '\\') ppath.add_byte('\\');
        ppath += "*.*";
        _path = ppath;
        _find = uFindFirstFile(ppath);
    }
    if (_find)
    {
        do
        {
            if (_find->IsDirectory() && strcmp(_find->GetFileName(), ".") && strcmp(_find->GetFileName(), ".."))
            {
                pfc::string8 npath(_path);
                npath.truncate(npath.length() - 3);
                npath += _find->GetFileName();
                npath.add_byte('\\');
                npath += "*.*";
                puFindFile pfind = uFindFirstFile(npath);
                if (pfind) enum_vsti_plugins(npath, pfind);
            }
            else if (_find->GetFileSize())
            {
                pfc::string8 npath(_path);
                npath.truncate(npath.length() - 3);
                npath += _find->GetFileName();
                if (npath.length() > 4 && !pfc::stricmp_ascii(npath.get_ptr() + npath.length() - 4, ".dll"))
                {
                    {
                        pfc::string8 temp;
                        temp = "Trying ";
                        temp += npath;
                        console::print(temp);
                    }

                    VSTiPlayer vstPlayer;

                    if (vstPlayer.LoadVST(npath))
                    {
                        vsti_info info;
                        info.path = npath;

                        std::string vendor, product;
                        vstPlayer.getVendorString(vendor);
                        vstPlayer.getProductString(product);

                        if (product.length() || vendor.length())
                        {
                            if (!vendor.length() ||
                                (product.length() >= vendor.length() &&
                                !strncmp(vendor.c_str(), product.c_str(), vendor.length())))
                            {
                                info.display_name = product;
                            }
                            else
                            {
                                info.display_name = vendor;
                                if (product.length())
                                {
                                    info.display_name += ' ';
                                    info.display_name += product;
                                }
                            }
                        }
                        else
                            info.display_name = _find->GetFileName();

                        info.unique_id = (uint32_t)vstPlayer.getUniqueID();

                        info.has_editor = vstPlayer.hasEditor();

                        vsti_plugins.append_single(info);
                    }
                }
            }
        }
        while (_find->FindNext());
        delete _find;
    }
}

typedef struct sc_info
{
    size_t size;
    hasher_md5_result hash;
} sc_info;

static const sc_info sc_hashes[] =
{
    // 1.0.3 - 32 bit - 27,472,384 - d44d1b8c9a6f956ca2324f2f5d348c44
    { 27472384, { (char) 0xd4, 0x4d, 0x1b, (char) 0x8c, (char) 0x9a, 0x6f, (char) 0x95, 0x6c, (char) 0xa2, 0x32, 0x4f, 0x2f, 0x5d, 0x34, (char) 0x8c, 0x44 } },

    // 1.0.3 - 64 bit - 27,440,128 - f16b5eb9c7e204de7f9b3a829d2d5500
    { 27440128, { (char) 0xf1, 0x6b, 0x5e, (char) 0xb9, (char) 0xc7, (char) 0xe2, 0x04, (char) 0xde, 0x7f, (char) 0x9b, 0x3a, (char) 0x82, (char) 0x9d, 0x2d, 0x55, 0x00 } },

    // 1.0.6 - 32 bit - 27,319,296 - 6588e6aa17a57ba874e8b675114214f0
    { 27319296, { 0x65, (char) 0x88, (char) 0xe6, (char) 0xaa, 0x17, (char) 0xa5, 0x7b, (char) 0xa8, 0x74, (char) 0xe8, (char) 0xb6, 0x75, 0x11, 0x42, 0x14, (char) 0xf0 } },

    // 1.0.6 - 64 bit - 27,358,208 - 6abfbf61869fc436d76c93d1bc7e2735
    { 27358208, { 0x6a, (char) 0xbf, (char) 0xbf, 0x61, (char) 0x86, (char) 0x9f, (char) 0xc4, 0x36, (char) 0xd7, 0x6c, (char) 0x93, (char) 0xd1, (char) 0xbc, 0x7e, 0x27, 0x35 } },

    // 1.0.7 - 32 bit - 27,319,296 - 25830a6c2ff5751f3a55915fb60702f4
    { 27319296, { 0x25, (char) 0x83, 0x0a, 0x6c, 0x2f, (char) 0xf5, 0x75, 0x1f, 0x3a, 0x55, (char) 0x91, 0x5f, (char) 0xb6, 0x07, 0x02, (char) 0xf4 } },

    // 1.1.3 - 64 bit - 27,358,208 - 80f1e673d249d1cda67a2936326f866b
    { 27358208, { (char) 0x80, (char) 0xf1, (char) 0xe6, 0x73, (char) 0xd2, 0x49, (char) 0xd1, (char) 0xcd, (char) 0xa6, 0x7a, 0x29, 0x36, 0x32, 0x6f, (char) 0x86, 0x6b } },

    // 1.1.0 (S) - 64 bit - 27,358,208 - 3703e0dc7bd93abd4c29e1a03f1f6c0a
    { 27358208, { 0x37, 0x03, (char) 0xe0, (char) 0xdc, 0x7b, (char) 0xd9, 0x3a, (char) 0xbd, 0x4c, 0x29, (char) 0xe1, (char) 0xa0, 0x3f, 0x1f, 0x6c, 0x0a } },

    // 1.1.6 (S) - 64 bit - 27,347,456 - dbd9a30c168efef577d40a28d9adf37d
    { 27347456, { (char) 0xdb, (char) 0xd9, (char) 0xa3, 0x0c, 0x16, (char) 0x8e, (char) 0xfe, (char) 0xf5, 0x77, (char) 0xd4, 0x0a, 0x28, (char) 0xd9, (char) 0xad, (char) 0xf3, 0x7d } },

    { 0, { 0 } }
};

bool Preferences::check_secret_sauce()
{
    pfc::string8 path;

    cfg_sc_path.get(path);

    if (path.is_empty())
        return false;

    path += "\\";
    path += _DLLFileName;

    pfc::stringcvt::string_os_from_utf8 pathnative(path);

    FILE * f = _tfopen(pathnative, _T("rb"));

    if (!f)
        return false;

    fseek(f, 0, SEEK_END);

    size_t FileSize = ftell(f);

    bool found = false;

    for (int i = 0; sc_hashes[i].size; ++i)
    {
        if (sc_hashes[i].size == FileSize)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        fclose(f);
        return false;
    }

    fseek(f, 0, SEEK_SET);

    unsigned char buffer[1024];
    size_t BytesReadTotal = 0;

    static_api_ptr_t<hasher_md5> m_hasher;
    hasher_md5_state m_hasher_state;

    m_hasher->initialize(m_hasher_state);

    while (!feof(f))
    {
        size_t BytesRead = fread(buffer, 1, 1024, f);

        BytesReadTotal += BytesRead;

        if (BytesRead)
            m_hasher->process(m_hasher_state, buffer, BytesRead);

        if (BytesRead < 1024)
            break;
    }

    fclose(f);

    if (BytesReadTotal != FileSize)
        return false;

    hasher_md5_result m_hasher_result = m_hasher->get_result(m_hasher_state);

    for (int i = 0; sc_hashes[i].size; ++i)
    {
        if (FileSize == sc_hashes[i].size && m_hasher_result == sc_hashes[i].hash)
            return true;
    }

    return false;
}

static const char * chip_counts[] = { "1", "2", "5", "10", "25", "50", "100" };

const Preferences::plugin_names_ids Preferences::Plugins[] =
{
    { "Emu de MIDI", 0, -1, plugin_always_present },

#ifdef FLUIDSYNTHSUPPORT
    { "FluidSynth",
#ifdef BASSMIDISUPPORT
      2, -1,
#else
      4, 2,
#endif
      plugin_always_present },
#endif

#ifdef BASSMIDISUPPORT
    { "BASSMIDI",
#ifdef FLUIDSYNTHSUPPORT
      4, -1,
#else
      4, 2,
#endif
      plugin_always_present },
#endif

    { "Super MUNT GM", 3, -1, plugin_always_present },

    { "libADLMIDI", 6, -1, plugin_always_present },
    { "libOPNMIDI", 7, -1, plugin_always_present },
    { "oplmidi", 8, -1, plugin_never_present },
    { "Nuclear Option", 9, -1, plugin_always_present },
    { "Secret Sauce", 10, -1, plugin_sauce_present }
};

BOOL Preferences::OnInitDialog(CWindow, LPARAM)
{
    CWindow w;

    int plugin = cfg_plugin;
    int plugin_selected = -1;

    secret_sauce_found = check_secret_sauce();

    plugins_reverse_map[plugin] = -1;
    plugins_present_map[plugin] = -1;
    plugins_present_reverse_map[-1] = -1;

    for (unsigned int i = 0, j = _countof(Plugins); i < j; ++i)
    {
        plugins_reverse_map[Plugins[i].plugin_number] = i;
        if (Plugins[i].plugin_number_alt >= 0)
            plugins_reverse_map[Plugins[i].plugin_number_alt] = i;
    }

    int plugin_reverse = -1;

    if (plugin != 1 && plugin != 5)
    {
        plugin_reverse = plugins_reverse_map[plugin];

        if (plugin_reverse < 0 || !Plugins[plugin_reverse].plugin_present(this))
        {
            plugin = default_cfg_plugin;
            plugin_reverse = plugins_reverse_map[default_cfg_plugin];
        }
        else if (Plugins[plugin_reverse].plugin_number_alt == plugin)
        {
            plugin = Plugins[plugin_reverse].plugin_number;
        }
    }

    plugins_reported = 0;

    w = GetDlgItem(IDC_PLUGIN);

    for (unsigned i = 0, j = _countof(Plugins); i < j; ++i)
    {
        const plugin_names_ids& plugin = Plugins[i];

        if (plugin.plugin_present(this))
        {
            plugins_present_map[plugin.plugin_number] = plugins_reported;

            if (plugin.plugin_number_alt >= 0)
                plugins_present_map[plugin.plugin_number_alt] = plugins_reported;

            plugins_present_reverse_map[plugins_reported] = plugin.plugin_number;
            ++plugins_reported;

            uSendMessageText(w, CB_ADDSTRING, 0, plugin.name);
        }
    }

    enum_vsti_plugins();

    if (vsti_plugins.get_size())
    {
        pfc::string8 temp;
        temp = "Found ";
        temp += pfc::format_int(vsti_plugins.get_size());
        temp += " plug-ins";
        console::print(temp);

        for (unsigned i = 0, j = vsti_plugins.get_count(); i < j; ++i)
        {
            plugins_present_reverse_map[plugins_reported + i] = 1;
        }
    }

    unsigned vsti_count = vsti_plugins.get_size(), vsti_selected = (unsigned int)~0;

    for (unsigned i = 0; i < vsti_count; ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, vsti_plugins[i].display_name.c_str());

        if (plugin == 1 && !stricmp_utf8(vsti_plugins[i].path.c_str(), cfg_vst_path))
            vsti_selected = i;
    }

    if (plugin == 1 && vsti_selected == ~0)
    {
        plugin = default_cfg_plugin;
        plugin_reverse = plugins_reverse_map[default_cfg_plugin];
    }

    if (plugin != 2 && plugin != 4)
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }

    if (plugin == 3)
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }

    if (plugin != 6)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
    }

    if (plugin != 6 && plugin != 7)
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }

    if (plugin != 1)
    {
        GetDlgItem(IDC_PLUGIN_CONFIGURE).EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_PLUGIN_CONFIGURE).EnableWindow(vsti_plugins[vsti_selected].has_editor);
        vsti_config = cfg_vst_config[vsti_plugins[vsti_selected].unique_id];
    }

    if (plugin != 9)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }

    if (plugin != 1 && plugin != 2 && plugin != 4 && plugin != 10)
    {
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(FALSE);
    }

    {
        m_soundfont = cfg_soundfont_path;
        const char * filename;
        if (m_soundfont.is_empty())
            filename = click_to_set;
        else
            filename = m_soundfont.get_ptr() + m_soundfont.scan_filename();
        uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, filename);
    }

    {
        m_munt_path = cfg_munt_base_path;
        const char * path;
        if (m_munt_path.is_empty())
            path = click_to_set;
        else
            path = m_munt_path;
        uSetDlgItemText(m_hWnd, IDC_MUNT, path);
    }

#ifdef DXISUPPORT
    unsigned dxi_selected = ~0;

    dxi_plugins.set_count(0);

    ::CoInitialize(NULL);

    {
        CPlugInInventory theInventory;

        if (SUCCEEDED(theInventory.EnumPlugIns()))
        {
            unsigned count = theInventory.GetCount();
            pfc::string8_fastalloc name;
            CLSID theClsid;

            for (unsigned i = 0; i < count; ++i)
            {
                if (SUCCEEDED(theInventory.GetInfo(i, &theClsid, name)))
                {
                    dxi_plugins.append_single(theClsid);
                    uSendMessageText(w, CB_ADDSTRING, 0, name);

                    if (theClsid == cfg_dxi_plugin.get_value())
                        dxi_selected = i;

                    plugins_present_reverse_map[plugins_reported + vsti_count + i] = 5;
                }
            }
        }
    }

    ::CoUninitialize();

#endif
    if (plugin == 1) plugin_selected = plugins_reported + vsti_selected;
#ifdef DXISUPPORT
    else if (plugin == 5)
    {
        if (dxi_selected != ~0)
            plugin_selected = plugins_reported + vsti_count + dxi_selected;
        else
            plugin = 0;
    }
#endif
    else
    {
        plugin_selected = plugins_present_map[plugin];
    }

    ::SendMessage(w, CB_SETCURSEL, plugin_selected, (WPARAM)0);

    {
        char temp[16];

        for (size_t n = _countof(SampleRates); n--;)
        {
            if (SampleRates[n] != cfg_srate)
            {
                _itoa_s(SampleRates[n], temp, _countof(temp), 10);
                cfg_history_rate.add_item(temp);
            }
        }

        _itoa_s(cfg_srate, temp, _countof(temp), 10);
        cfg_history_rate.add_item(temp);

        w = GetDlgItem(IDC_SAMPLERATE);

        cfg_history_rate.setup_dropdown(w);

        ::SendMessage(w, CB_SETCURSEL, 0, 0);
    }

    if (!plugin)
    {
        if (g_running)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
    }

    w = GetDlgItem(IDC_LOOP);

    for (unsigned i = 0; i < _countof(loop_txt); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, loop_txt[i]);
    }
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_loop_type, 0);

    w = GetDlgItem(IDC_CLOOP);

    for (unsigned i = 0; i < _countof(loop_txt) - 2; ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, loop_txt[i]);
    }
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_loop_type_other, 0);

    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, (WPARAM)cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, (WPARAM)cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, (WPARAM)cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, (WPARAM)cfg_ff7loopz);

    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, (WPARAM)cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM)cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, (WPARAM)cfg_filter_banks);

    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, (WPARAM)cfg_ms_panning);
    SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_SETCHECK, !cfg_midi_reverb);

    const char * const * banknames = adl_getBankNames();
    const unsigned bank_count = adl_getBanksCount();

    for (unsigned i = 0; i < bank_count; ++i)
    {
        m_bank_list += adl_bank(i, banknames[i]);
    }

    m_bank_list.sort();

    unsigned bank_selected = 0;

    w = GetDlgItem(IDC_ADL_BANK);

    for (unsigned i = 0; i < m_bank_list.get_count(); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, m_bank_list[i].name);

        if (m_bank_list[i].number == cfg_adl_bank)
            bank_selected = i;
    }

    w.SendMessage(CB_SETCURSEL, bank_selected);

    w = GetDlgItem(IDC_ADL_CHIPS);

    for (unsigned i = 0; i < _countof(chip_counts); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, chip_counts[i]);
    }

    SetDlgItemInt(IDC_ADL_CHIPS, (UINT)cfg_adl_chips, 0);

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, (WPARAM)cfg_adl_panning);

#ifdef BASSMIDISUPPORT
    w = GetDlgItem(IDC_RESAMPLING);
    uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "8pt Sinc interpolation");
    if (_bassmidi_src2_avail)
        uSendMessageText(w, CB_ADDSTRING, 0, "16pt Sinc interpolation");
    if (!_bassmidi_src2_avail && cfg_resampling > 1)
        ::SendMessage(w, CB_SETCURSEL, 1, 0);
    else
        ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_resampling, 0);
#endif
#ifdef FLUIDSYNTHSUPPORT
    w = GetDlgItem(IDC_RESAMPLING);
    uSendMessageText(w, CB_ADDSTRING, 0, "No interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "Cubic interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "7pt Sinc interpolation");
    if (cfg_fluid_interp_method == 0)
        ::SendMessage(w, CB_SETCURSEL, 0, 0);
    else if (cfg_fluid_interp_method == 1)
        ::SendMessage(w, CB_SETCURSEL, 1, 0);
    else if (cfg_fluid_interp_method == 4)
        ::SendMessage(w, CB_SETCURSEL, 2, 0);
    else if (cfg_fluid_interp_method == 7)
        ::SendMessage(w, CB_SETCURSEL, 3, 0);
    else
        ::SendMessage(w, CB_SETCURSEL, 3, 0);
#endif
    {
        w = GetDlgItem(IDC_MUNT_GM);

        for (unsigned i = 0, j = _countof(MuntBankNames); i < j; ++i)
        {
            uSendMessageText(w, CB_ADDSTRING, 0, MuntBankNames[i]);
        }

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_munt_gm, 0);
    }

    {
        size_t PresetNumber = 0;

        w = GetDlgItem(IDC_MS_PRESET);

        for (size_t i = 0, j = _MSPresets.get_count(); i < j; ++i)
        {
            const MSPreset & preset = _MSPresets[i];

            uSendMessageText(w, CB_ADDSTRING, 0, preset.name);

            if (preset.synth == (unsigned int)cfg_ms_synth && preset.bank == (unsigned int)cfg_ms_bank)
                PresetNumber = i;
        }

        ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);
    }

    w = GetDlgItem(IDC_FILTER_FLAVOR);

    uSendMessageText(w, CB_ADDSTRING, 0, "Default");
    uSendMessageText(w, CB_ADDSTRING, 0, "GM");
    uSendMessageText(w, CB_ADDSTRING, 0, "GM2");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-55");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88 Pro");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-8820");
    uSendMessageText(w, CB_ADDSTRING, 0, "XG");
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_midi_flavor, 0);

#ifndef BASSMIDISUPPORT
    uSetWindowText(GetDlgItem(IDC_CACHED), "No info.");
#endif

    SetTimer(ID_REFRESH, 20);

    busy = false;

    m_hooks.AddDialogWithControls(*this);
    return FALSE;
}

void Preferences::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnButtonConfig(UINT, int, CWindow)
{
    int plugin_selected = (int)GetDlgItem(IDC_PLUGIN).SendMessage(CB_GETCURSEL, 0, 0);

    if ((plugin_selected >= plugins_reported) && plugin_selected < (int)(plugins_reported + vsti_plugins.get_count()))
    {
        busy = true;
        OnChanged();

        VSTiPlayer vstPlayer;

        if (vstPlayer.LoadVST(vsti_plugins[(size_t)(plugin_selected - plugins_reported)].path.c_str()))
        {
            if (vsti_config.size())
                vstPlayer.setChunk(&vsti_config[0], vsti_config.size());

            vstPlayer.displayEditorModal();
            vstPlayer.getChunk(vsti_config);
        }

        busy = false;

        OnChanged();
    }
}

void Preferences::OnPluginChange(UINT, int, CWindow w)
{
    // t_size vsti_count = vsti_plugins.get_size();
    int plugin_selected = (int)::SendMessage(w, CB_GETCURSEL, 0, 0);
//  int plugin_index = -1;
    int plugin = 0;

    if ((plugin_selected >= plugins_reported) && (plugin_selected < (int)(plugins_reported + vsti_plugins.get_count())))
        plugin = 1;
#ifdef DXISUPPORT
    else if (plugin_selected >= plugins_reported + vsti_plugins.get_count())
        plugin = 5;
#endif
    else
        plugin = plugins_present_reverse_map[plugin_selected];

    GetDlgItem(IDC_SAMPLERATE).EnableWindow(plugin || !g_running);

    GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_SOUNDFONT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_RESAMPLING).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_CACHED_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_CACHED).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(plugin == 6);
    GetDlgItem(IDC_ADL_BANK).EnableWindow(plugin == 6);
    GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_ADL_CHIPS).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_ADL_PANNING).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(plugin == 9);
    GetDlgItem(IDC_MS_PRESET).EnableWindow(plugin == 9);
    GetDlgItem(IDC_MS_PANNING).EnableWindow(plugin == 9);
    {
        bool enable = (plugin == 1) || (plugin == 2) || (plugin == 4) || (plugin == 10);
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(enable);
    }

    if (plugin == 3)
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else if (vsti_plugins.get_count() == 0)
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_SHOW);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }

    GetDlgItem(IDC_PLUGIN_CONFIGURE).EnableWindow(plugin_selected >= plugins_reported && plugin < plugins_reported + vsti_plugins.get_count() && vsti_plugins[(size_t)(plugin_selected - plugins_reported)].has_editor);

    if ((plugin_selected >= plugins_reported) && (plugin_selected < (int)(plugins_reported + vsti_plugins.get_count())))
    {
        vsti_config = cfg_vst_config[vsti_plugins[(size_t)(plugin_selected - plugins_reported)].unique_id];
    }

    OnChanged();
}

void Preferences::OnSetFocus(UINT, int, CWindow w)
{
    SetFocus();

    if (w == GetDlgItem(IDC_SOUNDFONT))
    {
        pfc::string8 directory, filename;

        directory = m_soundfont;
        filename = m_soundfont;

        directory.truncate(directory.scan_filename());

        if (uGetOpenFileName(m_hWnd,
            "SoundFont and list files|*.sf2;*.sf3;*.sflist"
        #ifdef SF2PACK
            "*.sf2pack;*.sfogg;"
        #endif
        #ifdef BASSMIDISUPPORT
            ";*.json"
        #endif

            "*.sflist|SoundFont files|*.sf2;*.sf3"
        #ifdef SF2PACK
            ";*.sf2pack;*.sfogg;"
        #endif

            "|SoundFont list files|*.sflist"
        #ifdef BASSMIDISUPPORT
            ";*.json"
        #endif
            ,
            0, "sf2", "Choose a SoundFont bank or list...", directory, filename, FALSE))
        {
            m_soundfont = filename;

            uSetWindowText(w, filename.get_ptr() + filename.scan_filename());
            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_MUNT))
    {
        pfc::string8 path;
        if (uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM set...", path))
        {
            m_munt_path = path;

            t_size length = m_munt_path.length();

            if (length >= 1 && !pfc::is_path_separator((unsigned int)*(m_munt_path.get_ptr() + length - 1)))
                m_munt_path.add_byte('\\');

            const char * display_path;

            if (length)
                display_path = m_munt_path;
            else
                display_path = click_to_set;

            uSetWindowText(w, display_path);
            OnChanged();
        }
    }
}

void Preferences::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_REFRESH)
    {
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(cfg_plugin || !g_running);

    #ifdef BASSMIDISUPPORT
        m_cached.reset();

        uint64_t total_sample_size, samples_loaded_size;

        if (GetSoundFontStatistics(total_sample_size, samples_loaded_size))
        {
            m_cached = pfc::format_file_size_short(samples_loaded_size);
            m_cached += " / ";
            m_cached += pfc::format_file_size_short(total_sample_size);
        }
        else
        {
            m_cached = "BASS not loaded.";
        }

        if (strcmp(m_cached, m_cached_current))
        {
            m_cached_current = m_cached;
            uSetWindowText(GetDlgItem(IDC_CACHED), m_cached);
        }
    #endif
    }
}

t_uint32 Preferences::get_state()
{
    t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
    if (HasChanged()) state |= preferences_state::changed;
    if (busy) state |= preferences_state::busy;
    return state;
}

void Preferences::reset()
{
    int default_plugin_entry = plugins_reverse_map[default_cfg_plugin];
    int plugin_selected = plugins_present_map[default_plugin_entry];

    SendDlgItemMessage(IDC_PLUGIN, CB_SETCURSEL, (WPARAM)plugin_selected);

    if (default_cfg_plugin != 2 && default_cfg_plugin != 4)
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }
    if (default_cfg_plugin != 6)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }
    if (default_cfg_plugin == 3)
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else if (vsti_plugins.get_count() == 0)
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_SHOW);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_WARNING).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PLUGIN_CONFIGURE).ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    if (default_cfg_plugin != 9)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }
    {
        bool enable = (default_cfg_plugin == 1) || (default_cfg_plugin == 2) || (default_cfg_plugin == 4) || (default_cfg_plugin == 10);
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(enable);
    }

    uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, click_to_set);
    uSetDlgItemText(m_hWnd, IDC_MUNT, click_to_set);

    m_soundfont.reset();
    m_munt_path.reset();

    SetDlgItemInt(IDC_SAMPLERATE, default_cfg_srate, FALSE);

    if (!default_cfg_plugin)
    {
        if (g_running)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
    }

    SendDlgItemMessage(IDC_LOOP, CB_SETCURSEL, default_cfg_loop_type);
    SendDlgItemMessage(IDC_CLOOP, CB_SETCURSEL, default_cfg_loop_type_other);
    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, default_cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, default_cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz);
    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks);
    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, default_cfg_ms_panning);
    SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_SETCURSEL, default_cfg_midi_flavor);
    SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_SETCHECK, !default_cfg_midi_reverb);

    {
        unsigned bank_selected = 0;

        for (unsigned i = 0; i < m_bank_list.get_count(); ++i)
        {
            if (m_bank_list[i].number == default_cfg_adl_bank)
            {
                bank_selected = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, bank_selected);
    }

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, default_cfg_adl_panning);
    SetDlgItemInt(IDC_ADL_CHIPS, default_cfg_adl_chips, 0);
//  SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, default_cfg_recover_tracks );

#ifdef FLUIDSYNTHSUPPORT
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, 2 /* 4 */);
#else
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, default_cfg_resampling);
#endif
    {
        size_t preset_number = 0;

        for (size_t i = 0, j = _MSPresets.get_count(); i < j; ++i)
        {
            const MSPreset & preset = _MSPresets[i];

            if (preset.synth == DefaultMSSynth && preset.bank == DefaultMSBank)
            {
                preset_number = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_MS_PRESET, CB_SETCURSEL, preset_number);
    }

    SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_SETCURSEL, (WPARAM)cfg_midi_flavor);

    vsti_config.resize(0);

    OnChanged();
}

void Preferences::apply()
{
    int t = (int)GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE);

    if (t < 6000)
        t = 6000;
    else
    if (t > 192000)
        t = 192000;

    SetDlgItemInt(IDC_SAMPLERATE, (UINT)t, FALSE);

    {
        char temp[16];

        _itoa_s(t, temp, _countof(temp), 10);

        cfg_history_rate.add_item(temp);
        cfg_srate = t;
    }

    {
        t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)m_bank_list.get_count())
            t = 0;

        cfg_adl_bank = m_bank_list[(t_size)t].number;
    }

    {
        t = (int)GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE);

        if (t < 1)
            t = 1;
        else
        if (t > 100)
            t = 100;

        SetDlgItemInt(IDC_ADL_CHIPS, (UINT)t, FALSE);

        cfg_adl_chips = t;
    }

    cfg_adl_panning = (t_int32)SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    cfg_munt_gm = (t_int32)SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL);

    {
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        cfg_ms_synth = (t_int32)preset.synth;
        cfg_ms_bank = (t_int32)preset.bank;
    }

    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if (plugin_selected >= plugins_reported && plugin_selected < (int)(plugins_reported + vsti_plugins.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + vsti_plugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = plugins_present_reverse_map[plugin_selected];

        cfg_vst_path = "";

        cfg_plugin = plugin;

        if (plugin == 1)
        {
            cfg_vst_path = vsti_plugins[(size_t)(plugin_selected - plugins_reported)].path.c_str();

            cfg_vst_config[vsti_plugins[(size_t)(plugin_selected - plugins_reported)].unique_id] = vsti_config;
        }
    #ifdef DXISUPPORT
        else if (plugin == 5)
        {
            cfg_dxi_plugin = dxi_plugins[plugin_selected - vsti_plugins.get_count() - plugins_reported];
        }
    #endif
    }

    cfg_soundfont_path = m_soundfont;
    cfg_munt_base_path = m_munt_path;

    cfg_loop_type = (t_int32)SendDlgItemMessage(IDC_LOOP, CB_GETCURSEL);
    cfg_loop_type_other = (t_int32)SendDlgItemMessage(IDC_CLOOP, CB_GETCURSEL);
    cfg_thloopz = (t_int32)SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK);
    cfg_rpgmloopz = (t_int32)SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK);
    cfg_xmiloopz = (t_int32)SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK);
    cfg_ff7loopz = (t_int32)SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK);
    cfg_emidi_exclusion = (t_int32)SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK);
    cfg_filter_instruments = (t_int32)SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
    cfg_filter_banks = (t_int32)SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);
    cfg_ms_panning = (t_int32)SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK);
//  cfg_recover_tracks = SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK );

#ifdef FLUIDSYNTHSUPPORT
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        cfg_fluid_interp_method = interp_method;
    }
#endif

#ifdef BASSMIDISUPPORT
    cfg_resampling = (t_int32)SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
#endif
    cfg_midi_flavor = (t_int32)SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_GETCURSEL);
    cfg_midi_reverb = !SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_GETCHECK);

    OnChanged(); // our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool Preferences::HasChanged()
{
    // returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    bool changed = false;

    if (!changed && GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT)cfg_srate) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP, CB_GETCURSEL) != cfg_loop_type) changed = true;
    if (!changed && SendDlgItemMessage(IDC_CLOOP, CB_GETCURSEL) != cfg_loop_type_other) changed = true;
    if (!changed && SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK) != cfg_thloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK) != cfg_rpgmloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK) != cfg_xmiloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK) != cfg_ff7loopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK) != cfg_emidi_exclusion) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != cfg_filter_instruments) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != cfg_filter_banks) changed = true;
    if (!changed && SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK) != cfg_ms_panning) changed = true;

#ifdef FLUIDSYNTHSUPPORT
    if (!changed)
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        if (interp_method != cfg_fluid_interp_method) changed = true;
    }
#endif

#ifdef BASSMIDISUPPORT
    if (!changed && SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL) != cfg_resampling)
        changed = true;
#endif
    if (!changed && SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_GETCURSEL) != cfg_midi_flavor)
        changed = true;

    if (!changed && !SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_GETCHECK) != cfg_midi_reverb)
        changed = true;

    if (!changed)
    {
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        if (!(preset.synth == (unsigned int)cfg_ms_synth && preset.bank == (unsigned int)cfg_ms_bank))
            changed = true;
    }

    if (!changed)
    {
        int t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)m_bank_list.get_count())
            t = 0;

        if (m_bank_list[(t_size)t].number != (int)cfg_adl_bank)
            changed = true;
    }

    if (!changed && GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT)cfg_adl_chips)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != cfg_adl_panning)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL) != cfg_munt_gm)
        changed = true;

    if (!changed)
    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if ((plugin_selected >= plugins_reported) && (plugin_selected < plugins_reported + (int)vsti_plugins.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + vsti_plugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = plugins_present_reverse_map[plugin_selected];

        if (plugin != cfg_plugin)
            changed = true;

        if (!changed && plugin == 1)
        {
            if (stricmp_utf8(cfg_vst_path, vsti_plugins[(size_t)(plugin_selected - plugins_reported)].path.c_str()))
                changed = true;

            if (!changed)
            {
                t_uint32 unique_id = vsti_plugins[(size_t)(plugin_selected - plugins_reported)].unique_id;

                if (vsti_config.size() != cfg_vst_config[unique_id].size() || (vsti_config.size() && memcmp(&vsti_config[0], &cfg_vst_config[unique_id][0], vsti_config.size())))
                    changed = true;
            }
        }
    #ifdef DXISUPPORT
        else if (!changed && plugin == 5)
        {
            if (dxi_plugins[plugin_selected - vsti_plugins.get_count() - plugins_reported] != cfg_dxi_plugin.get_value()) changed = true;
        }
    #endif
    }

    if (!changed)
    {
        if (stricmp_utf8(m_soundfont, cfg_soundfont_path))
            changed = true;
    }

    if (!changed)
    {
        if (stricmp_utf8(m_munt_path, cfg_munt_base_path))
            changed = true;
    }

    return changed;
}

void Preferences::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

class PreferencesPage : public preferences_page_impl<Preferences>
{
public:
    const char * get_name()
    {
        return COMPONENT_NAME;
    }

    GUID get_guid()
    {
        return COMPONENT_GUID;
    }

    GUID get_parent_guid()
    {
        return guid_input;
    }
};

static preferences_page_factory_t<PreferencesPage> PreferencesPageFactory;
