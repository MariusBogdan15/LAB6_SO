#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

void findPrimesInRange(int start, int end, int writePipe) {
    std::vector<int> primes;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }
    
    for (int prime : primes) {
        write(writePipe, &prime, sizeof(prime));
    }
    
    int endMarker = -1;
    write(writePipe, &endMarker, sizeof(endMarker));
}

int main() {
    const int NUM_PROCESSES = 10;
    const int RANGE = 1000;
    int start = 1;

    int pipes[NUM_PROCESSES][2];

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("Error creating pipe");
            return 1;
        }
    }

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int pid = fork();
        if (pid == -1) {
            perror("Error creating process");
            return 1;
        } else if (pid == 0) {
            // Proces copil
            close(pipes[i][0]); // Închide capătul de citire
            int rangeStart = start + i * RANGE;
            int rangeEnd = rangeStart + RANGE - 1;
            findPrimesInRange(rangeStart, rangeEnd, pipes[i][1]);
            close(pipes[i][1]); // Închide capătul de scriere
            exit(0);
        }
        // Proces părinte
        close(pipes[i][1]); // Închide capătul de scriere
    }

    
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime;
        while (read(pipes[i][0], &prime, sizeof(prime)) > 0) {
            if (prime == -1) break; // Marcaj de final
            std::cout << "Proces " << i << ": " << prime << std::endl;
        }
        close(pipes[i][0]); // Închide capătul de citire
    }

    
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        wait(NULL);
    }

    return 0;
}
