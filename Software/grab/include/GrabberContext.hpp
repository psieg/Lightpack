/*
 * GrabberContext.hpp
 *
 *	Created on: 12/24/2013
 *		Project: Prismatik
 *
 *	Copyright (c) 2013 tim
 *
 *	Lightpack is an open-source, USB content-driving ambient lighting
 *	hardware.
 *
 *	Prismatik is a free, open-source software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Prismatik and Lightpack files is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GRABBERCONTEXT_HPP
#define GRABBERCONTEXT_HPP

#include <QList>
#include <QRgb>
#include <QRgba64>

class GrabWidget;

struct AllocatedBuf {
	AllocatedBuf()
		: size(0)
		, ptr(NULL)
		, isAvail(true)
	{}

	size_t size;
	unsigned char * ptr;
	bool isAvail;
};

class GrabberContext {
public:
	GrabberContext()
	{}

	~GrabberContext(){
//		releaseAllBufs();
//		freeReleasedBufs();
	}

	unsigned char * queryBuf(size_t reqSize) {
		AllocatedBuf *bestFitBuf = NULL;
		for (int i = 0; i < _allocatedBufs.size(); ++i) {
			if (_allocatedBufs[i]->isAvail
				&& _allocatedBufs[i]->size >= reqSize
				&& ( bestFitBuf == NULL
					|| _allocatedBufs[i]->size < bestFitBuf->size))
			{
				bestFitBuf = _allocatedBufs[i];
			}
		}
		if (bestFitBuf == NULL) {
			bestFitBuf = new AllocatedBuf();
			bestFitBuf->ptr = (unsigned char *)malloc(reqSize);

			if (bestFitBuf->ptr == NULL)
				return NULL;

			bestFitBuf->size = reqSize;
			_allocatedBufs.append(bestFitBuf);
		}
		return bestFitBuf->ptr;
	}

	void releaseAllBufs() {
		for (QList<AllocatedBuf*>::const_iterator iter = _allocatedBufs.cbegin(); iter != _allocatedBufs.end(); ++iter) {
			(*iter)->isAvail = true;
		}
	}

	void freeReleasedBufs() {
		for (QList<AllocatedBuf*>::iterator iter = _allocatedBufs.begin(); iter != _allocatedBufs.end(); ++iter) {
			if ((*iter)->isAvail) {
				AllocatedBuf *buf = *iter;
				free(buf->ptr);
				_allocatedBufs.removeAll(buf);
				delete buf;
			}
		}
	}
public:
	QList<GrabWidget *> *grabWidgets;
	QList<QRgba64> *grabResult;


private:
	QList<AllocatedBuf *> _allocatedBufs;
};


#endif // GRABBERCONTEXT_HPP
