
/** $VER: MainMenu.cpp (2025.07.02) P. Stuer **/

#include "pch.h"

#include "CLAPHost.h"

#pragma hdrstop

namespace
{
    #pragma region Node Commands

    class NodeCommand : public mainmenu_node_command
    {
    public:
        NodeCommand() noexcept { };

        NodeCommand(const NodeCommand &) = delete;
        NodeCommand(const NodeCommand &&) = delete;
        NodeCommand & operator=(const NodeCommand &) = delete;
        NodeCommand & operator=(NodeCommand &&) = delete;

        virtual ~NodeCommand() { };

        void get_display(pfc::string_base & text, t_uint32 & flags) override
        {
            text = "Show plug-in GUI";
            flags = CLAP::_Host.HasGUI() ? mainmenu_commands::flag_disabled : 0;
        }

        void execute(service_ptr_t<service_base> callback) override
        {
        };

        GUID get_guid() override
        {
            return { 0x7facf590, 0xeab8, 0x4534, { 0xac, 0xfa, 0x63, 0x6e, 0x2, 0x61, 0xd8, 0x76 } };
        };

        bool get_description(pfc::string_base & description) override
        {
            description = "Shows or hides the GUI of a plug-in.";

            return true;
        }
    };

    #pragma endregion

    #pragma region Node Group

    class NodeGroup : public mainmenu_node_group
    {
    public:
        NodeGroup()
        {
            auto Node = fb2k::service_new<NodeCommand>();

            _Nodes.push_back(std::move(Node));
        }

        NodeGroup(const NodeGroup &) = delete;
        NodeGroup(const NodeGroup &&) = delete;
        NodeGroup & operator=(const NodeGroup &) = delete;
        NodeGroup & operator=(NodeGroup &&) = delete;

        virtual ~NodeGroup() { };

        void get_display(pfc::string_base & text, t_uint32 & flags) override
        {
            text = "MIDI Player";
            flags = 0;
        }

        t_size get_children_count() override
        {
            return _Nodes.size();
        }

        mainmenu_node::ptr get_child(t_size index) override
        {
            return _Nodes[index];
        }

    private:
        std::vector<mainmenu_node::ptr> _Nodes;
    };

    #pragma endregion

    #pragma region Menu

    class Menu : public mainmenu_commands_v2
    {
    public:
        Menu() noexcept { };

        Menu(const Menu &) = delete;
        Menu(const Menu &&) = delete;
        Menu & operator=(const Menu &) = delete;
        Menu & operator=(Menu &&) = delete;

        virtual ~Menu() { };

        t_uint32 get_command_count() override
        {
            return 1;
        }

        GUID get_command(t_uint32 index) override
        {
            return { 0xd27e394e, 0xdcb5, 0x4717, { 0xb5, 0x89, 0x74, 0xfb, 0xa6, 0xb3, 0x81, 0x11 } };
        }

        void get_name(t_uint32 index, pfc::string_base & name) override
        {
            name = "MIDI Player";
        }

        bool get_description(t_uint32 index, pfc::string_base & description) override
        {
            description = "Contains the MIDI Player menu items.";

            return true;
        }

        GUID get_parent() override
        {
            return mainmenu_groups::view;
        }

        void execute(t_uint32 index, service_ptr_t<service_base> callback) override
        {
        }

        bool is_command_dynamic(t_uint32 index) override
        {
            return true;
        }

        mainmenu_node::ptr dynamic_instantiate(t_uint32 index) override
        {
            return fb2k::service_new<NodeGroup>();
        }

        bool dynamic_execute(t_uint32 index, const GUID & subID, service_ptr_t<service_base> callback) override
        {
            return __super::dynamic_execute(index, subID, callback);
        }
    };

//  static mainmenu_commands_factory_t<Menu> _Menu; // FIXME: CLAP GUI does not work yet

#pragma endregion
}
