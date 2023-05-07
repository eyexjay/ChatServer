#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>

#define MAX_NAME 32     // Max username and profile_pic filename lengths
#define MAX_FRIENDS 10  // Max number of friends a user can have
#define BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \n"
#define MAX_BACKLOG 5


typedef struct user {
    char name[MAX_NAME];
    struct post *first_post;
    struct user *friends[MAX_FRIENDS];
    struct user *next;
} User;

typedef struct post {
    char author[MAX_NAME];
    char *contents;
    time_t *date;
    struct post *next;
} Post;

struct sockname {
    int sock_fd;
    char username[MAX_NAME];
    struct sockname *next;
    char buf[BUFFER_SIZE];
    int inbuf;           
    int room;  
    char *after;   
    
};



/*
 * Searches the first n characters of buf for a network newline (\r\n).
 * Returns one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found. The return value is the index into buf
 * where the current line ends.
 */
int find_network_newline(const char *buf, int n);

/*
 * Finds file desrcriptor of user with the username name
 */
int find_fd(char *name, struct sockname *head);


/*
 * Read and process commands
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, struct sockname *head, int fd);


/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv, int fd);


/*
 * Read a message from user and process the message.
 * Return the user fd if it has been closed or 0 otherwise.
 */
int buffered_read(struct sockname *user, struct sockname *head, User **users_list);

/*
 * Accept a connection. A new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, struct sockname *users);


/*
 * Create a new user with the given name. Insert it at the tail of the list
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add, struct sockname *head);


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 */
User *find_user(const char *name, const User *head);


/*
 * Returns a string with the usernames of all users in the list starting at curr.
 */
char *list_users(const User *curr);



/*
 * Make two users friends with each other.  This is symmetric - a pointer to
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * If multiple errors apply, return the *largest* error code that applies.
 */
int make_friends(const char *name1, const char *name2, User *head);


/*
 * Return string with a user profile.
 */
char *print_user(const User *user);


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents);





