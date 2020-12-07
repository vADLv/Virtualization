#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

static char child_stack[1048576];

static int child_fn() {
  // calling unshare() from inside the init process lets you create a new namespace after a new process has been spawned
  //unshare(CLONE_NEWNET);
  getchar();

  //benchmarks
  system("sysbench --test=cpu --cpu-max-prime=20000 run");
  getchar();
  system("sysbench --num-threads=16 --test=fileio --file-total-size=1G --file-test-mode=rndrw prepare");
  system("sysbench --num-threads=16 --test=fileio --file-total-size=1G --file-test-mode=rndrw run");
  system("sysbench --num-threads=16 --test=fileio --file-total-size=1G --file-test-mode=rndrw cleanup");
  getchar();
  system("sysbench --test=memory --memory-block-size=1K --memory-scope=global --memory-total-size=10G --memory-oper=write run");
  getchar();
  system("sysbench --test=memory --memory-block-size=1K --memory-scope=global --memory-total-size=10G --memory-oper=read run");
  getchar();
  system("sysbench --num-threads=64 --test=threads --thread-yields=100 --thread-locks=2 run");
  getchar();


  system("mount -t ext4 /dev/loop10 loopfs"); //mount virtual storage device

  system("df -hP loopfs/"); //check virtual device volume
  system("df -hP v/"); //check host device volume
  system("echo test > loopfs/test.txt"); //create some file
  system("ls -l"); //show file structure

  getchar();
  //also we can change root directory to hide all host files from the comtainer view
  //system("chroot(\"./fs\")"");
  //system("chdir(\"/\");");

  //memory.limit_in_bytes = 1073741824; //doesn't work

  //veth settings
  system("ip addr add 192.168.1.11/24 dev veth1");
  system("ip link set veth1 up");
  system("ip route add default via 192.168.1.10");
  system("ip route");

  //check network settings
  printf("New `net` Namespace:\n");
  system("ip link");
  printf("\n\n");
  system("ifconfig | grep \"inet \"");
  printf("\n\n");

  //check process namespace
  printf("PID: %ld\n", (long)getpid());
  printf("Parent PID: %ld\n", (long)getppid());

  getchar();

  //ping parent network namespace
  system("ping -c 4 192.168.1.10");

  //check memory usage
  //system("cat /proc/meminfo | grep Mem");
  system("free -h");

  system("mount /dev/sda3 v/"); //mount windows folder change sda3 as you wish :)

  getchar();
  //while (1){

  //}
  return 0;
}

int main() {
  //create 2 folders for testing
  system("mkdir -p v");
  system("mkdir -p loopfs");

  //network setup
  system("ip link add name br1 type bridge");
  system("ip addr add 192.168.1.10/24 brd + dev br1");
  system("ip link set br1 up");

  //the command below to create virtal image need to be launch just once
  //system("dd if=/dev/zero of=loopbackfile.img bs=500M count=10");
  //system("losetup -fP loopbackfile.img");
  //system("mkfs.ext4 /root/loopbackfile.img");

  char cmd[200];
  //char meml[200];

  getchar();

  //new child process
  pid_t child_pid = clone(child_fn, child_stack+1048576, CLONE_NEWPID | CLONE_NEWNET| CLONE_NEWNS | SIGCHLD, NULL);
  printf("clone() = %ld\n", (long)child_pid);

  //create string command for network setup in the child network namespace
  int length = snprintf(NULL,0,"%d",child_pid);
  char* str = malloc(length + 1);
  snprintf(str, length+1,"%d", child_pid);
  strcpy(cmd, "ip link add name veth0 type veth peer name veth1 netns ");
  strcat(cmd, str);
  //printf("%s\n",cmd); //check the command
  system(cmd);

  system("ip link set veth0 up");
  system("ip link set veth0 master br1");

  //use cgroups to limit memory
  //system("echo 100M > /sys/fs/cgroup/memory/group0/memory.limit_in_bytes");
  //strcpy(meml, "echo ");
  //strcat(meml, str);
  //strcat(meml, " > /sys/fs/cgroup/memory/group0/tasks");
  //strcat(meml, " > /sys/fs/cgroup/memory/group0/cgroup.procs");
  //system(meml);
  //printf("%s\n",meml);

  waitpid(child_pid, NULL, 0);

  //check network settings
  printf("Original `net` Namespace:\n");
  system("ip link");
  printf("\n\n");
  system("ifconfig | grep \"inet \"");
  printf("\n\n");

  //delete all unnecessary settings
  system("ip link delete br1");
  system("sudo umount /dev/sda3");

  //check memory usage
  //system("cat /proc/meminfo | grep Mem");
  system("free -h");
  //system("rm -rf v");
  return 0;
}
