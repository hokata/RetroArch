/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <file/file_path.h>
#include "menu.h"
#include "menu_entries_cbs.h"
#include "menu_setting.h"
#include "menu_shader.h"
#include "menu_navigation.h"

#include "../retroarch.h"


#ifdef HAVE_SHADER_MANAGER
static void shader_action_parameter_left_common(
      struct video_shader_parameter *param,
      struct video_shader *shader)
{
   if (!shader)
      return;

   param->current -= param->step;
   param->current = min(max(param->minimum, param->current), param->maximum);
}
#endif

static int shader_action_parameter_left(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   struct video_shader *shader = video_shader_driver_get_current_shader();
   struct video_shader_parameter *param = 
      &shader->parameters[type - MENU_SETTINGS_SHADER_PARAMETER_0];

   shader_action_parameter_left_common(param, shader);
#endif
   return 0;
}

static int shader_action_parameter_preset_left(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   struct video_shader_parameter *param = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   struct video_shader *shader = menu ? menu->shader : NULL;
   if (!menu || !shader)
      return -1;

   param = &shader->parameters[type - MENU_SETTINGS_SHADER_PRESET_PARAMETER_0];

   shader_action_parameter_left_common(param, shader);
#endif
   return 0;
}

static int action_left_cheat(unsigned type, const char *label,
      bool wraparound)
{
   global_t *global       = global_get_ptr();
   cheat_manager_t *cheat = global->cheat;
   size_t idx             = type - MENU_SETTINGS_CHEAT_BEGIN;

   if (!cheat)
      return -1;

   cheat->cheats[idx].state = !cheat->cheats[idx].state;
   cheat_manager_update(cheat, idx);

   return 0;
}

static int action_left_input_desc(unsigned type, const char *label,
      bool wraparound)
{
   unsigned inp_desc_index_offset = type - MENU_SETTINGS_INPUT_DESC_BEGIN;
   unsigned inp_desc_user         = inp_desc_index_offset / RARCH_FIRST_META_KEY;
   unsigned inp_desc_button_index_offset = inp_desc_index_offset - (inp_desc_user * RARCH_FIRST_META_KEY);
   settings_t *settings = config_get_ptr();

   if (settings->input.remap_ids[inp_desc_user][inp_desc_button_index_offset] > 0)
      settings->input.remap_ids[inp_desc_user][inp_desc_button_index_offset]--;

   return 0;
}

static int action_left_save_state(unsigned type, const char *label,
      bool wraparound)
{
   settings_t *settings = config_get_ptr();

   /* Slot -1 is (auto) slot. */
   if (settings->state_slot >= 0)
      settings->state_slot--;

   return 0;
}

static int action_left_scroll(unsigned type, const char *label,
      bool wraparound)
{
   unsigned scroll_speed = 0, fast_scroll_speed = 0;
   menu_list_t *menu_list = menu_list_get_ptr();
   menu_navigation_t *nav = menu_navigation_get_ptr();
   if (!nav || !menu_list)
      return -1;

   scroll_speed      = (max(nav->scroll.acceleration, 2) - 2) / 4 + 1;
   fast_scroll_speed = 4 + 4 * scroll_speed;

   if (nav->selection_ptr > fast_scroll_speed)
      menu_navigation_set(nav, nav->selection_ptr - fast_scroll_speed, true);
   else
      menu_navigation_clear(nav, false);

   return 0;
}

static int action_left_mainmenu(unsigned type, const char *label,
      bool wraparound)
{
   menu_file_list_cbs_t *cbs = NULL;
   unsigned        push_list = 0;
   driver_t          *driver = driver_get_ptr();
   menu_list_t    *menu_list = menu_list_get_ptr();
   menu_handle_t       *menu = menu_driver_get_ptr();
   unsigned           action = MENU_ACTION_LEFT;
   if (!menu)
      return -1;

   if (file_list_get_size(menu_list->menu_stack) == 1)
   {

      if (!strcmp(driver->menu_ctx->ident, "xmb"))
      {
         menu->navigation.selection_ptr = 0;
         if (menu->categories.selection_ptr != 0)
         {
            push_list = 1;
         }
      }
   }
   else 
      push_list = 2;

   cbs = menu_list_get_actiondata_at_offset(menu_list->selection_buf,
         menu->navigation.selection_ptr);

   switch (push_list)
   {
      case 1:
         menu_driver_list_cache(true, action);

         if (cbs && cbs->action_content_list_switch)
            return cbs->action_content_list_switch(
                  menu_list->selection_buf, menu_list->menu_stack,
                  "", "", 0);
         break;
      case 2:
         action_left_scroll(0, "", false);
         break;
      case 0:
      default:
         break;
   }

   return 0;
}

static int action_left_shader_scale_pass(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   unsigned pass = type - MENU_SETTINGS_SHADER_PASS_SCALE_0;
   struct video_shader *shader = NULL;
   struct video_shader_pass *shader_pass = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;
   
   shader = menu->shader;
   if (!shader)
      return -1;
   shader_pass = &shader->pass[pass];
   if (!shader_pass)
      return -1;

   {
      unsigned current_scale   = shader_pass->fbo.scale_x;
      unsigned delta           = 5;
      current_scale            = (current_scale + delta) % 6;

      shader_pass->fbo.valid   = current_scale;
      shader_pass->fbo.scale_x = shader_pass->fbo.scale_y = current_scale;

   }
#endif
   return 0;
}

static int action_left_shader_filter_pass(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   unsigned pass = type - MENU_SETTINGS_SHADER_PASS_FILTER_0;
   struct video_shader *shader = NULL;
   struct video_shader_pass *shader_pass = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;
   
   shader = menu->shader;
   if (!shader)
      return -1;
   shader_pass = &shader->pass[pass];
   if (!shader_pass)
      return -1;

   {
      unsigned delta = 2;
      shader_pass->filter = ((shader_pass->filter + delta) % 3);

   }
#endif
   return 0;
}

static int action_left_shader_filter_default(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   rarch_setting_t *setting = menu_setting_find("video_smooth");
   unsigned action = MENU_ACTION_LEFT;
   if (setting)
      menu_setting_handler(setting, action);
#endif
   return 0;
}

static int action_left_cheat_num_passes(unsigned type, const char *label,
      bool wraparound)
{
   unsigned new_size = 0;
   global_t *global       = global_get_ptr();
   cheat_manager_t *cheat = global->cheat;
   menu_handle_t *menu    = menu_driver_get_ptr();
   if (!menu)
      return -1;

   if (!cheat)
      return -1;

   if (cheat->size)
      new_size = cheat->size - 1;
   menu_set_refresh();

   if (menu_needs_refresh())
      cheat_manager_realloc(cheat, new_size);

   return 0;
}

static int action_left_shader_num_passes(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   struct video_shader *shader = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;
   
   shader = menu->shader;
   if (!shader)
      return -1;

   if (shader->passes)
      shader->passes--;
   menu_set_refresh();

   if (menu_needs_refresh())
      video_shader_resolve_parameters(NULL, menu->shader);

#endif
   return 0;
}

static int action_left_video_resolution(unsigned type, const char *label,
      bool wraparound)
{ 
   global_t *global = global_get_ptr();
    
   (void)global;

#if defined(__CELLOS_LV2__)
   if (global->console.screen.resolutions.current.idx)
   {
      global->console.screen.resolutions.current.idx--;
      global->console.screen.resolutions.current.id =
         global->console.screen.resolutions.list
         [global->console.screen.resolutions.current.idx];
   }
#else
   video_driver_get_video_output_prev();
#endif

   return 0;
}

static int core_setting_left(unsigned type, const char *label,
      bool wraparound)
{
   unsigned idx     = type - MENU_SETTINGS_CORE_OPTION_START;
   global_t *global = global_get_ptr();

   (void)label;

   core_option_prev(global->system.core_options, idx);

   return 0;
}

static int disk_options_disk_idx_left(unsigned type, const char *label,
      bool wraparound)
{
   event_command(EVENT_CMD_DISK_PREV);

   return 0;
}

static int bind_left_generic(unsigned type, const char *label,
      bool wraparound)
{
   unsigned action = MENU_ACTION_LEFT;
   return menu_setting_set(type, label, action, wraparound);
}

void menu_entries_cbs_init_bind_left(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      const char *elem0, const char *elem1, const char *menu_label)
{
   int i;

   if (!cbs)
      return;

   if (label)
   {
      if (menu_entries_common_is_settings_entry(elem0))
      {
         cbs->action_left = action_left_scroll;
         return;
      }
   }

   cbs->action_left = bind_left_generic;

   switch (type)
   {
      case MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_INDEX:
         cbs->action_left = disk_options_disk_idx_left;
         break;
      case MENU_FILE_PLAIN:
      case MENU_FILE_DIRECTORY:
      case MENU_FILE_CARCHIVE:
      case MENU_FILE_CORE:
      case MENU_FILE_RDB:
      case MENU_FILE_RDB_ENTRY:
      case MENU_FILE_CURSOR:
      case MENU_FILE_SHADER:
      case MENU_FILE_SHADER_PRESET:
      case MENU_FILE_IMAGE:
      case MENU_FILE_OVERLAY:
      case MENU_FILE_VIDEOFILTER:
      case MENU_FILE_AUDIOFILTER:
      case MENU_FILE_CONFIG:
      case MENU_FILE_USE_DIRECTORY:
      case MENU_FILE_PLAYLIST_ENTRY:
      case MENU_FILE_DOWNLOAD_CORE:
      case MENU_FILE_CHEAT:
      case MENU_FILE_REMAP:
      case MENU_SETTING_GROUP:
         if (!strcmp(menu_label, "Horizontal Menu")
               || !strcmp(menu_label, "Main Menu"))
            cbs->action_left = action_left_mainmenu;
         else
            cbs->action_left = action_left_scroll;
         break;
      case MENU_SETTING_ACTION:
      case MENU_FILE_CONTENTLIST_ENTRY:
         cbs->action_left = action_left_mainmenu;
         break;
   }

   if (strstr(label, "rdb_entry"))
      cbs->action_left = action_left_scroll;

   else if (type >= MENU_SETTINGS_SHADER_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PARAMETER_LAST)
      cbs->action_left = shader_action_parameter_left;
   else if (type >= MENU_SETTINGS_SHADER_PRESET_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PRESET_PARAMETER_LAST)
      cbs->action_left = shader_action_parameter_preset_left;
   else if (type >= MENU_SETTINGS_CHEAT_BEGIN
         && type <= MENU_SETTINGS_CHEAT_END)
      cbs->action_left = action_left_cheat;
   else if (type >= MENU_SETTINGS_INPUT_DESC_BEGIN
         && type <= MENU_SETTINGS_INPUT_DESC_END)
      cbs->action_left = action_left_input_desc;
   else if (!strcmp(label, "savestate") ||
         !strcmp(label, "loadstate"))
      cbs->action_left = action_left_save_state;
   else if (!strcmp(label, "video_shader_scale_pass"))
      cbs->action_left = action_left_shader_scale_pass;
   else if (!strcmp(label, "video_shader_filter_pass"))
      cbs->action_left = action_left_shader_filter_pass;
   else if (!strcmp(label, "video_shader_default_filter"))
      cbs->action_left = action_left_shader_filter_default;
   else if (!strcmp(label, "video_shader_num_passes"))
      cbs->action_left = action_left_shader_num_passes;
   else if (!strcmp(label, "cheat_num_passes"))
      cbs->action_left = action_left_cheat_num_passes;
   else if (type == MENU_SETTINGS_VIDEO_RESOLUTION)
      cbs->action_left = action_left_video_resolution;
   else if ((type >= MENU_SETTINGS_CORE_OPTION_START))
      cbs->action_left = core_setting_left;

   for (i = 0; i < MAX_USERS; i++)
   {
      char label_setting[PATH_MAX_LENGTH];
      snprintf(label_setting, sizeof(label_setting), "input_player%d_joypad_index", i + 1);

      if (!strcmp(label, label_setting))
         cbs->action_left = bind_left_generic;
   }
}
