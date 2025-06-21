#include <stdio.h>

int g = 8;
char arr[90] = "hello";

void prime() {
	int isPrime, num;
	printf("prime:\nEnter a number: \n");
    	scanf("%d", &num);
	for (int i = 2; i <= num; i++) {
		isPrime = 1;
		for (int j = 2; j < i && isPrime; j++) {
			if (i % j == 0) {
				isPrime = 0;
			}
		}
		if (isPrime) {
			printf("%d\n", i);
		}
	}
}

void perimeter() {
	int num1, num2;
	printf("perimeter:\nEnter 2 numbers: \n");
    	scanf("%d", &num1);
    	scanf("%d", &num2);
    	printf("perimeter: %d\n", (num1 + num2) * 2);
}

void area() {
	int num1, num2;
	printf("area:\nEnter 2 numbers: \n");
    	scanf("%d", &num1);
    	scanf("%d", &num2);
    	printf("area: %d\n", num1 * num2);
}

void cal() {
	int x = 5;
	x *= 5;
	for (int i = 0; i < 5; i++) {
		x += 5;
		x--;
	}
	x *= 10;
	if (x) {
		return;
	}
}

int factRec(int num) {
	if (num == 1) {
		return 1;
	}
	return num * factRec(num - 1);
}

void fact() {
	int num;
	printf("factorial:\nEnter a number: \n");
    	scanf("%d", &num);
    	printf("factorial: %d\n", factRec(num));
}

int main(int argc, char** argv) {
    int num;
    void (*menu[])() = {perimeter, area, prime, fact};
    while (1) {
    	printf("choose an option:\n1. perimeter\n2. area\n3. prime\n4. fact\n5. exit\n");
    	scanf("%d", &num);
    	if (num == 5) {
    		return 0;
    	}
    	menu[num - 1]();
    	cal();
    }
    return 0;
}