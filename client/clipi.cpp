#include    "clipi.h"

CliPI::CliPI(const char *host)
{
    Socket cliSocket(CLI_SOCKET, host, CTRPORT);
    connfd = cliSocket.init();
    connSockStream.init(connfd);
}

// void CliPI::init(const char *host)
// {
// 	int connfd;

//     Socket cliSocket(CLI_SOCKET, host, CTRPORT);
//     connfd = cliSocket.init();
//     connSockStream.init(connfd);

// }
// void CliPI::sendOnePacket()
// {
	
// }

void CliPI::recvOnePacket()
{
	int n;
	packet.reset(NPACKET);
	if ( (n = connSockStream.Readn(packet.getPs(), PACKSIZE)) == 0)
	{
		Socket::tcpClose(connfd);
		Error::quit("server terminated prematurely");
	} else if (n < 0){
		Error::ret("connSockStream.Readn()");
		Error::quit("socket connection exception");
	}
	packet.ntohp();
	//packet.print();
}

void CliPI::run(uint16_t cmdid, std::vector<string> & cmdVector)
{
	switch(cmdid)
	{
		case USER:
			cmdUSER(cmdVector);
			break;
		case PASS:
			cmdPASS(cmdVector);
			break;
		case GET:
			cmdGET(cmdVector);
			break;
		case PUT:
			cmdPUT(cmdVector);
			break;
		case LS:
			cmdLS(cmdVector);
			break;
		case LLS:
			cmdLLS(cmdVector);
			break;
		case CD:
			cmdCD(cmdVector);
			break;
		case LCD:
			cmdLCD(cmdVector);
			break;
		case RM:
			cmdRM(cmdVector);
			break;
		case LRM:
			cmdLRM(cmdVector);
			break;	
		case PWD:
			cmdPWD(cmdVector);
			break;
		case LPWD:
			cmdLPWD(cmdVector);
			break;
		case MKDIR:
			cmdMKDIR(cmdVector);
			break;
		case LMKDIR:
			cmdLMKDIR(cmdVector);
			break;
		default:
			Error::msg("Client: Sorry! this command function not finished yet.\n");
			break;
	}
	
}
void CliPI::cmd2pack(uint32_t sesid, uint16_t cmdid, std::vector<string> & cmdVector)
{
	packet.reset(HPACKET);

	uint32_t nslice = 0;
	uint32_t sindex = 0;
	uint16_t statid = 0;
	uint16_t bsize = 0;
	char body[PBODYCAP];
	if(cmdVector.size() > 1)
	{
		strcpy(body,cmdVector[1].c_str());
		bsize = strlen(body); 
	}

	// Error::msg("body: %s\n", body);
	packet.fill(TAG_CMD, cmdid, statid, nslice, sindex, bsize, body);
	//packet.print();
	packet.htonp(); 
}

void CliPI::pass2pack(uint32_t sesid, uint16_t cmdid, std::vector<string> & cmdVector)
{
	packet.reset(HPACKET);

	uint32_t nslice = 0;
	uint32_t sindex = 0;
	uint16_t statid = 0;
	uint16_t bsize = 0;
	char body[PBODYCAP];
	string params;
	vector<string>::iterator iter=cmdVector.begin();
	params += *iter;
	for (++iter; iter!=cmdVector.end(); ++iter)
   	{
   		params += "\t" + *iter;
   	}
   	//cout << params << endl;
   	strcpy(body, params.c_str());
	bsize = strlen(body); 

	// Error::msg("body: %s\n", body);
	packet.fill(TAG_CMD, cmdid, statid, nslice, sindex, bsize, body);
	//packet.print();
	packet.htonp(); 
}

bool CliPI::cmdUSER(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 1)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: [username]");
		return false;
	} else {
		return true;
	}
 
}

bool CliPI::cmdPASS(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: [password]");
		for (vector<string>::iterator iter=cmdVector.begin(); iter!=cmdVector.end(); ++iter)
	   	{
	    	std::cout << *iter << '\n';
	   	}
		return false;
	}

	pass2pack(0, PASS, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			// init userID, same as session id
			char buf[MAXLINE];
			snprintf (buf, MAXLINE, "%u", packet.getSesid());
			userID = buf;
			cout<< packet.getSBody() << endl;
			return true;
		} else if (packet.getStatid() == STAT_ERR){
			cerr<< packet.getSBody() << endl;
			return false;
		} else {
			Error::msg("CliPI::cmdPASS: unknown statid %d", packet.getStatid());
			return false;
		}
		
	} else {
		Error::msg("CliPI::cmdPASS: unknown tagid %d", packet.getTagid());
		return false;
	}
 
}

void CliPI::cmdGET(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: get [FILE]");
		return;
	}
	char pathname[MAXLINE];
	char buf[MAXLINE];
	strcpy(pathname,cmdVector[1].c_str()); 
	FILE *fp;
	if ((access(pathname,F_OK)) == 0) {
		snprintf(buf, MAXLINE, "File [%s] already exists, overwrite (y/n) ? ", pathname);
		//Error::msg("%s", buf);
		string inputline, word;
		vector<string> paramVector;
		while (printf("%s", buf), getline(std::cin, inputline))
    	{
			paramVector.clear();
		    std::istringstream is(inputline);
		    while(is >> word)
		        paramVector.push_back(word);

		    // if user enter nothing, assume special anonymous user
		    if (paramVector.size() == 1){
		       if (paramVector[0] == "y"){
		       		break;
		       } else if (paramVector[0] == "n"){
		       		return;
		       } else {
		       		continue;
		       }
		    } else {
		        continue;
		    }
		}
	}

	if ( (fp = fopen(pathname, "wb")) == NULL) {
		Error::msg("%s", strerror_r(errno, buf, MAXLINE));
		return;
	} else {
		// command to packet
		cmd2pack(0, GET, cmdVector);
	    connSockStream.Writen(packet.getPs(), PACKSIZE);
	}

    // pathname exist on server? need test
    CliDTP cliDTP(this->connSockStream, &(this->packet), this->connfd);
    //cliDTP.init(connSockStream, packet);
	cliDTP.recvFile(pathname, fp);
 
}
void CliPI::cmdPUT(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: put [FILE]");
		return;
	}

	char pathname[MAXLINE];
	char buf[MAXLINE];
	uint32_t nslice = 0;

	strcpy(pathname,cmdVector[1].c_str()); 
	struct stat statBuf;
    int n = stat(cmdVector[1].c_str(), &statBuf);
    if(!n) // stat call success
	{	
		if (S_ISREG(statBuf.st_mode)){
			;
	    } else if (S_ISDIR(statBuf.st_mode)){
			cout << "put: cannot upload [" << cmdVector[1] << "]: Is a directory" << endl;
			return;
	    } else {
	    	cout << "put: [" << cmdVector[1] << "] not a regular file or directory" << endl;
			return;
	    }
		
	} else { // stat error
		Error::msg("%s", strerror_r(errno, buf, MAXLINE));
		return;
	}

	
	FILE *fp;
	if ( (fp = fopen(pathname, "rb")) == NULL)
	{
		Error::msg("%s", strerror_r(errno, buf, MAXLINE));
		return;
	} else if ( (n = getFileNslice(pathname, &nslice)) < 0)  {
		if ( n == -2) {
			Error::msg("Too large file size.", buf);
		} else {
			Error::msg("File stat error.");
		}
		return;
	} else {
		// command to packet
		cmd2pack(0, PUT, cmdVector);
	    connSockStream.Writen(packet.getPs(), PACKSIZE);
	}

	while (1)
	{
		recvOnePacket();
		if (packet.getTagid() == TAG_STAT) {
			if (packet.getStatid() == STAT_OK) {
				cout << packet.getSBody() <<endl;
				// must contain sesssion id
				CliDTP cliDTP(this->connSockStream, &(this->packet), this->connfd);
				cliDTP.sendFile(pathname, fp, nslice);
				break;
			} else if (packet.getStatid() == STAT_CFM) {
				string inputline, word;
				vector<string> paramVector;
				while (printf("%s", packet.getSBody().c_str()), getline(std::cin, inputline))
		    	{
					paramVector.clear();
				    std::istringstream is(inputline);
				    while(is >> word)
				        paramVector.push_back(word);

				    // if user enter nothing, assume special anonymous user
				    if (paramVector.size() == 1){
				       if (paramVector[0] == "y"){
				       		packet.sendSTAT_CFM(connSockStream, "y");
				       		break;
				       } else if (paramVector[0] == "n"){
				       		packet.sendSTAT_CFM(connSockStream, "n");
				       		return;
				       } else {
				       		continue;
				       }
				    } else {
				        continue;
				    }
				}
			} else if (packet.getStatid() == STAT_ERR) {
				cerr << packet.getSBody() <<endl;
				return;
			} else {
				Error::msg("CliDTP::sendFile: unknown statid %d", packet.getStatid());
				packet.print();
				return;
			}
			
		} else {
			Error::msg("CliDTP::sendFile: unknown tagid %d", packet.getTagid());
			packet.print();
			return;
		}
	}
	
}
void CliPI::cmdLS(std::vector<string> & cmdVector)
{
	if(cmdVector.size() > 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: ls [DIR]");
		return;
	}
	
	cmd2pack(0, LS, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			cout<< packet.getSBody() << endl;
		} else if (packet.getStatid() == STAT_ERR){
			cerr<< packet.getSBody() << endl;
			return;
		} else {
			Error::msg("unknown statid %d", packet.getStatid());
			return;
		}
		
	} else {
		Error::msg("unknown tagid %d", packet.getTagid());
		return;
	}

	while(1) 
	{
		recvOnePacket();
		if (packet.getTagid() == TAG_DATA) {
			cout<< packet.getSBody() << endl;
			
		} else if (packet.getTagid() == TAG_STAT && packet.getStatid() == STAT_EOT){
			cout<< packet.getSBody() << endl;
			break;
		}
	}
}

void CliPI::cmdLLS(std::vector<string> & cmdVector)
{
	string shellCMD = "ls --color=auto";
	for (auto it = cmdVector.begin() + 1; it != cmdVector.end(); ++it){
       	//std::cout << *it << std::endl;
       	shellCMD += " " + *it;
	}
	if (system(shellCMD.c_str()) == -1) {
		char buf[MAXLINE];
		std::cout << "system(): " << strerror_r(errno, buf, MAXLINE) << std::endl;
	}
}

void CliPI::cmdCD(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: cd [DIR]");
		return;
	}

	cmd2pack(0, CD, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			cout << packet.getSBody() <<endl;
			return;
		} else if (packet.getStatid() == STAT_ERR){
			cerr << packet.getSBody() <<endl;
			return;
		} else {
			Error::msg("CliPI::recvFile: unknown statid %d", packet.getStatid());
			return;
		}
		
	} else {
		Error::msg("CliPI::recvFile: unknown tagid %d", packet.getTagid());
		return;
	}
 
}

void CliPI::cmdLCD(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: cd [DIR]");
		return;
	}

	int n;
	char buf[MAXLINE];
	if( (n = chdir(cmdVector[1].c_str())) == -1)
	{
		// GNU-specific strerror_r: char *strerror_r(int errnum, char *buf, size_t buflen);
		Error::msg("lcd: %s\n", strerror_r(errno, buf, MAXLINE));
		return;
	}
}

void CliPI::cmdRM(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: rm [FILE|DIR]");
		return;
	}

	cmd2pack(0, RM, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			cout << packet.getSBody() <<endl;
			return;
		} else if (packet.getStatid() == STAT_ERR){
			cerr << packet.getSBody() <<endl;
			return;
		} else {
			Error::msg("unknown statid %d", packet.getStatid());
			return;
		}
		
	} else {
		Error::msg("unknown tagid %d", packet.getTagid());
		return;
	}
	
}

void CliPI::cmdLRM(std::vector<string> & cmdVector)
{
	string shellCMD = "rm";
	for (auto it = cmdVector.begin() + 1; it != cmdVector.end(); ++it){
       	//std::cout << *it << std::endl;
       	shellCMD += " " + *it;
	}
	if (system(shellCMD.c_str()) == -1) {
		char buf[MAXLINE];
		std::cout << "system(): " << strerror_r(errno, buf, MAXLINE) << std::endl;
	}
}

void CliPI::cmdPWD(std::vector<string> & cmdVector)
{
	if(cmdVector.size() > 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: pwd [-a]");
		return;
	} else if (cmdVector.size() == 2 && cmdVector[1] != "-a")
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: pwd [-a]");
		return;
	}

	cmd2pack(0, PWD, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			cout << packet.getSBody() <<endl;
			return;
		} else if (packet.getStatid() == STAT_ERR){
			cerr << packet.getSBody() <<endl;
			return;
		} else {
			Error::msg("unknown statid %d", packet.getStatid());
			return;
		}
		
	} else {
		Error::msg("unknown tagid %d", packet.getTagid());
		return;
	}
}

void CliPI::cmdLPWD(std::vector<string> & cmdVector)
{
	string shellCMD = "pwd";
	for (auto it = cmdVector.begin() + 1; it != cmdVector.end(); ++it){
       	//std::cout << *it << std::endl;
       	shellCMD += " " + *it;
	}
	if (system(shellCMD.c_str()) == -1) {
		char buf[MAXLINE];
		std::cout << "system(): " << strerror_r(errno, buf, MAXLINE) << std::endl;
	}
}

void CliPI::cmdMKDIR(std::vector<string> & cmdVector)
{
	if(cmdVector.size() != 2)
	{
		Error::msg("\033[31mIllegal Input\033[0m\nUsage: mkdir [DIR]");
		return;
	}

	cmd2pack(0, MKDIR, cmdVector);
	connSockStream.Writen(packet.getPs(), PACKSIZE);

	// first receive response
	recvOnePacket();
	if (packet.getTagid() == TAG_STAT) {
		if (packet.getStatid() == STAT_OK) {
			cout << packet.getSBody() <<endl;
			return;
		} else if (packet.getStatid() == STAT_ERR){
			cerr << packet.getSBody() <<endl;
			return;
		} else {
			Error::msg("CliPI::recvFile: unknown statid %d", packet.getStatid());
			return;
		}
		
	} else {
		Error::msg("CliPI::recvFile: unknown tagid %d", packet.getTagid());
		return;
	}
}


void CliPI::cmdLMKDIR(std::vector<string> & cmdVector)
{
	string shellCMD = "mkdir";
	for (auto it = cmdVector.begin() + 1; it != cmdVector.end(); ++it){
       	//std::cout << *it << std::endl;
       	shellCMD += " " + *it;
	}
	if (system(shellCMD.c_str()) == -1) {
		char buf[MAXLINE];
		std::cout << "system(): " << strerror_r(errno, buf, MAXLINE) << std::endl;
	}
}

int CliPI::getFileNslice(const char *pathname, uint32_t *pnslice_o)  
{  
 
    unsigned long filesize = 0, n = MAXNSLICE;

    struct stat statbuff;  
    if(stat(pathname, &statbuff) < 0){  
        return -1;  // error
    } else {  
        if (statbuff.st_size == 0)
		{
			return 0; // file is empty.
		} else {
			filesize = statbuff.st_size;  
		}  
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
  	//printf("getFileNslice nslice: %d\n", *pnslice_o);
    return 1;  
}
