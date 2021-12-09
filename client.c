#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 5555

int socket_fd;

int be_invited = 0;
int another_player_fd = 0;
int game_start = 0;
int turned_me = 0;
char ox[11];
int watch = 0;

typedef struct sockaddr SA;

void logout();

void print_table(){
	printf("------\n");
	printf("%c|%c|%c\n", ox[1], ox[2], ox[3]);
	printf("------\n");
	printf("%c|%c|%c\n", ox[4], ox[5], ox[6]);
	printf("------\n");
	printf("%c|%c|%c\n", ox[7], ox[8], ox[9]);
}

// int isfair()
// {
// 	int i;
// 	for (i = 1; i <= 9; i++)
// 	{
// 		if (ox[i] == ' ')
// 			return 0;
// 	}
// 	return 1;
// }

int iswin(char le){
	if (ox[1] == le && ox[2] == le && ox[3] == le)
		return 1;
	if (ox[4] == le && ox[5] == le && ox[6] == le)
		return 1;
	if (ox[7] == le && ox[8] == le && ox[9] == le)
		return 1;
	if (ox[1] == le && ox[4] == le && ox[7] == le)
		return 1;
	if (ox[2] == le && ox[5] == le && ox[8] == le)
		return 1;
	if (ox[3] == le && ox[6] == le && ox[9] == le)
		return 1;
	if (ox[1] == le && ox[5] == le && ox[9] == le)
		return 1;
	if (ox[3] == le && ox[5] == le && ox[7] == le)
		return 1;
	return 0;
}

int isfair(){
	for (int i = 1; i <= 9; i++){
		if (ox[i] == ' ')
			return 0;
	}
	return 1;
}

void game(){
	char buf[100];
	printf("\t#1\t|\t#2\t|\t#3\t\n");
	printf("---------------------\n");
	printf("\t#4\t|\t#5\t|\t#6\t\n");
	printf("------------------------------\n");
	printf("\t#7\t|\t#8\t|\t#9\t\n");

for(int i = 1; i <= 9; i++){
	ox[i] = ' ';
}

	print_table();
	if(turned_me){
		printf("輪到你攻\n");
	}else{
		printf("對手先攻\n");
	}
	char decision[10];
	while(fgets(decision, 10, stdin)){
		if(!game_start){
			printf("遊戲結束！\n");
			break;
		}
		if(!turned_me){
			printf("還沒輪到你！\n");
			continue;
		}

		int position = atoi(&decision[1]);

		if(position < 1 || position > 9){
			printf("請重新輸入！\n");
			continue;
		}
		if(ox[position] != ' '){
			printf("已存在\n");
			continue;
		}
		ox[position] = 'O';
		print_table();
		if(iswin('O')){
			printf("你贏了！！！\n");
			game_start = 0;
		}else if (isfair()){
			printf("平手！\n");
			game_start = 0;
		}

		sprintf(buf, "#%d %d", position, another_player_fd);
		send(socket_fd, buf, strlen(buf), 0);
		turned_me = 0;
	}
	
}

void start_game(){
	char decision[10];
	char buf[100] = {0};
	while(1){
		printf("\n---\t\t遊戲空間\t\t---\n");
		printf("\n(1) 列出目前上線使用者\n(2) 回上一層\n\n");
		// printf("\n(1) 創建房間\n(2) 進入已存在房間\n\n");
		printf("輸入您要進入的選項：\n");

		fgets(decision, 10, stdin);

		// printf("decision:%c\n", decision);

		if(decision[0] == '1'){
			// printf("送出 list\n");
			send(socket_fd, "GetAllOnlineUser", strlen("GetAllOnlineUser"), 0);

			fgets(decision, 10, stdin);

			if(strncmp(decision, "exit", 4) == 0){
				continue;
			}
			memset(buf, 0, sizeof(buf));
			// sprintf(buf, "game:invite:%d", atoi(decision));
			
			another_player_fd = atoi(&decision[1]);
			if(decision[strlen(decision)-1] == '\n'){
				decision[strlen(decision)-1] = 0;
			}

			send(socket_fd, decision, strlen(decision), 0);
		}else if(decision[0] == '2'){
			return;
		}
		else if (strncmp(decision, "yes", 3) == 0){
			if(!be_invited){
				printf("您沒有被邀請\n");
				continue;
			}	
			sprintf(buf, "agree %d", another_player_fd);
			send(socket_fd, buf, strlen(buf), 0);
			turned_me = 0;
			game_start = 1;
			game();
		}

		if(game_start){
			turned_me = 1;
			game();
		}
			
		// else if (decision[0] == '#'){
		// 	int position = atoi(&decision[1]);

			
		// }
	}	
}

void chat(){
	send(socket_fd, "join_chat", strlen("join_chat"), 0);

	printf("---\t已進入聊天室\t---\n");
	printf("---\t此為公共頻道\t---\n");
	printf("---\t輸入 exit 離開\t---\n");

	char buf[200], buf2[200];

	while(fgets(buf, 200, stdin)){
		if(strncmp(buf, "exit", 4) == 0){
			send(socket_fd, "exit_chat", strlen("exit_chat"), 0);
			return;
		}
			

		if(buf[strlen(buf)-1] == '\n'){
			buf[strlen(buf)-1] = 0;
		}
		
		sprintf(buf2, "chat:%s", buf);
		send(socket_fd, buf2, strlen(buf2), 0);
	}

}

void watch_game(){
	// 使用者可以知道誰在對戰 = 雙方合意後，要塞到 array 裡
	// 當有進來 watch game，變列出

	char decision[10];
	char buf[100];
	send(socket_fd, "watch_game", strlen("watch_game"), 0);

	printf("exit 離開\n");
	fgets(decision, 10, stdin);

	if(strncmp(decision, "exit", 4) == 0){
		return;
	}
	sprintf(buf, "watcher:%d", atoi(decision));
	send(socket_fd, buf, strlen(buf), 0);
}

void account_info(){
	printf("---\tOX 棋遊戲\t---\n");
	printf("---\t\t帳戶資訊\t\t---\n");
	printf("\n(1) 修改密碼\n(2) 修改名稱\n\n");

	char decision[10];
	while(1){
		fgets(decision, 10, stdin);

		if(decision[0] == '1'){
			char password[100];
			char new_password[100];
			printf("請輸入原密碼：");
			fgets(password, 100, stdin);
			printf("請輸入新密碼：");
			fgets(new_password, 100, stdin);

			char buf[200];
			sprintf(buf, "pw:%s:%s", password, new_password);
			send(socket_fd, buf, strlen(buf), 0);
		}else if (decision[0] == '2'){
			char name[100];
			char buf[200];
			printf("請輸入新名稱：");
			fgets(name, 100, stdin);
			if(name[strlen(name)-1] == '\n'){
				name[strlen(name)-1] = 0;
			}
			sprintf(buf, "change_name:%s", name);
			send(socket_fd, buf, strlen(buf), 0);
		}
	}
}

void menu_home(){
	while(1){
		char decision[10];
		printf("---\tOX 棋遊戲\t---\n");
		printf("---\t\t首頁\t\t---\n");
		printf("\n(1) 開始遊戲 (進入才能被邀請)\n(2) 公共頻道 (聊天)\n(3) 觀戰\n(4) 帳戶資訊\n(5) 登出\n\n");
		printf("輸入您要進入的選項：");
		
		fgets(decision, 10, stdin);

		switch (decision[0]){
			case '1':
				start_game();
				break;
			case '2':
				chat();
				break;
			case '3':
				watch_game();
				break;
			case '4':
				account_info();
				break;
			case '5':
				logout();
				break;
			default:
				printf("\n輸入選項錯誤，請重新輸入\n");
				break;
		}
		printf("\n");
		
	}
}

void *recv_thread(){
	char buf[100];
	char decision[10];
	while(1){
		memset(buf, 0, sizeof(buf));
		if (recv(socket_fd, buf, sizeof(buf), 0) <= 0){
			return NULL;
		}

		// printf("收到:%s\n", buf);

		if(strcmp("GetAllOnlineUser", buf) == 0){
			send(socket_fd, "OK", strlen("OK"), 0);
			memset(buf, 0, sizeof(buf));
			recv(socket_fd, buf, sizeof(buf), 0);
			printf("%s\n", buf);
			printf("請輸入要邀請的對象 @(ID)，或輸入 exit 離開：\n");
		}else if(strcmp("user_not_found", buf) == 0){
			printf("找不到人，有可能他不再線上了ＱＱ\n");
		}else if(strncmp("game:be_invited", buf, 15) == 0){
			char decision[10];
			// 才能使用 yes 
			be_invited = 1;

			printf("不好意思打擾您，有經由 ");
			for(int i = 16; i < strlen(buf); i++){
				printf("%c", buf[i]);
			}
			printf(" 發送的遊戲請求，請回覆 [yes/no]：\n");
			another_player_fd = atoi(&buf[20]);
		}else if (strcmp(buf, "game_start") == 0){
			printf("對方已回應，請按 enter 開始遊戲\n");
			game_start = 1;
		}else if (buf[0] == '#'){
			ox[atoi(&buf[1])] = 'X';
			if(iswin('X')){
				printf("輸了Ｑ！下次加油\n");
				game_start = 0;
			}else if (isfair()){
				printf("平手！\n");
				game_start = 0;
			}
			turned_me = 1;
			print_table();
		}else if (strncmp(buf, "watch", 5) == 0){
			if(watch){
				ox[atoi(&buf[7])] = 'O';
			}else{
				ox[atoi(&buf[7])] = 'X';
			}
			watch = 1;
			printf("-------------\n");
			print_table();
		}
		else{
			printf("%s\n", buf);
		}
		

	}
}

void init()
{
	socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (connect(socket_fd, (SA *)&addr, sizeof(addr)) == -1)
	{
		perror("無法連接");
		exit(-1);
	}
	printf("[INFO] Client start Successfully !\n");
}

void begin(){
	char buf[100] = {0};
	char acc[100], pwd[100], tmp;
	memset(buf, 0, sizeof(buf));
	recv(socket_fd, buf, sizeof(buf), 0);
	if(strcmp("init_success", buf) == 0){
		printf("[INFO] 初始化完成\n");
		}else{
		printf("[INFO] 初始化失敗\n");
		exit(-1);
	}

	while(1){
		printf("---\tOX 棋遊戲\t---\n");
		printf("登入後才可進入系統\t (如需註冊請輸入 new)\n");
		printf("帳號：");
		scanf("%s", acc);
		printf("密碼：");
		scanf(" %s", pwd);
		scanf("%c", &tmp);
		printf("\n");
		// TODO: send 到 server，由 server 確認 file 
		send(socket_fd, "account_check", strlen("account_check"), 0);
		memset(buf, 0, sizeof(buf));
		recv(socket_fd, buf, sizeof(buf), 0);

		if(strcmp(buf, "start_account_check") == 0){
			memset(buf, 0, sizeof(buf));
			strcat(buf, acc);
			strcat(buf, ":");
			strcat(buf, pwd);
			send(socket_fd, buf, strlen(buf), 0);
			memset(buf, 0, sizeof(buf));
			recv(socket_fd, buf, sizeof(buf), 0);

			if(strcmp("login_successful", buf) == 0){
				printf("登入成功！\n");
					pthread_t tid;
					void *recv_thread(void *);
					pthread_create(&tid, 0, recv_thread, 0);
				break;
			}else{
				printf("登入失敗！有可能是帳號已經在其他地方登入了！\n");
			}
		}

	}
	// 進入選單
	menu_home();
}
void logout(){
	// 需要給 server 處理 fds 
	send(socket_fd, "logout", strlen("logout"), 0);
	init();
	begin();
}
int main(){
	init();
	begin();
	return 0;
}