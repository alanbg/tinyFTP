#include    "srvpi.h"

void SrvPI::run(int connfd)
{
	connSockStream.init(connfd);

	packet.reset(NPACKET);
	if ( connSockStream.Readn(packet.ps, PACKSIZE) == 0)
	            Error::quit_pthread("client terminated prematurely");
    packet.ntohp();
    packet.print();
    if (packet.ps->tagid == TAG_CMD)
    {
    	switch(packet.ps->cmdid)
		{
			case GET:
				cmdGET();
				break;
			case PUT:
				cmdPUT();
				break;
			case LS:
				cmdLS();
				break;
			default:
				Error::msg("Sorry! this command function not finished yet.\n");
				break;
		}
    } else {
    	Error::msg("Error: received packet is not a command.\n");
    }
	
}
void SrvPI::cmd2pack(uint32_t sesid, uint16_t cmdid, std::vector<string> & cmdVector)
{
	packet.reset(HPACKET);

	//uint16_t bsize = 18;
	//char body[PBODYCAP] = "Server: echo, ctr packet.";
	//packet.init(sesid, cmdid, bsize, body);
}

void SrvPI::cmd2pack(uint32_t sesid, uint16_t cmdid, uint16_t bsize, char body[PBODYCAP])
{
	packet.reset(HPACKET);
	//packet.init(sesid, cmdid, bsize, body);
}

void SrvPI::cmd2pack(uint32_t sesid, uint16_t cmdid, string str)
{
	packet.reset(HPACKET);
	if(str.size() > 65535)
		Error::msg("body size overflow");
	//uint16_t bsize = str.size();
	//char body[PBODYCAP];
	//std::strcpy(body, str.c_str());
	//packet.init(sesid, cmdid, bsize, body);
}
void SrvPI::cmdGET()
{
	printf("GET request\n");

	srvDTP.init(connSockStream);
	packet.ps->body[packet.ps->bsize] = 0;
	srvDTP.sendFile(packet.ps->body);
}
void SrvPI::cmdPUT()
{
	printf("PUT request\n");
	
	srvDTP.init(connSockStream);
	packet.ps->body[packet.ps->bsize] = 0;
	srvDTP.recvFile(packet.ps->body);
}
void SrvPI::cmdLS()
{
	printf("LS request\n");
	char buf[MAXLINE];
	char body[PBODYCAP];
	string sbody;
	DIR* dir;

	if (packet.ps->bsize == 0)
	{
		dir= opendir(".");
	} else {
		packet.ps->body[packet.ps->bsize] = 0;
		dir= opendir(packet.ps->body);
	}
	if(!dir)
		Error::sys("opendir()");
	struct dirent* e;
	int cnt = 0;
	while( (e = readdir(dir)) )
	{
		//packet.reset(HPACKET);
		//sprintf(body, "%s\t%s", e->d_type == 4 ? "DIR:" : e->d_type == 8 ? "FILE:" : "UNDEF:", e->d_name);
		if (e->d_type == 4)
		{
			if (strlen(e->d_name) > 10)
			{
				snprintf(buf, MAXLINE, "\n\033[36m%s\033[0m\n", e->d_name);
				cnt = 0;
			} else {
				snprintf(buf, MAXLINE, "\033[36m%-10s\033[0m\t", e->d_name);
			}
			sbody += buf;
			//snprintf(body, PBODYCAP, "\033[34m%s\033[0m", e->d_name);
		} else if(e->d_type == 8) {
			if (strlen(e->d_name) > 10)
			{
				snprintf(buf, MAXLINE, "\n%s\n", e->d_name);
				cnt = 0;
			} else {
				snprintf(buf, MAXLINE, "%-10s\t", e->d_name);
			}
			sbody += buf;
			//snprintf(body, PBODYCAP, "%s", e->d_name);
		} else {
			if (strlen(e->d_name) > 10)
			{
				snprintf(buf, MAXLINE, "\n%s\n", e->d_name);
				cnt = 0;
			} else {
				snprintf(buf, MAXLINE, "%-10s\t", e->d_name);
			}
			sbody += buf;
			//snprintf(body, PBODYCAP, "%s", e->d_name);
		}
		if (((++cnt) % 4) == 0)
		{
			sbody += "\n";
		}
		// packet.fillData(0, 0, 0, strlen(body), body);
		// packet.htonp();
		// connSockStream.Writen(packet.ps, PACKSIZE);
	}
	if (sbody.size() > PBODYCAP)
	{
		Error::msg("LS: too many dir ents\n");
		return;
	}
	packet.reset(HPACKET);
	std::strcpy(body, sbody.c_str());
	packet.fillData(0, 0, 0, strlen(body), body);
	packet.htonp();
	connSockStream.Writen(packet.ps, PACKSIZE);

	// send EOT
	packet.reset(HPACKET);
	snprintf(buf, MAXLINE, "\033[32mEnd of Tansfer\033[0m");
	packet.fillStat(0, STAT_EOT, strlen(buf), buf);
	//packet.print();
	packet.htonp();
	connSockStream.Writen(packet.ps, PACKSIZE);

}
void SrvPI::cmdCD()
{
	printf("CD request\n");
}
void SrvPI::cmdDELE()
{
	printf("DELE request\n");
}