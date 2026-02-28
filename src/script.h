/* Goxel 3D voxels editor
 *
 * copyright (c) 2023-present Guillaume Chereau <guillaume@noctua-software.com>
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

#ifndef SCRIPT_H
#define SCRIPT_H

/*
 * Function: script_run
 * Run a lua script from a file.
 */
int script_run_from_file(const char *filename, int argc, const char **argv);

void script_init(void);

/*
 * Reload all scripts dynamically.
 */
void script_reload(void);

/*
 * List all the registered scripts to show in the script menu.
 */
void script_iter_all(void *user, void (*f)(void *user, const char *name));

/*
 * Execute a registered script.
 */
int script_execute(const char *name);

/*
 * Get the scripts directory path (exe_dir/data/scripts).
 * Returns 1 on success, 0 on failure.
 */
int script_get_dir(char *buf, size_t size);

/*
 * Execute arbitrary JavaScript from a string.
 */
int script_run_from_str(
        const char *script, int len, const char *filename, int argc,
        const char **argv);

#endif // SCRIPT_H
