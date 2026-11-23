/**************************************************************************/
/*  cook_trace.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "cook_trace.h"

#include "core/object/class_db.h"

void CookTrace::_note(const String &p_path) {
	MutexLock lock(mutex);
	if (active.is_set()) {
		paths.insert(p_path);
	}
}

void CookTrace::begin() {
	MutexLock lock(mutex);
	paths.clear();
	active.set();
}

PackedStringArray CookTrace::_sorted() {
	Vector<String> sorted;
	for (const String &path : paths) {
		sorted.push_back(path);
	}
	sorted.sort();
	PackedStringArray result;
	for (const String &path : sorted) {
		result.push_back(path);
	}
	return result;
}

PackedStringArray CookTrace::snapshot() {
	MutexLock lock(mutex);
	return _sorted();
}

PackedStringArray CookTrace::end() {
	MutexLock lock(mutex);
	active.clear();
	PackedStringArray result = _sorted();
	paths.clear();
	return result;
}

bool CookTrace::is_active() const {
	return active.is_set();
}

void CookTrace::_bind_methods() {
	ClassDB::bind_method(D_METHOD("begin"), &CookTrace::begin);
	ClassDB::bind_method(D_METHOD("snapshot"), &CookTrace::snapshot);
	ClassDB::bind_method(D_METHOD("end"), &CookTrace::end);
	ClassDB::bind_method(D_METHOD("is_active"), &CookTrace::is_active);
}

CookTrace::CookTrace() {
	singleton = this;
}

CookTrace::~CookTrace() {
	active.clear();
	singleton = nullptr;
}
