#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100
#define NAME_SIZE 20
	
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
	
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
	
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;   //스레드 : 다중일처리 
	void * thread_return;

	//IP주소, PORT번호, 닉네임 다 입력 안하면 종료하기 
	if(argc!=4) {  
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	 }
	
	sprintf(name, "[%s]", argv[3]);         //닉네임 변수에 입력받은 닉네임 입력하기 
	sock=socket(PF_INET, SOCK_STREAM, 0);   //IPv4, TCP 소켓 생성 
	
	//인터넷 주소정보 초기화 
	memset(&serv_addr, 0, sizeof(serv_addr));     //구조체 변수 serv_addr의 모든 멤버를 0으로 초기화 -> 구조체 멤버 sin_zero를 0으로 초기화하기 위해 
	serv_addr.sin_family=AF_INET;                 //주소체계 지정 
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]); //문자열 기반의 IP주소 초기화 
	serv_addr.sin_port=htons(atoi(argv[2]));      //문자열 기반의 PORT번호 초기화 

    //connect함수 호출을 통해서 서버 프로그램에 연결 요청  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)   
		error_handling("connect() error");
    
    printf("Welcome to GBC Network Client!\n");
	
	//스레드 생성 
	//첫번째 인자 : pthread 식별자로 thread가 성공적으로 생성되면 thread 식별값이 주어짐
	//세번째 인자 : pthread로 분기할 함수. 반환값이 void* 타입이고 매개변수도 void* 으로 선언된 함수만 가능
	//네번째 인자 : 분기할 함수로 넘겨줄 인자값. 어떤 자료형을 넘겨줄 지 모르기 때문에 void형으로 넘겨주고 상황에 맞게 분기하는 함수내에서 원래의 자료형으로 캐스팅해서 사용하면 됨 
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); //send_msg함수로 분기, 인자로는 sock을 넘겨준다 
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); //recv_msg함수로 분기, 인자로는 sock을 넘겨준다 

	//pthread가 종료될 때까지 기다리다가 특정 pthread가 종료시 자원 해제시켜줌 
	pthread_join(snd_thread, &thread_return);  
	pthread_join(rcv_thread, &thread_return);
	//소켓을 닫고 통신 종료 
	close(sock);   
	return 0;
}
	
void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);  //형변환 
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1) 
	{
		fgets(msg, BUF_SIZE, stdin);   //키보드로 msg에 메세지 입력받기 
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) //q나 Q입력받으면 종료 
		{
			close(sock);
			exit(0);
		}
		sprintf(name_msg,"%s %s", name, msg);  //name_msg 문자열 변수에 닉네임이랑 입력받았던 메시지 입력하기 
		write(sock, name_msg, strlen(name_msg));  //write: 데이터 전송 기능의 함수 (입력받은 메시지를 서버로 전송)
	}
	return NULL;
}
	
void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);  //형변환 
	char name_msg[NAME_SIZE+BUF_SIZE]; 
	int str_len;
	while(1)
	{                                         
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1); //서버로부터 데이터를 수신, read: 데이터를 입력(수신)하는 기능의 함수 (성공하면 수신한 바이트 수 반환)-> name_msg 바이트 수 
		if(str_len==-1) 
			return (void*)-1;
		name_msg[str_len]=0;  //마지막에 널문자 넣어주기 
		fputs(name_msg, stdout);  //표준출력으로 화면에 name_msg 출력하기 
	}
	return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}