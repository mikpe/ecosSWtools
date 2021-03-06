/* Hardware event manager.
   Copyright (C) 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef HW_EVENTS_H
#define HW_EVENTS_H

/* Event manager customized for hardware models.

   This interface is discussed further in sim-events.h. */

struct hw_event;
typedef void (hw_event_callback) (struct hw *me, void *data);

struct hw_event *hw_event_queue_schedule
(struct hw *me,
 signed64 delta_time,
 hw_event_callback *handler,
 void *data);

void hw_event_queue_deschedule
(struct hw *me,
 struct hw_event *event_to_remove);

signed64 hw_event_queue_time
(struct hw *me);

#endif
