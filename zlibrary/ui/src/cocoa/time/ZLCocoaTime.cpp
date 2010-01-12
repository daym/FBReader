/*
 * Copyright (C) 2004-2009 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "ZLCocoaTime.h"

/*
static bool taskFunction(gpointer *data) {
	((ZLRunnable*)data)->run();
	return true;
}
*/

void ZLCocoaTimeManager::addTask(shared_ptr<ZLRunnable> task, int interval) {
	removeTask(task);
	if ((interval > 0) && !task.isNull()) {
		// TODO: implement
		//myHandlers[task] = g_timeout_add(interval, (GSourceFunc)taskFunction, &*task);
	}
}

void ZLCocoaTimeManager::removeTaskInternal(shared_ptr<ZLRunnable> task) {
	// TODO: implement
	/*
	std::map<shared_ptr<ZLRunnable>,int>::iterator it = myHandlers.find(task);
	if (it != myHandlers.end()) {
		g_source_remove(it->second);
		myHandlers.erase(it);
	}
	*/
}