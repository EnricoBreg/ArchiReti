/*
 *  dump.h - dump functions header
 *
 *  Copyright (C) 2001-2012 Mauro Tortonesi <mauro.tortonesi@unife.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef LIBPRJ6_DUMP_H
#define LIBPRJ6_DUMP_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

void dumpsockaddr_in(const struct sockaddr_in *sin, FILE *fp);
void dumpsockaddr_in6(const struct sockaddr_in6 *sin6, FILE *fp);
void dumpsockaddr(const struct sockaddr *sa, FILE *fp);
void dumpaddrinfo(const struct addrinfo *ai, FILE *fp);

#endif /* LIBPRJ6_DUMP_H */
