#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>  //ioctl() and TIOCGWINSZ
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>  // for STDOUT_FILENO
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#define HOST "127.0.0.1"
#define PORT 8711
#define BUFFER_SIZE 1024
#define MSG_BOARD_NAME "Chat Room"

const std::string ADMIN_NAME = "ADMIN";
const std::string SERVER_CLOSE_MSG = "ADMIN|@|Server was closed.";
std::string USER_NAME = "";
std::string last_msg = "";
int msg_offset = 0;

struct msg_data {
    std::string name;
    std::string msg;
};

std::vector<msg_data> msg_d;
std::vector<std::string> msg_list;

std::string c_to_string(char* a, int size) {
    int i;
    std::string s = "";
    for (i = 0; i < size; i++) {
        if (a[i] == '\n')
            break;
        s = s + a[i];
    }
    return s;
}

void init_msg_board() {
    msg_offset = 0;
    msg_d.clear();
    msg_list.clear();
}
// <name>|@|<msg>
void new_msg(std::string msg_str) {
    std::string delimeter = "|@|", msg_board_name = MSG_BOARD_NAME;
    msg_data temp_cd;
    int pos;
    if ((pos = msg_str.find(delimeter)) == std::string::npos) {
        std::cout << msg_str << std::endl
                  << std::flush;
        return;
    }

    temp_cd.name = msg_str.substr(0, pos);
    temp_cd.msg = msg_str.substr(pos + delimeter.length());
    msg_d.push_back(temp_cd);
}

void render_console() {
    if (USER_NAME == "") {
        init_msg_board();
        return;
    }
    std::string msg_board_name = MSG_BOARD_NAME;

    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int win_width = size.ws_col, win_height = size.ws_row;
    msg_list.clear();

    for (auto el : msg_d) {
        int name_len = el.name.length(), msg_len = el.msg.length();
        std::string temp_str = "";
        if (el.name == ADMIN_NAME) {
            int left_space = (win_width - 4 - msg_len) / 2,
                right_space = win_width - 4 - msg_len - left_space;

            temp_str += " │";  // 2
            for (int i = 0; i < left_space; i++) temp_str += " ";
            temp_str += el.msg;
            for (int i = 0; i < right_space; i++) temp_str += " ";
            temp_str += "│";  // 1
            msg_list.push_back(temp_str);
            temp_str = "";
        } else if (el.name == USER_NAME) {
            int max_len = win_width - 10 - 2;
            int left_space = max_len - msg_len;

            temp_str += " │ ";
            temp_str += "  ";  // padding
            for (int i = 0; i < left_space; i++) temp_str += " ";
            temp_str += "╭";
            for (int i = 0; i < std::min(msg_len, max_len) + 2; i++) temp_str += "─";
            temp_str += "╮";
            temp_str += " │";
            msg_list.push_back(temp_str);
            temp_str = "";

            for (int i = 0; i <= msg_len; i += max_len) {
                temp_str += " │ ";
                temp_str += "  ";  // padding
                for (int i = 0; i < left_space; i++) temp_str += " ";
                temp_str += "│ " + el.msg.substr(i, max_len);
                if (left_space <= 0)
                    for (int j = 0; j < max_len - (msg_len - i); j++) temp_str += " ";
                temp_str += " │";
                temp_str += " │";
                msg_list.push_back(temp_str);
                temp_str = "";
            }

            temp_str += " │ ";
            temp_str += "  ";  // padding
            for (int i = 0; i < left_space; i++) temp_str += " ";
            temp_str += "╰";
            for (int i = 0; i < std::min(msg_len, max_len) + 2; i++) temp_str += "─";
            temp_str += "╯";
            temp_str += " │";
            msg_list.push_back(temp_str);
            temp_str = "";
        } else {
            int max_len = win_width - name_len - 12;
            int left_space = max_len - msg_len;

            temp_str += " │ ";
            for (int i = 0; i < name_len + 2; i++) temp_str += " ";  // won't handle length oveflow from name
            temp_str += "╭";
            for (int i = 0; i < std::min(msg_len, max_len) + 2; i++) temp_str += "─";
            temp_str += "╮";
            for (int i = 0; i < left_space; i++) temp_str += " ";
            temp_str += " │";
            msg_list.push_back(temp_str);
            temp_str = "";

            for (int i = 0, cnt = 0; i <= msg_len; i += max_len, cnt++) {
                temp_str += " │ ";
                if (cnt == 0) {
                    temp_str += el.name;
                    temp_str += ":";
                } else {
                    for (int j = 0; j < name_len + 1; j++) temp_str += " ";
                }
                temp_str += " │ ";
                temp_str += el.msg.substr(i, max_len);
                if (left_space <= 0)
                    for (int j = 0; j < max_len - (msg_len - i); j++) temp_str += " ";
                temp_str += " │";
                for (int i = 0; i < left_space; i++) temp_str += " ";
                temp_str += " │";
                msg_list.push_back(temp_str);
                temp_str = "";
            }

            temp_str += " │ ";
            for (int i = 0; i < name_len + 2; i++) temp_str += " ";
            temp_str += "╰";
            for (int i = 0; i < std::min(msg_len, max_len) + 2; i++) temp_str += "─";
            temp_str += "╯";
            for (int i = 0; i < left_space; i++) temp_str += " ";
            temp_str += " │";
            msg_list.push_back(temp_str);
            temp_str = "";
        }
    }

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    std::cout << " ┌";
    for (int i = 0; i < win_width - 4; i++) std::cout << "─";
    std::cout << "┐ \n";

    std::cout << " │ ";
    std::cout << msg_board_name;
    for (int i = 0; i < win_width - msg_board_name.length() - 6; i++) std::cout << " ";
    std::cout << " │ \n";

    std::cout << " ├";
    for (int i = 0; i < win_width - 4; i++) std::cout << "─";
    std::cout << "┤ \n";

    int line_capacity = win_height - 5;
    for (int i = 0; i < (int)(line_capacity - msg_list.size()); i++) {
        std::cout << " │";
        for (int j = 0; j < win_width - 4; j++) std::cout << " ";
        std::cout << "│ \n";
    }

    int start_pos = (int)(msg_list.size() - line_capacity - msg_offset),
        end_pos = (int)(msg_list.size() - msg_offset);
    if (start_pos < 0 && msg_offset != 0) {
        msg_offset--;
        start_pos++;
        end_pos++;
    }
    if (msg_offset < 0) msg_offset = 0;

    for (int i = (int)(msg_list.size() - line_capacity - msg_offset); i < (int)(msg_list.size() - msg_offset); i++) {
        if (i < 0)
            continue;
        std::cout << msg_list[i] << std::endl;
    }

    std::cout << " └";
    for (int i = 0; i < win_width - 4; i++) std::cout << "─";
    std::cout << "┘ \n";

    std::cout << " " << USER_NAME << " > " << std::flush;
}

int main() {
    int sock_cli = socket(AF_INET, SOCK_STREAM, 0);
    fd_set rfds;
    int max_fd;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(HOST);

    if (connect(sock_cli, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }

    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        max_fd = 0;
        FD_SET(sock_cli, &rfds);

        if (max_fd < sock_cli) {
            max_fd = sock_cli;
        }

        int activity = select(max_fd + 1, &rfds, NULL, NULL, NULL);

        switch (activity) {
            case -1:
                std::cout << "ERROR\n";
                exit(1);
                break;

            case 0:
                continue;

            default:
                /* Receive message from server */
                if (FD_ISSET(sock_cli, &rfds)) {
                    char recv_buf[BUFFER_SIZE];
                    memset(recv_buf, 0, sizeof(recv_buf));
                    int rec_val = recv(sock_cli, recv_buf, sizeof(recv_buf), 0);
                    if (rec_val == 0) {
                        new_msg(SERVER_CLOSE_MSG);
                        render_console();
                        close(sock_cli);
                        return 0;
                    }
                    std::string recv_str = c_to_string(recv_buf, sizeof(recv_buf) / sizeof(char));
                    new_msg(recv_str);
                    render_console();
                }

                /* Send message to server */
                if (FD_ISSET(0, &rfds)) {
                    char send_buf[BUFFER_SIZE];
                    fgets(send_buf, sizeof(send_buf), stdin);

                    std::string send_str = c_to_string(send_buf, sizeof(send_buf) / sizeof(char));
                    if (send_str == "") {
                        if (last_msg[0] == '/')
                            send_str = last_msg;
                        else {
                            render_console();
                            break;
                        }
                    }

                    if (send_str[0] == '/') {  // Command
                        switch (send_str[1]) {
                            case 'n':  // 向下滾動
                                msg_offset--;
                                break;
                            case 'N':  // 向上滾動
                                msg_offset++;
                                break;
                            case 'c':  // clear screen
                                init_msg_board();
                                break;
                            case 'e':
                                return 0;
                            default:
                                break;
                        }
                        render_console();
                    } else {
                        if (USER_NAME == "") {
                            USER_NAME = send_str;
                        }
                        send(sock_cli, send_buf, strlen(send_buf), 0);
                        memset(send_buf, 0, sizeof(sendbuf));
                    }
                    last_msg = send_str;
                }
                break;
        }
    }
    close(sock_cli);
    return 0;
}
