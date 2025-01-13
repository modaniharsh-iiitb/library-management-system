#ifndef LIBRARY_H
#define LIBRARY_H

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

// server details
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// error codes
#define INCORRECT_TOKEN -2
#define INCORRECT_USERNAME_OR_ID -1

// sizes
#define MAX_CLIENTS 10
#define TOKEN_SIZE 20
#define USERNAME_SIZE 20
#define TITLE_SIZE 50
#define AUTHOR_SIZE 20
#define GENRE_SIZE 20
#define BUFFER_SIZE 1024

// counts
#define MAX_BOOKS 100
#define MAX_COPIES 500
#define MAX_MEMBERS 100
#define MAX_ADMINS 20
#define MAX_BORROWED_BOOKS 5

// strings
#define MEMBER_PROMPT "\nEnter the following:\n1. View all books\n2. Search for a book\n3. Borrow a book\n4. Return a book\n5. See the list of books borrowed by you\n6. Change password\n7. Log out\n> "
#define ADMIN_PROMPT "\nEnter the following:\n1. View the list of books\n2. Add a new book\n3. Remove a book\n4. View the list of members\n5. Add a new member\n6. Remove a member\n7. View the list of copies\n8. Add a copy of a book\n9. Remove a copy\n10. Change password\n11. Log out\n> "

// structures used by the library system
typedef struct Admin {
    int admin_id;
    char token[TOKEN_SIZE];
} Admin;

typedef struct Member {
    int member_id;
    char username[USERNAME_SIZE];
    char token[TOKEN_SIZE];
} Member;

typedef struct Book {
    int book_id;
    char title[TITLE_SIZE];
    char author[AUTHOR_SIZE];
    char genre[GENRE_SIZE];
} Book;

typedef struct Copy {
    int book_id;
    int copy_id;
    int borrowed_member_id;
} Copy;

#endif