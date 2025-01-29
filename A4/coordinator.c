#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "boardgen.c"

void display_help(){
    printf("\nCommands supported\n");
    printf("\tn\t\tStart new game\n");
    printf("\tp b c d\t\tPut digit d [1-9] at cell [0-8] of block b [0-8]\n");
    printf("\ts\t\tShow solution\n");
    printf("\th\t\tPrint this help message\n");
    printf("\tq\t\tQuit\n");
    printf("\nNumbering scheme for blocks and cells\n\n");
    for(int i=0;i<9;i+=3){
        printf("\t+---+---+---+\n");
        printf("\t| %d | %d | %d |\n",i,i+1,i+2);
    }
    printf("\t+---+---+---+\n");
}

void send_msg(int fd, char* msg){
    int stdout_fd = dup(1);
    close(1);
    dup(fd);
    printf("%s\n",msg);
    close(1);
    dup(stdout_fd);
}

void new_game(int A[9][9], int S[9][9], int pfd[9][2]){
    newboard(A,S);
    for(int i=0;i<9;i++){
        int r=(i/3)*3, c=(i%3)*3;
        char msg[50];
        sprintf(msg,"n %d %d %d %d %d %d %d %d %d",
        A[r][c], A[r][c+1], A[r][c+2],
        A[r+1][c], A[r+1][c+1], A[r+1][c+2],
        A[r+2][c], A[r+2][c+1], A[r+2][c+2]);
        send_msg(pfd[i][1],msg);
    }
}

void display_solution(int S[9][9], int pfd[9][2]){
    for(int i=0;i<9;i++){
        int r=(i/3)*3, c=(i%3)*3;
        char msg[50];
        sprintf(msg,"s %d %d %d %d %d %d %d %d %d",
        S[r][c], S[r][c+1], S[r][c+2],
        S[r+1][c], S[r+1][c+1], S[r+1][c+2],
        S[r+2][c], S[r+2][c+1], S[r+2][c+2]);
        send_msg(pfd[i][1],msg);
    }
}

void put_digit(int pfd[9][2]){
    int b,c,d;
    scanf("%d %d %d", &b, &c, &d);
    char msg[20];
    sprintf(msg,"p %d %d",c,d);
    send_msg(pfd[b][1],msg);
}

void handle_quit(int pfd[9][2]){
    for(int i=0;i<9;i++) send_msg(pfd[i][1],"q");
    for(int i=0;i<9;i++) wait(NULL);
    exit(0);
}

int main(){
    int A[9][9], S[9][9], pfd[9][2];
    int rneigh[9][2] = {{1, 2}, {0, 2}, {0, 1}, {4, 5}, {3, 5}, {3, 4}, {7, 8}, {6, 8}, {6, 7}};
    int cneigh[9][2] = {{3, 6}, {4, 7}, {5, 8}, {0, 6}, {1, 7}, {2, 8}, {0, 3}, {1, 4}, {2, 5}};

    display_help();

    for(int i=0;i<9;i++){
        if(pipe(pfd[i])<0){
            perror("pipe");
            exit(1);
        }
    }
    for(int i=0;i<9;i++){
        pid_t pid = fork();
        if(pid<0){
            perror("fork");
            exit(1);
        }
        if(pid==0){
            char title[20], blockno[10], bfdin[10], bfdout[10], rn1fdout[10], rn2fdout[10], cn1fdout[10], cn2fdout[10], geometry[20];
            sprintf(title, "Block %d", i);
            sprintf(blockno, "%d", i);
            sprintf(bfdin, "%d", pfd[i][0]);
            sprintf(bfdout, "%d", pfd[i][1]);
            sprintf(rn1fdout, "%d", pfd[rneigh[i][0]][1]);
            sprintf(rn2fdout, "%d", pfd[rneigh[i][1]][1]);
            sprintf(cn1fdout, "%d", pfd[cneigh[i][0]][1]);
            sprintf(cn2fdout, "%d", pfd[cneigh[i][1]][1]);
            int geom_x = 900 + (i%3)*250, geom_y = 100 + (i/3)*300;
            sprintf(geometry,"17x8+%d+%d",geom_x,geom_y);
            execlp("xterm", "xterm", "-T", title, "-fa", "Monospace", "-fs", "15", "-geometry", geometry, "-bg", "#331100",
            "-e", "./block", blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout, NULL);
        }
    }

    while(1){
        printf("\nFoodoku> ");
        char cmd;
        scanf(" %c",&cmd);

        switch(cmd){
            case 'n':
                new_game(A,S,pfd);
                break;
            case 'h':
                display_help();
                break;
            case 's':
                display_solution(S,pfd);
                break;
            case 'p':
                put_digit(pfd);
                break;
            case 'q':
                handle_quit(pfd);
                break;
        }
    }
}