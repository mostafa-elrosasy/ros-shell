#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

void create_child_process(char **tokens, int tokens_num){
    int rc = fork();
    if(rc == 0){
        for(int i = 0; i < tokens_num; i++){
            if(strcmp(tokens[i], ">") == 0){
                if(i+1 == tokens_num){
                    perror("file name wasn't provided");
                    exit(EXIT_FAILURE);
                }
                int fd = open(tokens[i+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);
                tokens_num = i;
                break;
            }
        }
        char *temp = strdup(tokens[0]);
        tokens[0] = strdup("/bin/");
        tokens[0] = strcat(tokens[0], temp);
        tokens[tokens_num] = NULL;
        execv(tokens[0], tokens);
        perror("execution failed\n");
        exit(EXIT_FAILURE);
    }
}

char ** copy_tokens(char **tokens, int tokens_num){
    char **copy;
    copy = malloc(tokens_num * sizeof *copy);

    for(int i = 0; i < tokens_num; i++)
    {
        copy[i] = strdup(tokens[i]);
    }
    return copy;
}

void execute(char **tokens, int tokens_num){
    char ** new_cmd_start = tokens;
    int last_cmd_index = 0, i;
    for(i = 0; i < tokens_num; i++){
        if(strcmp(tokens[i], "&") == 0){
            create_child_process(new_cmd_start, i - last_cmd_index);
            new_cmd_start = tokens + i +1;
            last_cmd_index = i + 1;
        }
    }
    create_child_process(new_cmd_start, i - last_cmd_index);
    int rc_wait = wait(NULL);
}

int check_built_in_commands(char **tokens, int tokens_num){
    if (strcmp(tokens[0], "exit") == 0){
        exit(0);
    } else if (strcmp(tokens[0], "cd") == 0){
        if (tokens_num > 1){
            chdir(tokens[1]);
        } else {
            chdir(getenv("HOME"));
        }
    } else {
        return 0;
    }
    return 1;
}

void parse_line(char *line){
    char *token, *copy;
    char *tokens[100];
    int tokens_num = 0;
    copy = line;
    while(token = strsep(&copy, " ")){
        if(token[0] == '\0' || token[0] == '\n' || token[0] == '\t')
            continue;
        tokens[tokens_num] = token;
        tokens_num++;
    }
    int last_token_length = strlen(tokens[tokens_num-1]);
    if (tokens[tokens_num-1][last_token_length-1]  == '\n')
        tokens[tokens_num-1][last_token_length-1] = '\0';
    if (last_token_length > 1 && tokens[tokens_num-1][last_token_length-2]  == '\t')
        tokens[tokens_num-1][last_token_length-2] = '\0';
    if (! check_built_in_commands(tokens, tokens_num))
        execute(tokens, tokens_num);
}

int main(int argc, char *argv[]) {
    while(1){
        char cwd[100];
        getcwd(cwd, sizeof(cwd));
        printf("ROS: %s>", cwd);
        char* line = NULL, *token, *copy;
        size_t len =0;
        int n;
        if(argc > 1){
            FILE *stream = fopen(argv[1], "r");
            if (stream == NULL) {
               perror("file doesn't exist");
               exit(EXIT_FAILURE);
            }
            while((n = getline(&line, &len, stream)) != -1){
                if(! *line == '\n')
                    parse_line(line);
            }
            exit(0);
        }
        n = getline(&line, &len, stdin);
        if(! (*line == '\n'))
            parse_line(line);
    }
    return 0;
}
