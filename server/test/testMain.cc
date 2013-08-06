/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <glib.h>
#include <stdio.h>

#include "Hatohol.h"
#include "Utils.h"

namespace testMain {
GPid pid;
GMainLoop *loop;

void testEndChildProcess(GPid child_pid, gint status, gpointer data)
{
	const char *pid_file_path = "/var/run/hatohol.pid";
	FILE *pid_file;
	pid_file = fopen(pid_file_path, "r");
	cppcut_assert_not_null(pid_file);
	g_main_loop_quit(loop);
}

gboolean testTimeOutChildProcess(gpointer data)
{
	cut_fail("Timeout to be daemon.");
	return FALSE;
}

bool testMainLoop(void)
{
	loop = g_main_loop_new(NULL, TRUE);
	g_child_watch_add(pid, (GChildWatchFunc)testEndChildProcess, loop);
	g_timeout_add(100, (GSourceFunc)testTimeOutChildProcess, NULL);
	g_main_loop_run(loop);

	return true;
}

void teardown(void)
{
	g_spawn_close_pid(pid);
}

void test_daemonize(void)
{
	const gchar *argv[] = {"../src/hatohol", "--config-db-server", "localhost", NULL};
	const gchar *envp[] = {"LD_LIBRARY_PATH=../src/.libs/", NULL};
	gint stdOut, stdErr;
	GError *error;
	gboolean succeeded;
	gboolean expected = TRUE;
	succeeded = g_spawn_async_with_pipes(NULL,
			const_cast<gchar**>(argv),
			const_cast<gchar**>(envp),
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&pid,
			NULL,
			&stdOut,
			&stdErr,
			&error);
	cppcut_assert_equal(expected, succeeded);
	testMainLoop();
}
}

