#include"client.h"
void init()
{
    char recvbuf[BUF_SIZE];
    char sendbuf[BUF_SIZE];
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAdr;
    char message[BUF_SIZE];
    int strLen,readLen;
    WORD w_req =MAKEWORD(2,2);
    int err;
    err=WSAStartup(w_req,&wsaData);
    if(err!=0)
    {
        std::cout<<"init error"<<std::endl;
    }else {
        std::cout<<"init succeeded"<<std::endl;
    }

    hSocket=socket(AF_INET,SOCK_STREAM,0);
    memset(&servAdr,0,sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=inet_addr("127.0.0.1");
    servAdr.sin_port=htons(10240);

    if(connect(hSocket,(SOCKADDR*)&servAdr,sizeof(servAdr))==SOCKET_ERROR)
        std::cout<<"connect error!"<<std::endl;
    else
    {
        std::cout<<"connected..."<<std::endl;
        while(1)
        {
            
            int nRet=recv(hSocket,recvbuf,sizeof(recvbuf),0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
            std::cout<<"server:"<<recvbuf<<std::endl;
            
            send(hSocket,"pong",5,0);
            
            //send(hSocket,"pong",5,0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
            
        }
    }
        
    
    closesocket(hSocket);
    WSACleanup();
    

}