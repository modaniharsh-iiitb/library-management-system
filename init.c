#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

int main() {
    int fd = open("admins.dat", O_CREAT | O_WRONLY, 0644);
    for (int i = 1; i <= 5; i++) {
        int admin_id = 100+i;
        char token[TOKEN_SIZE];
        sprintf(token, "admin%d123", i);
        Admin admin;
        admin.admin_id = admin_id;
        strcpy(admin.token, token);
        write(fd, &admin, sizeof(Admin));
    }
    close(fd);

    fd = open("members.dat", O_CREAT | O_WRONLY, 0644);
    for (int i = 1; i <= 20; i++) {
        int member_id = i;
        char username[USERNAME_SIZE];
        sprintf(username, "member%d", i);
        char token[TOKEN_SIZE];
        sprintf(token, "user%d123", i);
        Member member;
        member.member_id = member_id;
        strcpy(member.username, username);
        strcpy(member.token, token);
        write(fd, &member, sizeof(Member));
    }
    close(fd);

    fd = open("books.dat", O_CREAT | O_WRONLY, 0644);
    for (int i = 1; i <= 50; i++) {
        int book_id = i;
        char title[TITLE_SIZE];
        sprintf(title, "Book %d", i);
        char author[AUTHOR_SIZE];
        sprintf(author, "Author %d", i);
        char genre[GENRE_SIZE];
        sprintf(genre, "Genre %d", i);
        Book book;
        book.book_id = book_id;
        strcpy(book.title, title);
        strcpy(book.author, author);
        strcpy(book.genre, genre);
        write(fd, &book, sizeof(Book));
    }
    close(fd);

    fd = open("copies.dat", O_CREAT | O_WRONLY, 0644);
    for (int i = 1; i <= 50; i++) {
        int book_id = i;
        int n = rand() % 5 + 1;
        for (int j = 1; j <= n; j++) {
            int copy_id = j;
            Copy copy = {book_id, copy_id, 0};
            write(fd, &copy, sizeof(Copy));
        }
    }
}