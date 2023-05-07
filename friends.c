#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


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
int create_user(const char *name, User **user_ptr_add, struct sockname *user) {
    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }

    int trunc = 0;
    if (strlen(name) >= MAX_NAME) {
        trunc = 1;
    }

    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME - 1
    new_user -> name[MAX_NAME - 1] = '\0';
    strcpy(user ->username,new_user -> name);

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {
        *user_ptr_add = new_user;
        if(trunc){return 2;}
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        if(trunc){return 2;}
        return 0;
    }
}


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 */
User *find_user(const char *name, const User *head) {
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }
    return (User *)head;
}


int user_space(const User *curr){
    int length = 0;
    while (curr != NULL) {
        length += strlen(curr->name);
        length += 2; // not adding space for null terminator, only newlines
        curr = curr->next;
    }
    return length + 1;
}

/*
 * Returns a string with the usernames of all users in the list starting at curr.
 */
char *list_users(const User *curr) {
    int len = user_space(curr);
    char buf[len];
    int pos = 1;
    while (curr != NULL) {
        pos += snprintf(buf + pos - 1, strlen(curr->name) + 3, "%s\r\n", curr->name);
        curr = curr->next;
    }

    char *string = malloc((sizeof(char) * strlen(buf)) + 1);
    if(string == NULL){
        perror("malloc");
        exit(1);
    }
    strcpy(string, buf);
    return string;
}


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
int make_friends(const char *name1, const char *name2, User *head) {
    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);

    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        }
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}


char *print_post(const Post *post) {
    if (post == NULL) {
        return "";
    }

    int author_length = strlen("From: %s\r\n") + strlen(post->author) - 1; //-2 for %s and + 1 for null term = -1  
    int date_length = strlen("Date: %s\r\n") + strlen(asctime(localtime(post->date))) - 1; 
    int content_length = strlen("%s\r\n") + strlen(post->contents) - 1;
    int length = author_length + date_length + content_length - 1;

    char buf[length];
    int pos = 0;
    pos += snprintf(buf + pos, author_length, "From: %s\r\n", post->author);
    pos += snprintf(buf + pos, date_length, "Date: %s\r\n", asctime(localtime(post->date)));
    pos += snprintf(buf + pos, content_length, "%s\r\n", post->contents);

    char *string = malloc((sizeof(char) * strlen(buf)) + 1);
    if(string == NULL){
        perror("malloc");
        exit(1);
    }
    strcpy(string, buf);
    return string;
}



/*
 * Return string with a user profile.
 */
char *print_user(const User *user) {
    if (user == NULL) {
        return "User not found\r\n";
    }
    int length = 0;
    int line = strlen("------------------------------------------\r\n");
    length += line *3; // 3 lines

    int name = strlen("Name: %s\r\n\r\n") + strlen(user->name) - 2; // -2 (%s)
    length += name;

    int friend = strlen("Friends:\r\n"); 
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        length += strlen("%s\r\n") + strlen(user->friends[i]->name) - 2; // -2 (%s)
    }
    length += friend;

    int post = strlen("Posts:\r\n");
    const Post *curr = user->first_post;
    while (curr != NULL) {
        char *post = print_post(curr);
        length += strlen(post);
        curr = curr->next;
        if (curr != NULL) {
            length += strlen("===\r\n");
        }
    }
    length += post;  
    
    length += 1; // adds last null terminator

    char buf[length];
    int pos = 0;
    pos += snprintf(buf + pos, name + 1, "Name: %s\r\n\r\n", user->name);
    pos += snprintf(buf + pos, line + 3, "%s", "------------------------------------------\r\n");
    pos += snprintf(buf + pos, friend + 3, "%s", "Friends:\r\n");
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        pos += snprintf(buf + pos, strlen(user->friends[i]->name) + 3, "%s\r\n",  user->friends[i]->name);
    }
    pos += snprintf(buf + pos, line + 3, "%s", "------------------------------------------\r\n");
    pos += snprintf(buf + pos, friend + 3, "%s", "Posts:\r\n");
    const Post *curr_ = user->first_post;
    while (curr_ != NULL) {
        char *post = print_post(curr_);
        pos += snprintf(buf + pos, strlen(post) + 3, "%s", post);
        curr_ = curr_->next;
        if (curr_ != NULL) {
            pos += snprintf(buf + pos, strlen("===\r\n") + 1, "%s", "===\r\n");
        }
    }
    pos += snprintf(buf + pos, line + 3, "%s", "------------------------------------------\r\n");

    int count = 0;
    for (int i = 0; i < length; i++){
        if (buf[i] != '\0'){
            count +=1;
        }
    }
    char *string = malloc((sizeof(char) * strlen(buf)) + 1);
    if(string == NULL){
        perror("malloc");
        exit(1);
    }

    strcpy(string,buf);
    return string;
}


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
int make_post(const User *author, User *target, char *contents) {
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}
