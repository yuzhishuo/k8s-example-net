#pragma onece
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024)

// sync primitive
int checkpoint[2];

static char child_stack[STACK_SIZE];

char * const child_args[] = {
    "/bin/bash",
    NULL
};

int child_main(void *arg)
{
    char c;
    // init sync primitive
    close(checkpoint[1]);

    // setup hostname
    printf(" - [%5d] Word ! \n", getpid());
    sethostname("in Namespace", 12);
    
    // remount "/proc" to get accurate "top" && "ps" output
    mount("proc", "/proc", "proc", 0, NULL);

    // wait for network setup in parent
    read(checkpoint[0], &c, 1);

    // setup network
    system("ip link set dev eth0 up");
    system("ip link veth1 up");
    system("ip addr add 169.254.1.2/30 dev veth1");

    execv(child_args[0], child_args);
    printf("0oooops\n");
    return -1;

}

int main(int, char**) {

    // init sync primitive
    pipe(checkpoint);

    printf(" - [%5d] Hello ? \n", getpid());

    int child_pid = clone(child_main, child_stack + STACK_SIZE,
                          CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET | SIGCHLD, NULL);
    // further init: create a veth pair
    char* cmd;

    asprintf(&cmd, "ip link set veth1 netns %d", child_pid);
    system("ip link add veth0 type veth peer name veth1");
    system(cmd);
    system("ip link set veth0 up");
    system("ip addr add 169.254.254.1.1/30 dev veth0");

    free(cmd);

    // signal "done"
    close (checkpoint[1]);

    waitpid(child_pid, NULL, 0);

    return 0;
}
