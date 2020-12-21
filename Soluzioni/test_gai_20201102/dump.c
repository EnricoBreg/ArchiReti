/*
 *  dump.c - dump functions module
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
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

#define PRINTFLAG(x,flag,file) if((x) & (flag)) fprintf(file, "%s ",#flag)
#define PRINTIFEQUAL(x,flag,file) if((x) == (flag)) fprintf(file, "%s ",#flag)

void dumpsockaddr_in(const struct sockaddr_in *sin, FILE *fp)
{
	fprintf(fp, "family: AF_INET");
	fprintf(fp, "\nport: %d", ntohs(sin->sin_port));
	fprintf(fp, "\naddress: %s\n", inet_ntoa(sin->sin_addr));
}

void dumpsockaddr_in6(const struct sockaddr_in6 *sin6, FILE *fp)
{
	char buf[80];
	
	inet_ntop(AF_INET6, &(sin6->sin6_addr), buf, sizeof(buf));
	
	fprintf(fp, "family: AF_INET6");
	fprintf(fp, "\nport: %d", ntohs(sin6->sin6_port));
	fprintf(fp, "\naddress: %s\n", buf);
}

void dumpsockaddr(const struct sockaddr *sa, FILE *fp)
{
	int no_inetfam = 0;
	
	fprintf(fp, "AI_ADDR:\n");

	switch(sa->sa_family) {
		case AF_INET:	dumpsockaddr_in((struct sockaddr_in *)sa, fp);
				break;
		case AF_INET6:	dumpsockaddr_in6((struct sockaddr_in6 *)sa, fp);
				break;
		default: 	no_inetfam = 1;
	}

	if (no_inetfam) {
		int i;

		fprintf(fp, "family: ");
		PRINTIFEQUAL(sa->sa_family, AF_LOCAL, fp); else
		PRINTIFEQUAL(sa->sa_family, AF_UNSPEC, fp); else
		fprintf(fp, "NONE");

		fprintf(fp, "\ndata: ");
		for (i = 0; i < 14; ++i) fprintf(fp, "%x ", sa->sa_data[i]);
		fprintf(fp, "\n");
	}
}

void dumpaddrinfo(const struct addrinfo *ai, FILE *fp)
{
	fprintf(fp, "flags: ");
	
	PRINTFLAG(ai->ai_flags, AI_PASSIVE, fp);
	PRINTFLAG(ai->ai_flags, AI_CANONNAME, fp);
	PRINTFLAG(ai->ai_flags, AI_NUMERICHOST, fp);
#ifdef AI_NUMERICSERV
	PRINTFLAG(ai->ai_flags, AI_NUMERICSERV, fp);
#endif
#ifdef AI_V4MAPPED
	PRINTFLAG(ai->ai_flags, AI_V4MAPPED, fp);
#endif
#ifdef AI_ALL
	PRINTFLAG(ai->ai_flags, AI_ALL, fp);
#endif
#ifdef AI_ADDRCONFIG
	PRINTFLAG(ai->ai_flags, AI_ADDRCONFIG, fp);
#endif
	
	fprintf(fp, "\nfamily: ");
	
	PRINTIFEQUAL(ai->ai_family, AF_UNSPEC, fp); else
	PRINTIFEQUAL(ai->ai_family, AF_INET, fp); else
	PRINTIFEQUAL(ai->ai_family, AF_INET6, fp);

	fprintf(fp, "\nsocket type (%d): ", ai->ai_socktype);

	PRINTIFEQUAL(ai->ai_socktype, SOCK_STREAM, fp); else
	PRINTIFEQUAL(ai->ai_socktype, SOCK_DGRAM, fp); else
	PRINTIFEQUAL(ai->ai_socktype, SOCK_RAW, fp); else
	PRINTIFEQUAL(ai->ai_socktype, SOCK_RDM, fp); else
	PRINTIFEQUAL(ai->ai_socktype, SOCK_SEQPACKET, fp); else
	fprintf(fp, "NONE");

	fprintf(fp, "\nprotocol: ");

	PRINTIFEQUAL(ai->ai_protocol, IPPROTO_IP, fp); else
	PRINTIFEQUAL(ai->ai_protocol, IPPROTO_IPV6, fp); else
	fprintf(fp, "%d", ai->ai_protocol);

	fprintf(fp, "\nsockaddr struct length: %d", ai->ai_addrlen);
	fprintf(fp, "\ncanonic name: %s\n", 
		(ai->ai_canonname == NULL ? "(null)": ai->ai_canonname));

	if (ai->ai_addr != NULL) dumpsockaddr(ai->ai_addr, fp);
}

