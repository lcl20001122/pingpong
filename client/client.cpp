#include"client.h"
SOCKET hSocket;
SOCKADDR_IN servAdr;

HANDLE hConnEvent;
HANDLE hWorkEvent;
std::mutex mtx;
std::vector<SOCKET> v;
DWORD WINAPI _ConnectionThread(LPVOID lpParam)
{
    int ThreadNum=5;

    
   
    for(int i=0;i<ThreadNum;i++)
    {
        ResetEvent(hConnEvent);
        hSocket=socket(AF_INET,SOCK_STREAM,0);
        memset(&servAdr,0,sizeof(servAdr));
        servAdr.sin_family=AF_INET;
        servAdr.sin_addr.s_addr=inet_addr("127.0.0.1");
        servAdr.sin_port=htons(10240);


        if(connect(hSocket,(SOCKADDR*)&servAdr,sizeof(servAdr))==SOCKET_ERROR)
            std::cout<<"connect error!"<<WSAGetLastError()<<std::endl;
        else
        {
            std::cout<<"client:"<<i<<"connected..."<<std::endl;
             v.push_back(hSocket);
             SetEvent(hConnEvent);
            //CreateThread(NULL,0,_WorkerThread,&hSocket,0,NULL);
            
        }
        

        
    }
    
    
    
    return 0;

}
/*DWORD WINAPI _WorkerThread(LPVOID lpParam)
{
    SOCKET *sock =(SOCKET*)lpParam;
    
    

    char recvbuf[1024];
    while(1)
        {
            int nRet=recv(*sock,recvbuf,sizeof(recvbuf),0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
            std::cout<<"server:"<<recvbuf<<std::endl;
            
            if(send(*sock,"pong",5,0))
            {
                std::cout<<"send over"<<std::endl;
            }
            
            //send(hSocket,"pong",5,0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
            
        } 
        SetEvent(hWorkEvent);
        return 0;
        
}*/
void init()
{
    char recvbuf[BUF_SIZE];
    char sendbuf[BUF_SIZE];
    WSADATA wsaData;

    
    char message[BUF_SIZE];
    int strLen,readLen;
    WORD w_req =MAKEWORD(2,2);
    int err;
    hConnEvent=CreateEvent(NULL, TRUE, FALSE, NULL); 
    hWorkEvent=CreateEvent(NULL, TRUE, FALSE, NULL);   
    err=WSAStartup(w_req,&wsaData);
    if(err!=0)
    {
        std::cout<<"init error"<<std::endl;
    }else {
        std::cout<<"init succeeded"<<std::endl;
    }

 
   
    CreateThread(NULL,0,_ConnectionThread,0,0,NULL);
    WaitForSingleObject(hConnEvent, INFINITE);


    while(1)
    {
        for(int i=0;i<v.size();i++)
        {
             char recvbuf[1024];

            int nRet=recv(v[i],recvbuf,sizeof(recvbuf),0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
            std::cout<<"server:"<<recvbuf<<std::endl;
            
            if(send(v[i],"pong",5,0))
            {
                std::cout<<"send over"<<std::endl;
            }
            
            //send(hSocket,"pong",5,0);
            if (nRet == SOCKET_ERROR)
		    { 
			    int nErr = WSAGetLastError();
			    std::cout << "SOCKET_ERROR:"<<std::endl;
			    break;
		    }
        }
    }
    
  
    
    
    
    WaitForSingleObject(hWorkEvent, INFINITE);
    
       
        //q.pop();
    

    /*if(connect(hSocket,(SOCKADDR*)&servAdr,sizeof(servAdr))==SOCKET_ERROR)
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
    }*/
        
    

    //closesocket(hSocket);
    WSACleanup();
    

}
