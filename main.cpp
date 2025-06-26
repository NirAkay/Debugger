//compile - g++ main.cpp -o MyDebugger
//need addr2line
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/user.h>
#include <errno.h>
#include <string.h>
#include <elf.h>
#include <string>
#include <unordered_map>
#include <limits.h>

using namespace std;

/*
add print vars
*/
#define EXITOPTION 7
#define MAX_BACKTRACKS 10
#define BUF_LEN 3

void nextStep(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData);

int validOption() {
	char buf[BUF_LEN];
	int ret;
	while (1) {
		ret = 0;
		if (fgets(buf, sizeof(buf), stdin) != NULL) {
			int isNumber = 1, hasEnter = 0;
			for (int i = 0; i < BUF_LEN; i++) {
				if (buf[i] == '\n') {
					hasEnter = 1;
					break;
				}
				if (buf[i] > '9' || buf[i] < '0') {
					isNumber = 0;
					continue;
				}
				ret *= 10;
				ret += buf[i] - '0';
			}
			if (hasEnter && isNumber && ret >= 0 && ret <= EXITOPTION) {
				break;
			}
			if (!hasEnter) {
				do {
					ret = getchar();
				} while (ret != '\n' && ret != EOF);
			}
		}
		printf("Invalid option please try again!\nChoose an option please: ");
	}
	return ret;
}

void showByte(int pid, unsigned long addr) {
	long word = ptrace(PTRACE_PEEKDATA, pid, (void*)addr, NULL);
    errno = 0;
    if (word == -1 && errno != 0) {
        perror("PTRACE_PEEKDATA failed");
        exit(1);
    }
    unsigned char value = (unsigned char)(word & 0xFF);
	printf("%lx: %02x\n", addr, value);
}

void printRegs(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	struct user_regs_struct regs;
    errno = 0;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("PTRACE_GETREGS failed");
        exit(1);
    }
	printf("RIP: 0x%llx\n"
		"RSP: 0x%llx\n"
		"RBP: 0x%llx\n"
		"RAX: 0x%llx\n"
		"RBX: 0x%llx\n"
		"RCX: 0x%llx\n"
		"RDX: 0x%llx\n"
		"RSI: 0x%llx\n"
		"RDI: 0x%llx\n"
		"R8 : 0x%llx\n"
		"R9 : 0x%llx\n"
		"R10: 0x%llx\n"
		"R11: 0x%llx\n"
		"R12: 0x%llx\n"
		"R13: 0x%llx\n"
		"R14: 0x%llx\n"
		"R15: 0x%llx\n"
		"EFLAGS: 0x%llx\n", regs.rip, regs.rsp, regs.rbp, regs.rax, regs.rbx, regs.rcx, regs.rdx, regs.rsi, regs.rdi, regs.r8, regs.r9, regs.r10, regs.r11, regs.r12, regs.r13, regs.r14, regs.r15, regs.eflags);	
}

void printCommand(char* path, long addr, int pid, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	char line[256], command[256], lineNumber[10], sourcePath[256], tmp;
	int len, numberLine = 0, i;
	snprintf(command, sizeof(command), "addr2line -e %s -f -p %lx", path, addr);
	FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen failed");
        exit(1);
    }
	fgets(line, sizeof(line), fp);
	if (!strcmp(line, "?? ??:0\n")) {
		pclose(fp);
		nextStep(pid, path, brkToAddr, addrToData);
		return;
	}
	len = strlen(line) - 2;
	int mul = 1;
	for (; line[len] != ':'; len--) {
		numberLine += (line[len] - '0') * mul;
		mul *= 10;
	}
	if (numberLine > 5000 || numberLine < 0) {
		nextStep(pid, path, brkToAddr, addrToData);
		pclose(fp);
		return;
	}
	len--;
	for (i = 0; line[len] != ' '; i++) {
		sourcePath[i] = line[len--];
	}
	sourcePath[i + 1] = '\0';
	for (int j = 0; j < i / 2; j++) {
		tmp = sourcePath[j];
		sourcePath[j] = sourcePath[i - j - 1];
		sourcePath[i - j - 1] = tmp;
	}
	FILE* src = fopen(sourcePath, "r");
    if (!src) {
        perror("fopen failed");
        pclose(fp);
        exit(1);
    }
	for (i = 1; i < numberLine; i++) {
		fgets(command, sizeof(command), src);
	}
	fgets(command, sizeof(command), src);
	printf("(%lx)%d. %s", addr, numberLine, command);
	fclose(src);
	pclose(fp);
}

void removeBreak(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	char line[256];
	scanf("%[^\n]", line);
	char c;
	scanf("%c", &c);
    errno = 0;
    if (ptrace(PTRACE_POKETEXT, pid, brkToAddr[line], (void *) ((addrToData[brkToAddr[line]] & 0xFFFFFFFFFFFFFF00) | addrToData[brkToAddr[line]])) == -1) {
        perror("PTRACE_POKETEXT failed");
        exit(1);
    }
	addrToData.erase(brkToAddr[line]);
	brkToAddr.erase(line);
}

void nextBreak(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	int status;
	struct user_regs_struct regs;
	for (auto itr: brkToAddr) {
		errno = 0;
		if (ptrace(PTRACE_POKETEXT, pid, (void *)itr.second, (void *)addrToData[itr.second]) == -1) {
		    perror("PTRACE_POKETEXT failed");
		    exit(1);
		}
	}
	errno = 0;
    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
        perror("PTRACE_SINGLESTEP failed");
        exit(1);
    }
	waitpid(pid, &status, 0);
	for (auto itr: brkToAddr) {
        errno = 0;
        if (ptrace(PTRACE_POKETEXT, pid, itr.second, (void *) ((addrToData[itr.second] & 0xFFFFFFFFFFFFFF00) | 0xCC)) == -1) {
            perror("PTRACE_POKETEXT failed");
            exit(1);
        }
	}
    errno = 0;
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("PTRACE_CONT failed");
    }
    waitpid(pid, &status, 0);
    errno = 0;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("PTRACE_GETREGS failed");
        exit(1);
    }
	regs.rip--;
	errno = 0;
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("PTRACE_SETREGS failed");
        exit(1);
    }
    if (WIFSTOPPED(status)) {
	    printCommand(path, regs.rip, pid, brkToAddr, addrToData);
	}
}

void nextStep(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	if (!brkToAddr.size()) {
		return;
	}
	int status;
	struct user_regs_struct regs;
	errno = 0;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("PTRACE_GETREGS failed");
        exit(1);
    }
	if (addrToData.find(regs.rip) != addrToData.end()) {
        errno = 0;
        if (ptrace(PTRACE_POKETEXT, pid, (void *)regs.rip, (void *)addrToData[regs.rip]) == -1) {
            perror("PTRACE_POKETEXT failed");
            exit(1);
        }
	}
    errno = 0;
    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
        perror("PTRACE_SINGLESTEP failed");
        exit(1);
    }
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
		printf("Process exited\n");
	    return;
	}
	if (WIFSIGNALED(status)) {
	    printf("Process killed by signal\n");
	   	return;
	}
	if (WIFSTOPPED(status)) {
	    errno = 0;
        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
            perror("PTRACE_GETREGS failed");
            exit(1);
        }
		if (addrToData.find(regs.rip) != addrToData.end()) {
            errno = 0;
            if (ptrace(PTRACE_POKETEXT, pid, (void *)regs.rip - 1, (void *) ((addrToData[regs.rip - 1] & 0xFFFFFFFFFFFFFF00) | 0xCC)) == -1) {
                perror("PTRACE_POKETEXT failed");
                exit(1);
            }
		}
		printCommand(path, regs.rip, pid, brkToAddr, addrToData);
	}
}

void addBreakPoint(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	char line[256], command[256];
	long addr;
	scanf("%[^\n]", line);
	char c;
	scanf("%c", &c);
	if (brkToAddr.find(line) != brkToAddr.end()) {
		return;
	}
	snprintf(command, sizeof(command), "nm %s | grep ' %s$'", path, line);
	FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen failed");
        exit(1);
    }
	fgets(command, sizeof(command), fp);
    sscanf(command, "%lx", &addr);
    pclose(fp);
    long data = ptrace(PTRACE_PEEKTEXT, pid, (void *) addr, NULL);
    errno = 0;
    if (data == -1 && errno != 0) {
        perror("PTRACE_PEEKDATA failed");
        exit(1);
    }
    if (data == 0xffffffffffffffff) {
    	return;
    }
    brkToAddr[line] = addr;
	addrToData[brkToAddr[line]] = data;
}

void printMenu(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
	printf("1. Add breakpoint\n"
		"2. Next Breakpoint\n"
		"3. Next Step\n"
		"4. Remove Breakpoint\n"
		"5. Print Registers\n"
		"6. Print Vars\n"
		"%d. Exit\n", EXITOPTION);
}

void printBacktrace(int pid, char* path, unordered_map<string, unsigned long>& brkToAddr, unordered_map<unsigned long, long>& addrToData) {
    struct user_regs_struct regs;
    unsigned long rbp, rip;
    char cmd[256], buf[256];
    int j;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("PTRACE_GETREGS failed");
        exit(1);
    }
    rbp = regs.rbp;
    rip = regs.rip;
    printf("Backtrace:\n");
    for (int i = 0; i < 10 && rbp != 0; i++) {
        snprintf(cmd, sizeof(cmd), "addr2line -e %s -f -p 0x%lx", path, rip);
        FILE* fp = popen(cmd, "r");
        if (!fp) {
			perror("popen failed");
			exit(1);
	    }
        while (fgets(buf, sizeof(buf), fp)) {
            if (!strcmp(buf, "?? ??:0\n")) {
				continue;
            }
            j = 0;
            while (buf[j] && buf[j] != ' ') {
            	j++;
            }
            buf[j] = '\0';
            printf("%s\n", buf);
        }
        pclose(fp);
        rip = ptrace(PTRACE_PEEKDATA, pid, rbp + 8, NULL);
        rbp = ptrace(PTRACE_PEEKDATA, pid, rbp, NULL);
    }
}

void choseMenu(int pid, char* path) {
	void (*menu[])(int, char*, unordered_map<string, unsigned long>&, unordered_map<unsigned long, long>&) = {printMenu, addBreakPoint, nextBreak, nextStep, removeBreak, printRegs, printBacktrace};
	int choice = -1;
	char c;
	unordered_map<string, unsigned long> brkToAddr;
	unordered_map<unsigned long, long> addrToData;
	printMenu(pid, path, brkToAddr, addrToData);
	while (1) {
		printf("Choose an option please: ");
		choice = validOption();
		if (choice == EXITOPTION) {
			kill(pid, SIGKILL);
			return;
		}
		menu[choice](pid, path, brkToAddr, addrToData);
	}
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please enter a program\n");
        return 1;
    }
    int pid = fork();
    if (pid) {
        int status;
        waitpid(pid, &status, 0);
        choseMenu(pid, argv[1]);
    } else {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv + 1);
        perror("execvp failed");
        return 1;
    }
    return 0;
}
