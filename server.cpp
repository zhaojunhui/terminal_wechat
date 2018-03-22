#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sys/stat.h> 

using namespace std;

#define SERVERPORT 3333
#define MAXDATASIZE 4096
#define SUPERADMIN "xzwkl"

bool serverRunning = false;
bool admin = false;
char sendBuffer[MAXDATASIZE];
char recvBuffer[MAXDATASIZE];

struct User {
	string username;
	string password;
	int numFriend;
	vector<string> friendList;
	int numMessage;
	vector<string> messageList;
	bool admin;
	User() {
		numFriend = 0;
		numMessage = 0;
		admin = false;
	}
	User(string un, string pw) {
		username = un;
		password = pw;
		numFriend = 0;
		numMessage = 0;
		admin = false;
	}
};

vector<User> adminList;
vector<string> adminName;


//vector<User> userList;
map<string, int> mapToSockFD;
map<string, User> mapToUser;
int userNum = 1;

void LoadUserInfo() {
	ifstream fin;
	fin.open("init.txt");
	if(fin.is_open()) {
		fin >> userNum;
		int tmp_num = userNum;
		while(tmp_num > 0) {
			User tmpUser;
			fin >> tmpUser.username; fin >> tmpUser.password;
			fin >> tmpUser.numFriend;
			int n;
			bool admin;
			string tmp;
			n = tmpUser.numFriend;
			while(n > 0) {
				fin >> tmp;
				tmpUser.friendList.push_back(tmp);
				n--;
			}
			fin >> tmpUser.numMessage;
			n = tmpUser.numMessage;
			while(n > 0) {
				fin >> tmp;
				tmpUser.messageList.push_back(tmp);
				n--;
			}
			fin >> tmpUser.admin;
			if(tmpUser.admin)
				adminList.push_back(tmpUser);
			//userList.push_back(tmpUser);
			mapToUser.insert(pair<string, User>(tmpUser.username, tmpUser));
			tmp_num--;
		}
		fin.close();
	}
	else {
		perror("fail to open file!");
		exit(1);
	}
}

void SaveUserInfo() {
	ofstream fout;
	fout.open("init.txt");
	if(fout.is_open()) {
		fout << userNum << endl;
		for(map<string, User>::iterator it = mapToUser.begin(); it != mapToUser.end(); ++it) {
			fout << it->second.username << endl;
			fout << it->second.password << endl;
			fout << it->second.numFriend << endl;
			for(vector<string>::iterator tmp_it = it->second.friendList.begin(); tmp_it != it->second.friendList.end(); ++tmp_it) {
				fout << *tmp_it << endl;
			}
			fout << it->second.numMessage << endl;
			for(vector<string>::iterator tmp_it = it->second.messageList.begin(); tmp_it != it->second.messageList.end(); ++tmp_it) {
				fout << *tmp_it << endl;
			}
			fout << it->second.admin << endl;
		}
		fout.close();
	}
	else {
		perror("fail to open file!");
		exit(1);
	}
}

int main() {

	//superadmin
	User superadmin = User("xzwkl", "123456");
	superadmin.admin = true;
	adminName.push_back("xzwkl");
	adminList.push_back(superadmin);
	//userList.push_back(superadmin);
	mapToUser.insert(pair<string, User>(superadmin.username, superadmin));

	//printf("ahahha\n");
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SERVERPORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(my_addr.sin_zero), sizeof(my_addr.sin_zero));

	int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); 

	//printf("ahahha\n");
	if(bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		perror("fail to bind");
		exit(1);
	}
	//printf("hahah\n");
	if(listen(sockfd, 1000) == -1) {
		perror("fail to listen");
		exit(1);
	};
	//printf("hah\n");
	serverRunning = true;
	printf("now weChat server is running...\n");

	LoadUserInfo();
	//printf("1\n");
	socklen_t sin_size = sizeof(struct sockaddr_in);
	int newfd;
	int num;
	bool at_login = false;

	struct sockaddr_in client_addr;
	while(serverRunning) {
		//printf("1.5\n");
		//cout << at_login << endl;
		int tmp_newfd;
		//check whether have new user
		if((tmp_newfd = accept(sockfd, (struct sockaddr *) NULL, NULL)) == -1) {
			//perror("fail to accept");
			//printf("1");
			//continue;
			if(at_login) {
				printf("1\n");
				num = recv(newfd, recvBuffer, MAXDATASIZE, 0);

				//check admin
				string tmp_admin = recvBuffer;
				cout << tmp_admin << endl;
				//printf("%d\n", tmp_admin.length());
				if(find(adminName.begin(), adminName.end(), tmp_admin) != adminName.end()) {
					admin = true;
					printf("welcome, you administor!\n");
				} else {
					admin = false;
				}

				//username
				string tmp_user = recvBuffer;
				string tmp_pw;
				bool old_user;
				User user = User();
				if(mapToUser.find(tmp_user) != mapToUser.end()) {
					old_user = true;
					user = mapToUser[tmp_user];
					tmp_pw = user.password;
				} else {
					old_user = false;
					user.username = tmp_user;
					printf("new user, already create account for you~\n");
				}
				sendBuffer[0] = '1';
				sendBuffer[1] = '\0';
				if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
					perror("fail to send");
					continue;
				}

				//password
				num = recv(newfd, recvBuffer, MAXDATASIZE, 0);
				string input_pw = recvBuffer;
				if(old_user && input_pw.compare(tmp_pw) == 0) {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					at_login = false;
					if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						continue;
					}
					printf("successfully log in!\n");
				} else if(old_user && input_pw.compare(tmp_pw) != 0) {
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					at_login = true;
					if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						continue;
					}
					printf("fail to log in\n");
				} else {
					user.password = input_pw;
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					at_login = false;
					if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						continue;
					}
					printf("new password set\n");
				//printf("%s", recvBuffer);
				}
				//printf("3\n");
			}
		}
		//printf("2\n");
		else {
			newfd = tmp_newfd;
			printf("accepted: get connection from %s\n", inet_ntoa(client_addr.sin_addr));
			num = recv(newfd, recvBuffer, MAXDATASIZE, 0);

			//check admin
			string tmp_admin = recvBuffer;
			cout << tmp_admin << endl;
			printf("%d\n", tmp_admin.length());
			if(find(adminName.begin(), adminName.end(), tmp_admin) != adminName.end()) {
				admin = true;
				printf("welcome, you administor!\n");
			} else {
				admin = false;
			}

			//username
			string tmp_user = recvBuffer;
			string tmp_pw;
			bool old_user;
			User user = User();
			if(mapToUser.find(tmp_user) != mapToUser.end()) {
				old_user = true;
				user = mapToUser[tmp_user];
				tmp_pw = user.password;
				//printf("now get pw\n");
				cout << tmp_pw << endl;
				mapToSockFD.insert(pair<string, int>(tmp_user, newfd));
			} else {
				old_user = false;
				mapToSockFD.insert(pair<string, int>(tmp_user, newfd));
				user.username = tmp_user;
				printf("new user, already create account for you~\n");
			}
			sendBuffer[0] = '1';
			sendBuffer[1] = '\0';
			if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
				perror("fail to send");
				continue;
			}

			//password
			num = recv(newfd, recvBuffer, MAXDATASIZE, 0);
			string input_pw = recvBuffer;
			cout << input_pw << endl;
			cout << tmp_pw << endl;
			if(old_user && input_pw.compare(tmp_pw) == 0) {
				sendBuffer[0] = '1';
				sendBuffer[1] = '\0';
				at_login = false;
				if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
					perror("fail to send");
					continue;
				}
				printf("successfully log in!\n");
			} else if(old_user && input_pw.compare(tmp_pw) != 0) {
				sendBuffer[0] = '0';
				sendBuffer[1] = '\0';
				at_login = true;
				if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
					perror("fail to send");
					continue;
				}
				printf("fail to log in\n");
			} else {
				//printf("now give pw\n");
				cout << input_pw << endl;
				user.password = input_pw;
				//userList.push_back(user);
				mapToUser.insert(pair<string, User>(tmp_user, user));
				userNum++;
				sendBuffer[0] = '1';
				sendBuffer[1] = '\0';
				at_login = false;
				if(send(newfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
					perror("fail to send");
					continue;
				}
				printf("new password set\n");
			//printf("%s", recvBuffer);
			}
			//printf("3\n");
			//close(newfd);
		}
		//printf("map %d\n", mapToSockFD.size());

		//listen on all existed sockets
		for(map<string, int>::iterator it = mapToSockFD.begin(); it != mapToSockFD.end(); ++it) {
			string username = it->first;
			int tmp_sockfd = it->second;
			User &user = mapToUser[username];
			int num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
			if(num == -1) continue;
			char tmp[MAXDATASIZE];
			memset(tmp, 0, sizeof(tmp));
			printf("rc %s\n", recvBuffer);
			if(strcmp(recvBuffer, "search") == 0) {
				for(map<string, User>::iterator it = mapToUser.begin(); it != mapToUser.end(); ++it) {
					string tmp_str = it->first;
					tmp_str.copy(sendBuffer, tmp_str.length(), 0);
					sendBuffer[tmp_str.length()] = '\n';
					//printf("send %s\n", sendBuffer);
					if(send(tmp_sockfd, sendBuffer, tmp_str.length() + 1, 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
				sendBuffer[0] = '\0';
				if(send(tmp_sockfd, sendBuffer, 1, 0) == -1) {
					perror("fail to send");
					exit(1);
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 7), "sendmsg") == 0) {
				//printf("hha\n");
				char tmp1[MAXDATASIZE];
				memset(tmp1, 0, sizeof(tmp1));
				char tmp2[MAXDATASIZE];
				memset(tmp2, 0, sizeof(tmp2));
				strcpy(tmp1, recvBuffer);
				char* sub1 = strtok(tmp1, " ");
				string opname = strtok(NULL, " ");
				strcpy(tmp2, recvBuffer);
				char* sub2 = strtok(tmp2, "\"");
				string words = strtok(NULL, "\"");
				if(mapToUser.find(opname) != mapToUser.end() && find(user.friendList.begin(), user.friendList.end(), opname) != user.friendList.end()) {
					//int opSockfd = mapToSockFD[opname];
					string finalwords = username + ": " + words;
					mapToUser[opname].messageList.push_back(finalwords);
					mapToUser[opname].numMessage++;
					//cout << finalwords << endl;
					//printf("fw%s\n", finalwords);
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				} else {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
			} else if(strcmp(recvBuffer, "recvmsg") == 0) {
				//printf("5\n");
				for(vector<string>::iterator it = user.messageList.begin(); it != user.messageList.end(); ++it) {
					(*it).copy(sendBuffer, it->length(), 0);
					sendBuffer[it->length()] = '\n';
					//printf("sb %s\n", sendBuffer);
					if(send(tmp_sockfd, sendBuffer, it->length() + 1, 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
				user.numMessage = 0;
				sendBuffer[0] = '\0';
				if(send(tmp_sockfd, sendBuffer, 1, 0) == -1) {
					perror("fail to send");
					exit(1);
				}
			} else if(strcmp(recvBuffer, "exit") == 0) {
				mapToSockFD.erase(username);
				close(tmp_sockfd);
			} else if(strcmp(recvBuffer,"exit_server") == 0) {
				if(user.admin) {
					serverRunning = false;
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
				else {
					serverRunning = true;
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}					
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 8), "sendfile") == 0) {
				struct timeval timeout100={100,0};
    			setsockopt(tmp_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout100, sizeof(timeout100));
				char tmp1[MAXDATASIZE];
				memset(tmp1, 0, sizeof(tmp1));
				strcpy(tmp1, recvBuffer);
				char* sub1 = strtok(tmp1, " ");
				string opname = strtok(NULL, " ");
				string dirname = "/home/parallels/Desktop/file/" + opname;
				if(mapToUser.find(opname) != mapToUser.end() && find(user.friendList.begin(), user.friendList.end(), opname) != user.friendList.end()) {
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
					int num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
					while(recvBuffer[0] == '1') {
						num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
						sendBuffer[0] = '1';
						sendBuffer[1] = '\0';
						if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
							perror("fail to send");
							exit(1);
						}
					}
					num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
					string orifilepath = recvBuffer;
					int pos = orifilepath.find_last_of("/");
					string filename = orifilepath.substr(pos + 1, orifilepath.length());
					string filepath = dirname + "/" + filename;
					cout << filename << endl;
					mkdir(dirname.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
					FILE *fp = fopen(filepath.c_str(), "w");
					while((num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0)) > 0) {
						printf("%s\n", recvBuffer);
						sendBuffer[0] = '1';
						sendBuffer[1] = '\0';
						if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
							perror("fail to send");
							exit(1);
						}
						if(recvBuffer[0] == '#' && recvBuffer[1] == '#') {
							break;
						} else {
							fwrite(recvBuffer, sizeof(char), num, fp);
						}
					}
					fclose(fp);
				} else {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}					
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 8), "recvfile") == 0) {
				//cout << "user " << username << endl;
				struct timeval timeout100={100,0};
    			setsockopt(tmp_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout100, sizeof(timeout100));
				string dirname = "/home/parallels/Desktop/file/" + username;
				char tmp1[MAXDATASIZE];
				memset(tmp1, 0, sizeof(tmp1));
				strcpy(tmp1, recvBuffer);
				char* sub1 = strtok(tmp1, " ");
				string filename = strtok(NULL, " ");
				string filepath = dirname + "/" + filename;
				//printf("666\n");
				cout << filepath << endl;
				FILE *fp = fopen(filepath.c_str(), "r");
				if(fp == NULL) {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}	
				} else {
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
					int num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
					while((num = fread(sendBuffer, sizeof(char), MAXDATASIZE, fp)) > 0) {
						if(send(tmp_sockfd, sendBuffer, num, 0) == -1) {
							perror("fail to send");
							exit(1);
						}						
						num = recv(tmp_sockfd, recvBuffer, MAXDATASIZE, 0);
					}
					sleep(1);
					sendBuffer[0] = '#';
					sendBuffer[1] = '#';
					sendBuffer[2] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}	
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 9), "add_admin") == 0) {
				char tmp1[MAXDATASIZE];
				memset(tmp1, 0, sizeof(tmp1));
				strcpy(tmp1, recvBuffer);
				char* sub1 = strtok(tmp1, " ");
				string tmp_name = strtok(NULL, " ");
				if(user.admin && mapToUser.find(tmp_name) != mapToUser.end()) {
					mapToUser[tmp_name].admin = true;
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				} else {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 10), "add_friend") == 0) {
				char tmp1[MAXDATASIZE];
				strcpy(tmp1, recvBuffer);
				char* sub1 = strtok(tmp1, " ");
				string friend_name = strtok(NULL, " ");
				cout << friend_name << endl;
				if(mapToUser.find(friend_name) != mapToUser.end() && find(user.friendList.begin(), user.friendList.end(), friend_name) == user.friendList.end()) {
					user.friendList.push_back(friend_name);
					user.numFriend++;
					sendBuffer[0] = '0';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				} else {
					sendBuffer[0] = '1';
					sendBuffer[1] = '\0';
					if(send(tmp_sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
						perror("fail to send");
						exit(1);
					}					
				}
			} else if(strcmp(strncpy(tmp, recvBuffer, 11), "show_friend") == 0) {
				for(vector<string>::iterator it = user.friendList.begin(); it != user.friendList.end(); ++it) {
					string tmp_str = *it;
					tmp_str.copy(sendBuffer, tmp_str.length(), 0);
					sendBuffer[tmp_str.length()] = '\n';
					//printf("send %s\n", sendBuffer);
					if(send(tmp_sockfd, sendBuffer, tmp_str.length() + 1, 0) == -1) {
						perror("fail to send");
						exit(1);
					}
				}
				sendBuffer[0] = '\0';
				if(send(tmp_sockfd, sendBuffer, 1, 0) == -1) {
					perror("fail to send");
					exit(1);
				}
			} else {
				printf("error!\n");
			}
		}
	}
	SaveUserInfo();
}