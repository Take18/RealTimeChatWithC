/*
 * server.c
 *   クライアントからの接続要求を受け付けるサーバープログラム。
 *
 *   クライアントから送られてきた文字列を大文字に変換して送り返す。
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>

#define PORT 8080

typedef struct {
	int id;
	char name[32];
} User;

typedef struct {
	int id;
	int user;
	char content[128];
} Message;

void http( int fd, User *users, Message *msgs, int *user_num, int *msg_num );
int sendMessage( int fd, char *msg );
int getStatus( char *str, char *ret, char *file_type );
int sendFileContent( int fd, char *file_path );
int getQueryString( char *request, char **query );
void postAction( char *dir, char *parameter, char *return_json, User *users, Message *msgs, int *user_num, int *msg_num );
void createMessage( int fd, char *parameter, User *users, Message *msgs, int user_num, int msg_num );

int main() {
	int    i;
	int    fd1, fd2;
	struct sockaddr_in    saddr;
	struct sockaddr_in    caddr;
	int    len;
	int    ret;
	char   buf[1024];
	char tmp[1024];
	int buf_len;
	int time = 0;
	User users[10];
	Message msgs[100];
	int user_num = 0;
	int msg_num = 0;


	/*
	 *  ストリーム型ソケット作る．
	 */
	if ((fd1 = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	  perror("socket");
	  exit(1);
	}

	/*
	 * saddrの中身を0にしておかないと、bind()でエラーが起こることがある
	 */
	bzero((char *)&saddr, sizeof(saddr));

	/*
	 * ソケットの名前を入れておく
	 */
	saddr.sin_family      = PF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port        = htons(PORT);

	/*
	 * ソケットにアドレスをバインドする。
	 */
	if (bind(fd1, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){
	  perror("bind");
	  exit(1);
	}

	/*
	 * listenをソケットに対して発行する
	 */
	if (listen(fd1, 1) < 0) {
	  perror("listen");
	  exit(1);
	}

	len = sizeof(caddr);
	while(1) {
		/*
		* accept()により、クライアントからの接続要求を受け付ける。
		* 成功すると、クライアントと接続されたソケットのディスクリプタが
		* fd2に返される。このfd2を通して通信が可能となる。
		* fd1は必要なくなるので、close()で閉じる。
		*/
		if ((fd2 = accept(fd1, (struct sockaddr *)&caddr, &len)) < 0) {
			perror("accept");
			exit(1);
		} else {
			http(fd2, users, msgs, &user_num, &msg_num);
			close(fd2);
		}
	}
	close(fd1);
}

void http( int fd, User *users, Message *msgs, int *user_num, int *msg_num ) {
	int len;
	int status_code;
	char buf[1024] = "";
	char file_path[1024];
	char file_type[1024];
	char method_name[16];
	char dir[256];
	char http_ver[64];
	char *query_string;
	char return_json[1024] = "";


	printf("HTTP connected.\n");
	if ( read(fd, buf, 1024) <= 0 ) {
		perror("reading");
	} else {
		printf("Scanning...\n");
		printf("%s\n", buf );
		sscanf(buf, "%s %s %s", method_name, dir, http_ver);
		printf("HTTP_VERSION : %s\n", http_ver);
		if ( strcmp(method_name, "GET") == 0 ) {
			printf("GET method\n");
			status_code = getStatus(dir, file_path, file_type);
			if ( status_code == 200 ) {
				sendMessage(fd, "HTTP/1.0 200 OK\r\n");
				sendMessage(fd, file_type);
				sendMessage(fd, "\r\n");
				printf("Received: %s, ResponseFile: %s\n", dir, file_path);
				if ( strcmp(file_type, "application/json\r\n")==0 ) {
					createMessage(fd, file_path, users, msgs, *user_num, *msg_num);
				} else {
					sendFileContent(fd, file_path);
				}
			} else {
				sendMessage(fd, "HTTP/1.0 404 Not Found\r\n");
				sendMessage(fd, "text/html\r\n");
				sendMessage(fd, "\r\n");
				printf("Not Found: %s\n", dir);
			}
		} else if ( strcmp(method_name, "POST") == 0 ) {
			getQueryString(buf, &query_string);
			postAction(dir, query_string, return_json, users, msgs, user_num, msg_num);
			printf("POST method\n");
			printf("QUERY PARAMETER : %s\n", query_string);
			sendMessage(fd, "HTTP/1.0 200 OK\r\n");
			sendMessage(fd, "application/json\r\n");
			sendMessage(fd, "\r\n");
			sendMessage(fd, return_json);
		} else {
			sendMessage(fd, "501 Not Implemented");
		}
	}
}

int sendMessage( int fd, char *msg ) {
	int len = strlen(msg);
	if ( write(fd, msg, len) != len ) {
		perror("writing");
	}
	return len;
}

int getStatus( char *dir, char *file_path, char *file_type ) {
	if ( (strcmp(dir, "/index")==0) || (strcmp(dir, "/")==0) ) {
		strcpy(file_path, "./index.html");
		strcpy(file_type, "text/html\r\n");
	} else if (strcmp(dir, "/script.js")==0) {
		strcpy(file_path, "./script.js");
		strcpy(file_type, "text/javascript\r\n");
	} else if (strcmp(dir, "/style.css")==0) {
		strcpy(file_path, "./style.css");
		strcpy(file_type, "text/css\r\n");
	} else if (strncmp(dir, "/receive", 8)==0) {
		strcpy(file_path, &(dir[9]));
		strcpy(file_type, "application/json\r\n");
		return 200;
	} else {
		return 404;
	}
	return 200;
}

int sendFileContent( int fd, char *file_path ) {
	int len;
	char line[1024] = "";
	FILE *fp;

	fp = fopen(file_path, "r");
	if ( fp == NULL ) {
		perror("Not Found");
		return -1;
	}
	printf("File Opened.\n");
	while( fgets(line, 1024, fp) != NULL ) {
		printf("Line Read.\n");
		sendMessage(fd, line);
	}
	fclose(fp);
}

int getQueryString( char *request, char **query ) {
	char *tmp_p = request;
	while((*tmp_p!='\r') || (*(tmp_p+1)!='\n') || (*(tmp_p+2)!='\r') || (*(tmp_p+3)!='\n')) {
		tmp_p++;
		if ( *tmp_p == '\0' ) {
			printf("Not Exist Query Parameter.\n");
			return -1;
		}
	}
	*query = tmp_p + 4;
}

void postAction( char *dir, char *parameter, char *return_json, User *users, Message *msgs, int *user_num, int *msg_num ) {
	int i = 0;
	int j = 0;
	char name[32];
	char id_str[3];
	char content[128];

	printf("DIR : %s\n", dir);
	printf("PARAMETER : %s\n", parameter);
	if ( strcmp(dir, "/user")==0 ) {
		while((parameter[i+5]!='\0')&&(parameter[i+5]!=';')&&(i<31)){
			name[i] = parameter[i+5];
			i++;
		}
		name[i] = '\0';
		(users+(*user_num))->id = (*user_num)+1;
		sprintf(id_str, "%d", (*user_num)+1);
		strcpy((users+(*user_num))->name, name);
		(*user_num)++;
		strcat(return_json, "{\"result\":\"success\",\"name\":\"");
		strcat(return_json, name);
		strcat(return_json, "\",\"id\":");
		strcat(return_json, id_str);
		strcat(return_json, "}");
		printf("Name : %s\n", name);
	} else if ( strcmp(dir, "/send")==0 ) {
		printf("%s\n", parameter);
		while(parameter[i+5]!='&'){
			id_str[i] = parameter[i+5];
			i++;
		}
		id_str[i] = '\0';
		j = i + 14;
		i = 0;
		while((parameter[i+j]!='\0')&&(i<127)){
			content[i] = parameter[i+j];
			i++;
		}
		content[i] = '\0';
		printf("ID : %s\n", id_str);
		printf("CONTENT : %s\n", content);
		(msgs+(*msg_num))->id = (*msg_num)+1;
		(msgs+(*msg_num))->user = atoi(id_str);
		strcpy((msgs+(*msg_num))->content, content);
		(*msg_num)++;
		strcat(return_json, "{\"result\":\"success\"}");
	}
}

void createMessage( int fd, char *parameter, User *users, Message *msgs, int user_num, int msg_num ) {
	int current;
	char return_json[1024] = "";
	int i, j;
	int id;
	int user;
	char content[128];
	char name[32];
	char user_id_str[3];
	char msg_num_str[4];

	sprintf(msg_num_str, "%d", msg_num);
	sscanf( parameter, "current=%d", &current );
	strcat(return_json, "{\"result\":\"success\",\"last_msg\":");
	strcat(return_json, msg_num_str);
	strcat(return_json, ",\"messages\":[");
	printf("CURRENT : %d\n", current);
	for ( i=current; i<msg_num; i++ ) {
		id = (msgs+i)->id;
		user = (msgs+i)->user;
		sprintf(user_id_str, "%d", user);
		strcpy(content, (msgs+i)->content);
		strcpy(name, "名無しさん");
		for ( j=0; j<user_num; j++ ) {
			if ( (users+j)->id == user ) {
				strcpy(name, (users+j)->name);
				break;
			}
		}
		if ( i != current ) {
			strcat(return_json, ",");
		}
		strcat(return_json, "{\"user_id\":");
		strcat(return_json, user_id_str);
		strcat(return_json, ",\"user_name\":\"");
		strcat(return_json, name);
		strcat(return_json, "\",\"content\":\"");
		strcat(return_json, content);
		strcat(return_json, "\"}");
	}
	strcat(return_json, "]}");
	sendMessage(fd, return_json);
}
