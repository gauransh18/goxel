/* Goxel 3D voxels editor
 *
 * copyright (c) 2019-2022 Guillaume Chereau <guillaume@noctua-software.com>
 *
 * Goxel is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.

 * Goxel is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.

 * You should have received a copy of the GNU General Public License along with
 * goxel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "goxel.h"
#include "script.h"
#include "system.h"

int gui_about_popup(void *data)
{
    gui_text("Goxel " GOXEL_VERSION_STR);
    gui_text("Copyright © 2015-2022 Guillaume Chereau");
    gui_text("<guillaume@noctua-software.com>");
    gui_text("All right reserved");
    if (!DEFINED(GOXEL_MOBILE)) gui_text("GPL 3 License");
    gui_text("http://guillaumechereau.github.io/goxel");

    if (gui_collapsing_header("Credits", true)) {
        gui_text("Libraries:");
        gui_text("● dear imgui (https://github.com/ocornut/imgui)");
        gui_text("● stb (https://github.com/nothings/stb)");
        gui_text("● yocto-gl (https://github.com/xelatihy/yocto-gl)");
        gui_text("● uthash (https://troydhanson.github.io/uthash/)");
        gui_text("● inih (https://github.com/benhoyt/inih)");
        gui_text("● voxelizer (https://github.com/karimnaaji/voxelizer)");
        gui_text("● tinyobjloader (https://github.com/syoyo/tinyobjloader-c)");
        gui_text("● boostrap icons (https://icons.getbootstrap.com)");
        gui_text("● meshoptimizer (https://github.com/zeux/meshoptimizer)");
        gui_text("● imguizmo (https://github.com/CedricGuillemet/ImGuizmo)");
        gui_text("● nativefiledialog (https://github.com/btzy/nativefiledialog-extended)");

        gui_text("Contributors:");
        gui_text("● Michal (https://github.com/YarlBoro)");
        gui_text("● Dustin Willis Webber <dustin.webber@gmail.com>");
        gui_text("● Pablo Hugo Reda <pabloreda@gmail.com>");
        gui_text("● Othelarian (https://github.com/othelarian)");
        gui_text("● Mariusz Pilipczuk (https://gitlab.com/madd-games)");
    }
    return gui_button("OK", 0, 0);
}

int gui_about_scripts_popup(void *data)
{
    char dir[1024] = "";
    const char *examples_url =
        "https://github.com/guillaumechereau/goxel/tree/master/data/scripts";

    script_get_dir(dir, sizeof(dir));

    gui_text("Starting from version 0.12.0 Goxel adds experimental support "
             "for javascript plugins.");
    gui_text("Add your own scripts in the directory:\n%s.", dir);
    gui_text("See some examples at %s.", examples_url);
    return gui_button("OK", 0, 0);
}

static char g_script_editor_buf[65536] = "";

// Script selector state
static char g_script_names[64][128]; // up to 64 scripts
static int  g_script_count = 0;
static int  g_script_selected = -1;

static void collect_script_name(void *user, const char *name)
{
    (void)user;
    if (g_script_count < 64) {
        snprintf(g_script_names[g_script_count], 128, "%s", name);
        g_script_count++;
    }
}

static void refresh_script_list(void)
{
    g_script_count = 0;
    script_iter_all(NULL, collect_script_name);
}

void gui_script_editor_panel(void)
{
    bool req_run = false;
    static char status_msg[256] = "";
    int i;

    // -- Script Selector Combo --
    refresh_script_list();
    if (g_script_count > 0) {
        const char *preview = (g_script_selected >= 0 && g_script_selected < g_script_count)
            ? g_script_names[g_script_selected] : "Load Script...";
        if (gui_combo_begin("##ScriptSelect", preview)) {
            for (i = 0; i < g_script_count; i++) {
                bool sel = (i == g_script_selected);
                if (gui_combo_item(g_script_names[i], sel)) {
                    g_script_selected = i;
                }
            }
            gui_combo_end();
        }
        // Load selected script from file
        if (g_script_selected >= 0 && g_script_selected < g_script_count) {
            gui_row_begin(2);
            if (gui_button("Load Selected", 0, 0)) {
                char spath[1024] = "";
                char scripts_dir[1024] = "";
                if (script_get_dir(scripts_dir, sizeof(scripts_dir))) {
                    snprintf(spath, sizeof(spath), "%s/%s.js",
                             scripts_dir, g_script_names[g_script_selected]);
                    FILE *f = fopen(spath, "rb");
                    if (f) {
                        size_t n = fread(g_script_editor_buf,
                                         1, sizeof(g_script_editor_buf) - 1, f);
                        g_script_editor_buf[n] = '\0';
                        fclose(f);
                        snprintf(status_msg, sizeof(status_msg),
                                 "Loaded: %s", g_script_names[g_script_selected]);
                    } else {
                        snprintf(status_msg, sizeof(status_msg),
                                 "File not found: %s", spath);
                    }
                }
            }
            gui_row_end();
        }
        gui_separator();
    }

    // -- Multiline text editor with line numbers --
    // Draw line numbers on the left using gui_text, then the editor
    {
        int line_count = 1;
        const char *s = g_script_editor_buf;
        while (*s) {
            if (*s == '\n') line_count++;
            s++;
        }
        // Show line count hint
        gui_text("Lines: %d  |  %d / %d bytes",
                 line_count,
                 (int)strlen(g_script_editor_buf),
                 (int)sizeof(g_script_editor_buf));
    }

    gui_input_text_multiline("##ScriptSource", g_script_editor_buf,
                             sizeof(g_script_editor_buf), -1, -100);

    // -- Button row --
    gui_row_begin(3);

    // Execute button
    if (gui_button("Execute", 0, 0)) {
        req_run = true;
    }

    // Load from file
    if (gui_button("Open...", 0, 0)) {
        const char *filters[] = {"*.js", NULL};
        const char *fpath = sys_open_file_dialog("Open Script", NULL,
                                                 filters, "JavaScript");
        if (fpath) {
            FILE *f = fopen(fpath, "rb");
            if (f) {
                size_t n = fread(g_script_editor_buf,
                                 1, sizeof(g_script_editor_buf) - 1, f);
                g_script_editor_buf[n] = '\0';
                fclose(f);
                snprintf(status_msg, sizeof(status_msg), "Opened: %s", fpath);
            }
        }
    }

    // Save to file
    if (gui_button("Save...", 0, 0)) {
        const char *filters[] = {"*.js", NULL};
        const char *fpath = sys_save_file_dialog("Save Script", "script.js",
                                                 filters, "JavaScript");
        if (fpath) {
            FILE *f = fopen(fpath, "wb");
            if (f) {
                fwrite(g_script_editor_buf, 1,
                       strlen(g_script_editor_buf), f);
                fclose(f);
                snprintf(status_msg, sizeof(status_msg), "Saved: %s", fpath);
            }
        }
    }

    gui_row_end();

    // -- Status line --
    if (status_msg[0]) {
        gui_text("%s", status_msg);
    }

    // Run the script
    if (req_run && strlen(g_script_editor_buf) > 0) {
        int ret = script_run_from_str(g_script_editor_buf,
                                      strlen(g_script_editor_buf),
                                      "Editor", 0, NULL);
        if (ret == 0) {
            snprintf(status_msg, sizeof(status_msg), "Script executed OK");
        } else {
            snprintf(status_msg, sizeof(status_msg),
                     "Script error (check console)");
        }
    }
}
