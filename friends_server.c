#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "friends.h"


#ifndef PORT
  #define PORT 50078
#endif

int main(void) {

    User *user_list = NULL;
    //initialize head of user linked list
    struct sockname *users = malloc(sizeof(struct sockname));
        users ->username[0] = '\0';
        users -> next = NULL;
        users -> sock_fd = -1;
        // initializing data for receiving messages
        users -> buf[0] = '\0'; 
        users ->inbuf = 0;
        users ->after = users ->buf;
        users ->room = sizeof(users ->buf);
        

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Setting information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    int on = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                            (const char *) &on, sizeof(on));
    if (status == -1) {
        perror("setsockopt -- REUSEADDR");
    }
    memset(&server.sin_zero, 0, 8);

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    // The client accept - message accept loop. 
    // Initializing a set of file descriptors.
    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    while (1) {
        fd_set listen_fds = all_fds; //listen_fds is a copy of all_fds
        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(1);
        }

        // Create new connection 
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, users);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection\r\n");
        }

        //Check the clients.
        struct sockname *curr = users;
        while(curr != NULL){
            if (FD_ISSET(curr ->sock_fd, &listen_fds)) {
                int client_closed = buffered_read(curr, users, &user_list); 
                if (client_closed > 0) {
                    FD_CLR(client_closed, &all_fds);
                    printf("Client %d disconnected\r\n", client_closed);
                } else {
                    printf("Echoing message from client %d\r\n", curr ->sock_fd);
                }   
            }
            curr = curr->next;
        }
    }

    // Should never get here.
    return 1;
}


/*
 * Searches the first n characters of buf for a network newline (\r\n).
 * Returns one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found. The return value is the index into buf
 * where the current line ends.
 */
int find_network_newline(const char *buf, int n) {
    for (int i = 0; i < n; i++){
        if ( buf[i] == '\r'){
            if ( i != n &&  buf[i + 1] == '\n'){
                return i + 2;
            }
        }
    }
    return -1;
}

/*
 * Finds file desrcriptor of user with the username name
 */
int find_fd(char *name, struct sockname *head){
    struct sockname *curr = head;
    while (curr!= NULL){
        if (strcmp(curr ->username, name) == 0){
            return curr ->sock_fd;
        }
        curr = curr->next;
    }
    return -1; //should never reach here 

}

/*
 * Read and process commands
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, struct sockname *head, int fd) { // send in sock struct head?
    User *user_list = *user_list_ptr;
    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 2) {
        return -1;
    } else if (strcmp(cmd_argv[0], "add_user") == 0 && cmd_argc == 2) {
        switch (create_user(cmd_argv[1], user_list_ptr, head)) {
            case 0:
                write(fd, "Welcome.\r\n", strlen("Welcome.\r\n")); //TODO: manage writing errors 
                break;
            case 1:
                write(fd, "Welcome back.\r\n", strlen("Welcome back.\r\n")); //TODO: manage writing errors 
                break;
            case 2: 
                write(fd, "Username too long, truncated to 31 chars.\r\n", strlen("Username too long, truncated to 31 chars.\r\n")); //TODO: manage writing errors 
                break;
        }

    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 2) { //TODO: change back to one if theory doesnt work
        char *buf = list_users(user_list);
        write(fd, buf, strlen(buf));
        free(buf);

    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 3) {
        int other_fd = find_fd(cmd_argv[1], head);
        int len = strlen("You are now friends with ") + strlen(cmd_argv[1]) + strlen("\r\n") + 1; //TODO: check if length is correct
        int other_len = strlen("You have been friended by ") + strlen(cmd_argv[2]) + strlen("\r\n") + 1;

        char message[len];
        message[0] = '\0';
        strcat(message, "You are now friends with " );
        strcat(message, cmd_argv[1]);
        strcat(message, "\r\n");

        char message_other[other_len];
        message_other[0] = '\0';
        strcat(message_other, "You have been friended by " );
        strcat(message_other, cmd_argv[2]);
        strcat(message_other, "\r\n");

        switch (make_friends(cmd_argv[1], cmd_argv[2], user_list)) {
            case 0:
                write(fd, message, strlen(message));
                write(other_fd, message_other, strlen(message_other));
                break;

            case 1:
                write(fd, "You are already friends.\r\n", strlen("You are already friends.\r\n"));
                break;
            case 2: 
                write(fd, "At least one of you entered has the max number of friends\r\n", strlen("At least one of you entered has the max number of friends\r\n"));
                break;
            case 3:
                write(fd, "You can't friend yourself\r\n", strlen("You can't friend yourself\r\n"));
                break;
            case 4:
                write(fd, "The user you entered does not exist\r\n", strlen("The user you entered does not exist\r\n"));
                break;
        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 4) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 2; i < cmd_argc -1; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc - 1; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(cmd_argv[cmd_argc-1], user_list);
        User *target = find_user(cmd_argv[1], user_list);

        int other_fd = find_fd(cmd_argv[1], head);
        int other_len = strlen("From ") + strlen(cmd_argv[cmd_argc - 1]) + strlen(": ") + strlen(contents) + strlen("\r\n") + 1;


        char message_other[other_len];
        message_other[0] = '\0';
        strcat(message_other, "From ");
        strcat(message_other, cmd_argv[cmd_argc - 1]);
        strcat(message_other, ": ");
        strcat(message_other, contents);
        strcat(message_other, "\r\n");

        switch (make_post(author, target, contents)) {
            case 0:
                write(other_fd, message_other, strlen(message_other));
                break;

            case 1:
                free(contents);
                write(fd, "You can only post to your friends\r\n", strlen("You can only post to your friends\r\n"));
                break;
            case 2:
                free(contents);
                write(fd, "The user you want to post to does not exist\r\n", strlen("The user you want to post to does not exist\r\n"));
                break;
        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 3) { 
        User *user = find_user(cmd_argv[1], user_list);
        if (user == NULL) {
            write(fd, "User not found\r\n", strlen("User not found\r\n"));
        }
        else{
            char *user_string = print_user(user);
            write(fd, user_string, strlen(user_string));
            free(user_string);
        }
    } else {
        write(fd, "Incorrect syntax\r\n", strlen("Incorrect syntax\r\n")); 
    }
    return 0;
}


/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv, int fd) {
    int cmd_argc = 0;
    char *next_token = strtok(cmd, DELIM);    
    while (next_token != NULL) {
        if (cmd_argc >= INPUT_ARG_MAX_NUM - 1) {
            write(fd, "Too many arguments!\r\n", strlen("Too many arguments!\r\n"));
            cmd_argc = -1;
            break;
        }
        cmd_argv[cmd_argc] = next_token;
        cmd_argc++;
        next_token = strtok(NULL, DELIM);
    }

    return cmd_argc;
}


/*
 * Read a message from user and process the message.
 * Return the user fd if it has been closed or 0 otherwise.
 */
int buffered_read(struct sockname *user, struct sockname *head, User **users_list){
    int quit_result;
    int nbytes = read(user ->sock_fd, user->after, user ->room);
    if (nbytes == 0){
        return user ->sock_fd;
    }
    user -> inbuf += nbytes; //update inbuf with number of bytes added
    int where;

    // one full lines is in the user buf
    if ((where = find_network_newline(user -> buf, user -> inbuf)) > 0) {
        // where is now the index into buf immediately after the first network newline

        user-> buf[where - 2] = '\0'; // removing /r/n

        if (user ->username[0] == '\0'){
            char temp[BUFFER_SIZE]; 
            strncpy(temp, "add_user ", strlen("add_user "));
            temp[strlen("add_user ")] = '\0';
            strcat(temp, user ->buf);

            char *cmd_argv[INPUT_ARG_MAX_NUM];
            int cmd_argc = tokenize(temp, cmd_argv, user ->sock_fd); 
            process_args(cmd_argc, cmd_argv, users_list, user, user ->sock_fd);
            write(user ->sock_fd, "Go ahead and enter user commands>\r\n", strlen("Go ahead and enter user commands>\r\n"));  
        }
        else{
            user-> buf[where - 2] = '\0'; // removing /r/n
            char *cmd_argv[INPUT_ARG_MAX_NUM];
            int cmd_argc = tokenize(user ->buf, cmd_argv, user ->sock_fd);
            cmd_argv[cmd_argc] = user ->username;
            quit_result = process_args(cmd_argc + 1, cmd_argv, users_list,head, user ->sock_fd);
            if (cmd_argc > 0 && quit_result == -1) {return user ->sock_fd;}

        }           

        // update inbuf and remove the full line from the buffer
        user-> inbuf -= where;
        memmove(user-> buf, user-> buf + where, user-> inbuf);
    }

    // update after and room, in preparation for the next read.
    user->room = sizeof(user-> buf) - user-> inbuf;  
    user->after = user-> buf + user->inbuf;    

    return 0;
}


/*
 * Accept a connection. A new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, struct sockname *users) {

    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (users ->sock_fd == -1){
        users -> sock_fd = client_fd;   
        write(users ->sock_fd, "What is your user name?\r\n", strlen("What is your user name?\r\n")); //TODO: manage writing errors
        return client_fd;
    }
    else{
        struct sockname *curr = users;
        while(curr ->next != NULL){
            curr = curr ->next;
        }

        //initialize new user
        struct sockname *new_user = malloc(sizeof(struct sockname));
        new_user ->username[0] = '\0';
        new_user -> next = NULL;
        new_user -> sock_fd = client_fd;
        // initializing data for receiving messages
        new_user -> buf[0] = '\0'; 
        new_user ->inbuf = 0;
        new_user ->after = new_user ->buf;
        new_user ->room = sizeof(new_user);
        

        write(new_user ->sock_fd, "What is your user name?\r\n", strlen("What is your user name?\r\n")); //TODO: manage writing errors
        curr -> next = new_user;

        return client_fd;
    }
    
}