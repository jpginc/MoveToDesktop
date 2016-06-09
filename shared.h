/**
* MoveToDesktop
*
* Copyright (C) 2015-2016 by Tobias Salzmann
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define MAXDESKTOPS 255

#ifdef _DEBUG
#define LOGFILE "%temp%\\MoveToDesktop.log"
#include <stdarg.h>
inline void Log(char *message, ...)
{
	FILE *file;

	TCHAR logFile[MAX_PATH] = { 0 };
	ExpandEnvironmentStrings(LOGFILE, logFile, _countof(logFile));
	fopen_s(&file, logFile, "a");

	if (file == NULL) {
		return;
	}
	else
	{
		char exepath[MAX_PATH];
		char inpmsg[1024] = "";
		char finalmsg[1024 + MAX_PATH] = "";
		va_list ptr;
		va_start(ptr, message);
		vsprintf_s(inpmsg, sizeof(inpmsg), message, ptr);
		va_end(ptr);
		GetModuleFileName(0, exepath, MAX_PATH);

		sprintf_s(finalmsg, sizeof(finalmsg), "%s: %s\n", exepath, inpmsg);
		fputs(finalmsg, file);
		fclose(file);
	}

	if (file)
		fclose(file);
}
#else
#define Log
#endif