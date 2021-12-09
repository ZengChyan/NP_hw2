#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define online_size 100
#define PORT 5555

int online_client = 0;
int socket_fd;
int socket_fds[online_size] = {0};
int chat_room[online_size] = {0};
char game_room[online_size][100] = {0};
int watcher[online_size] = {0};
char online_account[online_size][100];
typedef struct sockaddr SA;

int auth(int client_fd, char *line){
	FILE *fp;
    char tmp[100];
    fp = fopen("passwd", "r");
    while (fgets(tmp, 100, fp)){
        if(strstr(tmp, line) != NULL){
			for(int i = 0; i < online_size; i++){
				if(socket_fds[i] == client_fd){
					char *find_name = strstr(tmp, ":");
					char name[100];
					strncpy(name, tmp, find_name - tmp);
					for(int j = 0; j < online_size; j++){
						if(strcmp(online_account[j], name) == 0){
							return 0;
						}
					}
					// 將使用者的名稱放到 online_account
					strcpy(online_account[client_fd], name);
				}
			}
			online_client++;
			fclose(fp);
			return 1;
		}
			
    }
	fclose(fp);
	return 0;
}

int check_online(int client_fd){
	for(int i = 0; i < online_size; i++){
		if(socket_fds[i] == client_fd){
			return 1;
		}
	}
	return 0;
}

void init(){
	socket_fd = socket(PF_INET, SOCK_STREAM, 0);

	if(socket_fd < 0){
		perror("Socket create failed！");
		exit(-1);
	}

	struct sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(socket_fd, (SA *)&addr, sizeof(addr)) < 0){
		perror("Bind failed！");
		exit(-1);
	}

	if(listen(socket_fd, 100) < 0){
		perror("Listen failed！");
		exit(-1);
	}
}

void *thread_job(void *client_fd_tmp){
	char buf[100];
	int client_fd = *(int *)client_fd_tmp;

	// 告知 client 目前 init 完畢，可以讓 client 進行帳號認證
		send(client_fd, "init_success", strlen("init_success"), 0);
	while(1){	
		// client close
		memset(buf, 0, sizeof(buf));
		if(recv(client_fd, buf, sizeof(buf), 0) <= 0){
			for(int i = 0; i < online_size; i++){	
				if(socket_fds[i] == client_fd){
					socket_fds[i] = 0;
					break;
				}
			}

			for(int i = 0; i < strlen(online_account[client_fd]); i++){
				online_account[client_fd][i] = 0;
			}

			for(int i = 0; i < online_size; i++){
				if(watcher[i] == client_fd){
					watcher[i] = 0;
					break;
				}
			}

			printf("User exit\n");
			online_client--;
			// pthread_exit((void *)client_fd);
			pthread_exit(0);
		}
		printf("%s\n", buf);
		// TODO: 公共頻道應該要有另一個 arr 維護
		
		// TODO: 收到使用者的帳密進行認證 (可以額外加 hash)
		if(strcmp("account_check", buf) == 0){
			send(client_fd, "start_account_check", strlen("start_account_check"), 0);
			memset(buf, 0, sizeof(buf));
			recv(client_fd, buf, sizeof(buf), 0);
			if(auth(client_fd, buf)){
				printf("有一個使用者登入成功\n");
				send(client_fd, "login_successful", strlen("login_successful"), 0);
			}else{
				printf("有一個使用者登入失敗\n");
				send(client_fd, "login_failed", strlen("login_failed"), 0);
			}
		}
		
		if(strcmp("GetAllOnlineUser", buf) == 0){
			send(client_fd, "GetAllOnlineUser", strlen("GetAllOnlineUser"), 0);
			memset(buf, 0, sizeof(buf));
			recv(client_fd, buf, sizeof(buf), 0);
			printf("%s\n", buf);
			if(strcmp(buf, "OK") == 0){
				for(int i = 0; i < online_size; i++){
					if(socket_fds[i] != 0){
						char buf2[100];
						sprintf(buf2, "ID: %d, %s\n", socket_fds[i], online_account[socket_fds[i]]);
						send(client_fd, buf2, strlen(buf2), 0);
					}
				}
			}
		}

		if(buf[0] == '@'){
			char buf2[100];
			int invite_id;
			for(int i = 1, j = 0; i < strlen(buf); i++, j++){
				buf2[j] = buf[i];
			}
			printf("invite_id:%d\n", atoi(buf2));
			invite_id = atoi(buf2);

			if(!check_online(invite_id)){
				printf("找不到人、ㄙ\n");
				send(client_fd, "user_not_found", strlen("user_not_found"), 0);
				continue;
			}
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "game:be_invited:ID: %d, %s", client_fd, online_account[client_fd]);
			send(invite_id, buf, strlen(buf), 0);
			printf("fucn\n");
			send(client_fd, "等待對方回應...", strlen("等待對方回應..."), 0);
		}


		if(strncmp(buf, "agree", 5) == 0){
			int another_player_fd = atoi(&buf[5]);

			send(another_player_fd, "game_start", strlen("game_start"), 0);
			send(client_fd, "大戰開始", strlen("大戰開始"), 0);
			for(int i = 0; i < online_size; i++){
				if(strlen(game_room[i]) == 0){
					sprintf(game_room[i], "ID %d, %s V.S ID %d, %s", client_fd, online_account[client_fd], another_player_fd, online_account[another_player_fd]);
					break;
				}
			}
		}

		if(buf[0] == '#'){
			sprintf(buf, "#%c", buf[1]);
			send(atoi(&buf[3]), buf, strlen(buf), 0);

			for(int i = 0; i < online_size; i++){
				if(watcher[i] != 0){
					sprintf(buf, "watch:#%c", buf[1]);
					send(watcher[i], buf, strlen(buf), 0);
				}
			}
		}

		if(strcmp(buf, "join_chat") == 0){
			for(int i = 0; i < online_size; i++){
				if(chat_room[i] == 0){
					chat_room[i] = client_fd;
					break;
				}
			}
		}

		if(strncmp(buf, "chat:", 5) == 0){
			char buf2[200];
			for(int i = 5, j = 0; i < strlen(buf); i++, j++)
				buf2[j] = buf[i];

			printf("buf2:%s\n", buf2);
			char buf3[200];
			sprintf(buf3, "[%s] %s", online_account[client_fd], buf2);
			for(int i = 0; i < online_size; i++){
				if(chat_room[i] != 0){
					send(chat_room[i], buf3, strlen(buf3), 0);
				}
			}
		}

		if(strcmp(buf, "exit_chat") == 0){
			for(int i = 0; i < online_size; i++){
				if(chat_room[i] == client_fd){
					chat_room[i] = 0;
					break;
				}
			}
		}
		if(strcmp(buf, "logout") == 0){
			for(int i = 0; i < online_size; i++){
				if(socket_fds[i] == client_fd){
					socket_fds[i] = 0;
					break;
				}
			}

			for(int i = 0; i < strlen(online_account[client_fd]); i++){
				online_account[client_fd][i] = 0;
			}

			for(int i = 0; i < online_size; i++){
				if(watcher[i] == client_fd){
					watcher[i] = 0;
					break;
				}
			}
			
			online_client--;
			pthread_exit(0);
		}

		if(strncmp(buf, "pw", 2) == 0){
			char password[100], new_password[100];

			char *ptr;
			ptr = strstr(&buf[4], ":");
			strncpy(password, &buf[3], ptr - &buf[4]);
			strncpy(new_password, ptr+1, buf+strlen(buf) - ptr);

			printf("ori_pw:%s\n", password);
			printf("new_pw:%s\n", new_password);

			char command[100];

			strcat(command, "./change_passwd ");
			strcat(command, password);
			strcat(command, " ");
			strcat(command, new_password);
			system(command);

			send(client_fd, "更換密碼完成！", strlen("更換密碼完成！"), 0);
		}

		if(strncmp(buf, "change_name", 11) == 0){
			char command[100];

			strcat(command, "./change_passwd ");
			strcat(command, online_account[client_fd]);
			strcat(command, " ");
			strcat(command, &buf[13]);
			system(command);
		}

		if(strcmp(buf, "watch_game") == 0){

			for(int i = 0; i < online_size; i++){
				if(strlen(game_room[i]) != 0){
					sprintf(buf, "編號 %d -> 對戰組合 %s", i, game_room[i]);
					send(client_fd, buf, strlen(buf), 0);
				}
			}
			for(int i = 0; i < online_size; i++){
				if(watcher[i] == 0){
					watcher[i] = client_fd;
					break;
				}
			}
		}


	}
}

int main(){
	
	// initial 
	init();

	while(1){
		printf("Server Start！\n");

		struct sockaddr_in addr;
		socklen_t addr_size = sizeof(addr);

		printf("等待使用者連線：...\n");

		int client_fd = accept(socket_fd, (SA *)&addr, &addr_size);

		if(client_fd < 0){
			perror("Client connect failed！");
			// client 連線有問題不應該把 server 關閉
			// exit(-1);
			continue;
		}

		for(int i = 0; i < online_size; i++){
			if(socket_fds[i] == 0){
				socket_fds[i] = client_fd;
				pthread_t tid;
				pthread_create(&tid, 0, thread_job, &client_fd);
				break;
			}
		}
	}

	return 0;
}