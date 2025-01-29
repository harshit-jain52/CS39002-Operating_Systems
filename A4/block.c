#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

void print_puzzle(int B[3][3]){
    for(int i=0;i<3;i++){
        printf(" +---+---+---+\n");
        printf(" |");
        for(int j=0;j<3;j++){
            if(B[i][j]) printf(" %d |", B[i][j]);
            else printf("   |");
        }
        printf("\n");
    }
    printf(" +---+---+---+\n");
}

void new_game(int A[3][3], int B[3][3]){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            scanf("%d",&A[i][j]);
            B[i][j] = A[i][j];
        }
    }
}

void display_solution(int B[3][3]){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            scanf("%d",&B[i][j]);
        }
    }
}

bool read_only_check(int c, int A[3][3]){
    int row=c/3, col=c%3;
    return (A[row][col]==0);
}

bool block_check(int d, int B[3][3]){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(B[i][j]==d) return false;
        }
    }
    return true;
}

void send_msg(int fd, char* msg){
    int stdout_fd = dup(1);
    close(1);
    dup(fd);
    printf("%s\n",msg);
    close(1);
    dup(stdout_fd);
}

bool row_check(int i, int d, int rn1fdout, int rn2fdout, int bfdout){
    char msg[10]; int res;
    sprintf(msg,"r %d %d %d",i,d,bfdout);
    send_msg(rn1fdout,msg);
    scanf("%d",&res);
    if(res!=0) return false;
    sprintf(msg,"r %d %d %d",i,d,bfdout);
    send_msg(rn2fdout,msg);
    scanf("%d",&res);
    return (res==0);
}

bool col_check(int j, int d, int cn1fdout, int cn2fdout, int bfdout){
    char msg[10]; int res;
    sprintf(msg,"c %d %d %d",j,d,bfdout);
    send_msg(cn1fdout,msg);
    scanf("%d",&res);
    if(res!=0) return false;
    sprintf(msg,"c %d %d %d",j,d,bfdout);
    send_msg(cn2fdout,msg);
    scanf("%d",&res);
    return (res==0);
}

void print_msg(const char* msg){
    printf("%s",msg);
    fflush(NULL);
    sleep(2);
}

void put_digit(int A[3][3], int B[3][3], int rn1fdout, int rn2fdout, int cn1fdout, int cn2fdout, int bfdout){
    int c,d;
    scanf("%d %d", &c, &d);
    if(!read_only_check(c,A)){
        print_msg("Readonly Cell");
        return;
    }
    if(!block_check(d,B)){
        print_msg("Block conflict");
        return;
    }
    if(!row_check(c/3,d,rn1fdout,rn2fdout,bfdout)){
        print_msg("Row conflict");
        return;
    }
    if(!col_check(c%3,d,cn1fdout,cn2fdout,bfdout)){
        print_msg("Column conflict");
        return;
    }
    B[c/3][c%3] = d;
}

void row_req(int B[3][3]){
    int i,d,bfd;
    bool flag = false;
    scanf("%d %d %d", &i, &d, &bfd);
    for(int j=0;j<3;j++){
        flag |= (B[i][j]==d);
    }
    char msg[5];
    sprintf(msg,"%d",flag);
    send_msg(bfd,msg);
}

void col_req(int B[3][3]){
    int j,d,bfd;
    bool flag = false;
    scanf("%d %d %d", &j, &d, &bfd);
    for(int i=0;i<3;i++){
        flag |= (B[i][j]==d);
    }
    char msg[5];
    sprintf(msg,"%d",flag);
    send_msg(bfd,msg);
}

int main(int argc, const char *argv[]){
    int blockno = atoi(argv[1]);
    int bfdin = atoi(argv[2]);
    int bfdout = atoi(argv[3]);
    int rn1fdout = atoi(argv[4]);
    int rn2fdout = atoi(argv[5]);
    int cn1fdout = atoi(argv[6]);
    int cn2fdout = atoi(argv[7]);

    close(0);
    dup(bfdin);

    printf("Block %d ready\n",blockno);
    int A[3][3], B[3][3];
    while(1){
        char cmd;
        scanf("%c",&cmd);
        switch(cmd){
            case 'n':
                new_game(A,B);
                break;
            case 's':
                display_solution(B);
                break;
            case 'p':
                put_digit(A,B,rn1fdout,rn2fdout,cn1fdout,cn2fdout,bfdout);
                break;
            case 'r':
                row_req(B);
                break;
            case 'c':
                col_req(B);
                break;
            case 'q':
                print_msg("Bye...");
                exit(0);
        }
        print_puzzle(B);
    }
    return 0;
}