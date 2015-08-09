#include    "clidtp.h"

void CliDTP::init(SockStream & connSockStream)
{ 
	this->connSockStream = connSockStream;
	packet.init();
}
void CliDTP::sendFile(const char *pathname, FILE *fp, uint32_t nslice)
{
	int n;
	uint32_t sindex = 0;
	char buf[MAXLINE];
	// first PUT response
	if(packet.reset(NPACKET), (n = connSockStream.Readn(packet.ps, PACKSIZE)) > 0 ) 
	{
		packet.ntohp();
		if (packet.ps->tagid == TAG_STAT) {
			if (packet.ps->statid == STAT_OK) {
				packet.ps->body[packet.ps->bsize] = 0;
				fprintf(stdout, "%s\n", packet.ps->body);
			} else if (packet.ps->statid == STAT_ERR) {
				packet.ps->body[packet.ps->bsize] = 0;
				fprintf(stdout, "%s\n", packet.ps->body);
				return;
			} else {
				
				Error::msg("CliDTP::sendFile: unknown statid %d", packet.ps->statid);
				packet.print();
				return;
			}
			
		} else {
			Error::msg("CliDTP::sendFile: unknown tagid %d", packet.ps->tagid);
			return;
		}
	}

	char body[PBODYCAP];
	//printf("Send file [%s] now\n", pathname);
	int oldProgress = 0, newProgress = 0;
	fprintf(stderr, "Progress[%s]: %3d%%", pathname, newProgress);
	while( (n = fread(body, sizeof(char), PBODYCAP, fp)) >0 )
	{
		packet.reset(HPACKET);
		packet.fillData(0, nslice, ++sindex, n, body);
		//printf("file_block_length:%d\n",n);
		if(packet.ps->nslice == 0)
		{
			Error::msg("nslice is zero, can not divide\n");
			return;
		}
		newProgress = (packet.ps->sindex*1.0)/packet.ps->nslice*100;
		if (newProgress > oldProgress)
		{
			//printf("\033[2K\r\033[0m");
			fprintf(stderr, "\033[2K\r\033[0mProgress[%s]: %3d%%", pathname, newProgress);
		}
		oldProgress = newProgress;
		//packet.print();
		packet.htonp();
		connSockStream.Writen(packet.ps, PACKSIZE);

	}
	fclose(fp);
	// send EOT
	packet.reset(HPACKET);
	snprintf(buf, MAXLINE, "\033[32mEnd of Tansfer\033[0m (%d slices, last size %d)", nslice, n);
	packet.fillStat(0, STAT_EOT, strlen(buf), buf);
	packet.print();
	packet.htonp();
	connSockStream.Writen(packet.ps, PACKSIZE);

	fprintf(stderr, "\033[32mEnd of Tansfer\033[0m (%d slices, last size %d)\n", nslice, n);
}
void CliDTP::recvFile(const char *pathname, FILE *fp)
{
	int n;
	// first receive response
	if(packet.reset(NPACKET), (n = connSockStream.Readn(packet.ps, PACKSIZE)) > 0 ) 
	{
		packet.ntohp();
		if (packet.ps->tagid == TAG_STAT) {
			if (packet.ps->statid == STAT_OK) {
				packet.ps->body[packet.ps->bsize] = 0;
				fprintf(stdout, "%s\n", packet.ps->body);
			} else if (packet.ps->statid == STAT_ERR){
				packet.ps->body[packet.ps->bsize] = 0;
				fprintf(stderr, "%s\n", packet.ps->body);
				return;
			} else {
				Error::msg("CliDTP::recvFile: unknown statid %d", packet.ps->statid);
				return;
			}
			
		} else {
			Error::msg("CliDTP::recvFile: unknown tagid %d", packet.ps->tagid);
			return;
		}
	}

	// second transfer file
	//fprintf(stdout, "Recieve file now: %s\n", pathname);

	int m;
	int oldProgress = 0, newProgress = 0;
	fprintf(stderr, "Progress[%s]: %3d%%", pathname, newProgress);
	while (packet.reset(NPACKET), (n = connSockStream.Readn(packet.ps, PACKSIZE)) > 0)
	{
		packet.ntohp();
		//packet.print();
		if(packet.ps->tagid == TAG_DATA) {
			m = fwrite(packet.ps->body, sizeof(char), packet.ps->bsize, fp);

			if (m != packet.ps->bsize)
			{
				Error::msg("Recieved slice %u/%u: %hu vs %hu Bytes\n", packet.ps->sindex, packet.ps->nslice, packet.ps->bsize, m);
				fclose(fp);
				return;
			} else {
				newProgress = (packet.ps->sindex*1.0)/packet.ps->nslice*100;
				if (newProgress > oldProgress)
				{
					//printf("\033[2K\r\033[0m");
					fprintf(stderr, "\033[2K\r\033[0mProgress[%s]: %3d%%", pathname, newProgress);
				}
				oldProgress = newProgress;
			}
			//printf("Recieved packet %d: %d vs %d Bytes\n", packet.ps->sindex, packet.ps->bsize, m);
		} else if(packet.ps->tagid == TAG_STAT && packet.ps->statid == STAT_EOT) {
			fclose(fp);
			packet.ps->body[packet.ps->bsize] = 0;
			printf("\n%s\n", packet.ps->body);
			return;
		} else {
			Error::msg("CliDTP::recvFile: unknown tagid %hu with statid %hu", packet.ps->tagid, packet.ps->statid);
			fclose(fp);
			return;
		}
	}
}

int CliDTP::getFileNslice(const char *pathname,uint32_t *pnslice_o)  
{  
 
    unsigned long filesize = 0, n = MAXNSLICE;

    struct stat statbuff;  
    if(stat(pathname, &statbuff) < 0){  
        return -1;  // error
    } else {  
        filesize = statbuff.st_size;  
    }  
    if (filesize % SLICECAP == 0)
	{
		 *pnslice_o = filesize/SLICECAP; 
	} else if ( (n = filesize/SLICECAP + 1) > MAXNSLICE ){
		Error::msg("too large file size: %d\n (MAX: %d)", n, MAXNSLICE);
		return -2; 
	} else {
		 *pnslice_o = filesize/SLICECAP + 1; 
	}
  
    return 1;  
}