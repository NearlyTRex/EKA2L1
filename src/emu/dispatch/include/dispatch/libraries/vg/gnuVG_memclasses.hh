/*
 * gnuVG - a free Vector Graphics library
 * Copyright (C) 2014 by Anton Persson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __GNUVG_MEMCLASSES_HH
#define __GNUVG_MEMCLASSES_HH

#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>

namespace gnuVG {
	struct GvgArray {
		void* data;
		size_t size;

		GvgArray(size_t first_size) : size(first_size) {
			data = (void*)malloc(size);
		}

		~GvgArray() {
			free(data);
		}

		void resize(size_t new_size) {
			if(new_size <= size) return;

			void* new_data = (void*)realloc(data, new_size);
			data = new_data;
			size = new_size;
		}
	};

	class GvgAllocator {
	private:
		static std::map<void*, GvgArray*> active_arrays;
		static std::vector<GvgArray*> unused_arrays;

	public:
		static void* gvg_alloc(void* /*ignored*/, unsigned int size);
		static void* gvg_realloc(void* /*ignored*/, void* ptr, unsigned int new_size);
		static void gvg_free(void* /*ignored*/, void* ptr);
	};

};

#endif
