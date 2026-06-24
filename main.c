#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define FPS 60
#define WIDTH 800
#define HEIGHT 600
uint32_t pixels[WIDTH*HEIGHT];

#define STR2(s) #s
#define STR(s) STR2(s)
#define READ_END 0
#define WRITE_END 1

void check(int res, const char* errormsg)
{
    if (res < 0) {
        perror(errormsg);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    int wstatus, res;
    pid_t cpid, wpid;
    int pipefd[2];
    res = pipe(pipefd);
    check(res, "pipe2() failure");

    cpid = fork();
    check(cpid, "fork() failure");

    if (cpid == 0) {
        close(pipefd[WRITE_END]);
        res = dup2(pipefd[READ_END], STDIN_FILENO);
        check(res, "dup2() failure");

        res = execlp("ffmpeg",
                     "ffmpeg",
                     "-loglevel", "verbose",
                     "-y",
                     "-f", "rawvideo",
                     "-pix_fmt", "rgba",
                     "-s", STR(WIDTH) "x" STR(HEIGHT),
                     "-r", STR(FPS),
                     "-an",
                     "-i", "-",
                     "-c:v", "libx264",

                     "japan-flag.mp4",
                     NULL);

        check(res, "execlp() failure");
        close(pipefd[READ_END]);

        // THIS CODE MUST BE UNREACHABLE
        exit(EXIT_FAILURE);
    } 

    close(pipefd[READ_END]);
    // SENDING THE DATA TO THE FFMPEG STDING USING PIPES
    Olivec_Canvas oc = olivec_canvas(pixels, WIDTH, HEIGHT, WIDTH);
    uint32_t bg_color = 0xFFFFFFFF;
    uint32_t circle_color = 0xFF0000FF;
    olivec_fill(oc, bg_color);
    olivec_circle(oc, WIDTH/2, HEIGHT/2,  HEIGHT/8, circle_color);
    float duration = 5.f;

    for (size_t i = 0; i < duration*FPS; i++) {
        write(pipefd[WRITE_END], pixels, sizeof(pixels));
    }

    close(pipefd[WRITE_END]);


    // INSPECTION OF THE CHILD PROCESS
    do {
        wpid = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
        check(wpid, "waitpid() failure");

        if (WIFEXITED(wstatus)) {
            printf("exited,                     status=%d\n", WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("killed       by       signal       %d\n", WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            printf("stopped       by      signal       %d\n", WSTOPSIG(wstatus));
        } else if (WIFCONTINUED(wstatus)) {
            printf("continued\n");
        }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

    exit(EXIT_SUCCESS);
}
