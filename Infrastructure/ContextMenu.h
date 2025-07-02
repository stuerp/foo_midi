
/** $VER: ContextMenu.h (2025.07.02) **/

#pragma once

#include <sdk/foobar2000-lite.h>
#include <sdk/contextmenu.h>

class ContextMenu : public contextmenu_item_simple
{
public:
    ContextMenu() noexcept { };

    ContextMenu(const ContextMenu&) = delete;
    ContextMenu(const ContextMenu&&) = delete;
    ContextMenu& operator=(const ContextMenu&) = delete;
    ContextMenu& operator=(ContextMenu&&) = delete;

    virtual ~ContextMenu() { };

#pragma region contextmenu_item_simple

    virtual unsigned get_num_items() noexcept;

    virtual void get_item_name(unsigned int itemIndex, pfc::string_base & itemName) noexcept;
    virtual bool get_item_description(unsigned itemIndex, pfc::string_base & description) noexcept;
    virtual GUID get_item_guid(unsigned int itemIndex) noexcept;

    virtual bool context_get_display(unsigned int itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out, unsigned &, const GUID &);

    virtual void context_command(unsigned int itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & data, const GUID &);

#pragma endregion

#pragma region contextmenu_item_v2

    GUID get_parent() noexcept
    {
        return contextmenu_groups::utilities;
    }

#pragma endregion

private:
    static const char * Names[5];
};
