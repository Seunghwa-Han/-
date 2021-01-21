#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;

	if(argc!=2) {  //PORT번호 입력안하면 종료하기 
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
    
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);   //PF_INET은 IPv4인터넷 프로토콜 체계, SOCK_STREAM은 연결지향형 데이터 전송 -> TCP 소켓 생성 
    
	//인터넷 주소정보 초기화 
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)  //IP주소와 PORT번호 할당 
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)  //연결요청 가능상태로 변경 
		error_handling("listen() error");
    
    printf("Welcome to GBC Network Server!\n");
	
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr); 
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);  //연결요청에 대한 수락 
		
		//뮤텍스 락: 한 스레드가 일하고 있으면 기다렸다가 일하는 것, 독립적으로 실행됨 
		pthread_mutex_lock(&mutx);  
		clnt_socks[clnt_cnt++]=clnt_sock;  //새로운 연결이 형성될 때마다 변수 clnt_cnt와 배열 clnt_socks에 해당정보를 등록 
		pthread_mutex_unlock(&mutx);    
	    
        //추가된 클라이언트에게 서비스를 제공하기 위한 스레드 생성 
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //handle_clnt함수로 분기, 인자로 clnt_sock 변수를 넘겨준다 
		pthread_detach(t_id);  //종료된 스레드가 메모리에서 완전히 소멸되도록 함
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));  //네트워크 바이트 정렬 방식의 4바이트 정수의 IPv4 주소를 문자열 주소로 표현해서 출력 
	}
	close(serv_sock);
	return 0;
}
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);   //형변환 
	int str_len=0, i;
	char msg[BUF_SIZE];
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)  //읽을게 없을 때까지 메시지를 읽음 
		send_msg(msg, str_len);  //연결된 모든 클라이언트에게 메시지를 전송하는 기능 
	
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i++<clnt_cnt-1)
				clnt_socks[i]=clnt_socks[i+1];
			break;
		}
	}
	clnt_cnt--; 
	pthread_mutex_unlock(&mutx);
	close(clnt_sock); 
	return NULL;
}
void send_msg(char * msg, int len)   // send to all
{
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len); //메시지를 모든 클라이언트에게 전송 
	pthread_mutex_unlock(&mutx);
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}