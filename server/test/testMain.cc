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
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <sys/types.h>

#include "Hatohol.h"
#include "Utils.h"
using namespace std;

namespace testMain {
static GPid childPid;
static pid_t grandchildPid = 0;
static guint eventTimeout = 0;

struct functionArg {
	bool *timedOut;
	bool *isEndChildProcess;
	GMainLoop *loop;
};

void endChildProcess(GPid child_pid, gint status, gpointer data)
{
	functionArg *arg = (functionArg *) data;
	*arg->isEndChildProcess = true;
	g_main_loop_quit(arg->loop);
}

gboolean timeOutChildProcess(gpointer data)
{
	functionArg *arg = (functionArg *) data;
	*arg->timedOut = true;
	g_main_loop_quit(arg->loop);
	return FALSE;
}

bool childProcessLoop(void)
{
	functionArg arg;
	bool timedOut = false;
	bool isEndChildProcess = false;
	GMainLoop *loop;
	arg.timedOut = &timedOut;
	arg.isEndChildProcess = &isEndChildProcess;

	loop = g_main_loop_new(NULL, TRUE);
	arg.loop = loop;
	g_child_watch_add(childPid, endChildProcess, &arg);
	eventTimeout = g_timeout_add(100, timeOutChildProcess, &arg);
	g_main_loop_run(loop);

	cppcut_assert_equal(false, timedOut);
	cppcut_assert_equal(true, isEndChildProcess);
	return true;
}

bool parsePIDFile(void)
{
	const char *grandchildPidFilePath = "/var/run/hatohol.pid";
	cut_assert_exist_path(grandchildPidFilePath);
	FILE *grandchildPidFile;
	grandchildPidFile = fopen(grandchildPidFilePath, "r");
	cppcut_assert_not_null(grandchildPidFile);
	cppcut_assert_not_equal(EOF, fscanf(grandchildPidFile, "%d", &grandchildPid));
	cppcut_assert_equal(0, fclose(grandchildPidFile));

	return true;
}

bool parseStatFile(int &parentPid)
{
	stringstream ssStat;
	ssStat << "/proc/" << grandchildPid << "/stat";
	string grandchildProcFilePath = ssStat.str();
	cut_assert_exist_path(grandchildProcFilePath.c_str());
	FILE *grandchildProcFile;
	grandchildProcFile = fopen(grandchildProcFilePath.c_str(), "r");
	cppcut_assert_not_null(grandchildProcFile);
	int grandchildProcPid;
	char comm[11];
	char state;
	cppcut_assert_equal(4, fscanf(grandchildProcFile, "%d (%10s) %c %d ", &grandchildProcPid, comm, &state, &parentPid));
	cppcut_assert_equal(0, fclose(grandchildProcFile));

	return true;
}

bool parseEnvironFile(string makedMagicNumber)
{
	bool isMagicNumber;
	stringstream grandchildProcEnvironPath;
	ifstream grandchildEnvironFile;
	string env;
	grandchildProcEnvironPath << "/proc/" << grandchildPid << "/environ";
	grandchildEnvironFile.open(grandchildProcEnvironPath.str().c_str());
	while (getline(grandchildEnvironFile, env, '\0')) {
		isMagicNumber = env == makedMagicNumber;
		if (isMagicNumber)
			break;
	}
	cppcut_assert_equal(true, isMagicNumber);

	return true;
}

bool makeRandomNumber(string &magicNumber)
{
	srand((unsigned int)time(NULL));
	int randomNumber = rand();
	stringstream ss;
	ss << "MAGICNUMBER=" << randomNumber;
	magicNumber = ss.str();

	return true;
}

bool spawnChildProcess(string magicNumber)
{
	const gchar *argv[] = {"../src/hatohol", "--config-db-server", "localhost", NULL};
	const gchar *envp[] = {"LD_LIBRARY_PATH=../src/.libs/", magicNumber.c_str(), NULL};
	gint stdOut, stdErr;
	GError *error;
	gboolean succeeded;

	succeeded = g_spawn_async_with_pipes(NULL,
			const_cast<gchar**>(argv),
			const_cast<gchar**>(envp),
			G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&childPid,
			NULL,
			&stdOut,
			&stdErr,
			&error);

	return succeeded == TRUE;
}

void teardown(void)
{
	g_spawn_close_pid(childPid);

	if (grandchildPid > 1) {
		kill(grandchildPid, SIGTERM);
		grandchildPid = 0;
	}

	if (eventTimeout != 0) {
		gboolean expected = TRUE;
		cppcut_assert_equal(expected, g_source_remove(eventTimeout));
	}
}

void test_daemonize(void)
{
	int grandchildPpid;
	string magicNumber;

	cppcut_assert_equal(true, makeRandomNumber(magicNumber));
	cppcut_assert_equal(true, spawnChildProcess(magicNumber));
	cppcut_assert_equal(true, childProcessLoop());
	cppcut_assert_equal(true, parsePIDFile());
	cppcut_assert_equal(true, parseStatFile(grandchildPpid));
	cppcut_assert_equal(1, grandchildPpid);
	cppcut_assert_equal(true, parseEnvironFile(magicNumber));
}
}

