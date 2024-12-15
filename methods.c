#include <stdio.h>
#include "methods.h"
#include <string.h>

// ex1
void addGrades(int grades[], int size) {
    double total = 0;

    for (int i = 0; i < size; i++) {
        printf("Enter grade for student %d: ", i + 1);
        scanf("%d", &grades[i]);
        total += grades[i];
    }

    double avg = total / size;
    printf("The avg grade is: %.2lf\n", avg);

    printf("The grades that are above the avg are: ");
    for (int j = 0; j < size; j++) {
        if (grades[j] > avg) {
            printf("%d ", grades[j]);
        }
    }
    printf("\n");
}

//--------------------------------------------------------
// ex2
int isPalindrome(const char *str) {
    int length = strlen(str);
    int left = 0;
    int right = length - 1;

    while (left < right) {
        if (str[left] != str[right]) {
            printf("\"%s\" is not a palindrome.\n", str);
            return 0; 
        }
        left++;
        right--;
    }

    printf("\"%s\" is a palindrome.\n", str);
    return 1; 
}
//------------------------------------------------------
// ex3
void printMatrix(int matrix[][100], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

void transposeMatrix(int matrix[][100], int n) {
    printf("Matrix before transposition:\n");
    printMatrix(matrix, n);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            int temp = matrix[i][j];
            matrix[i][j] = matrix[j][i];
            matrix[j][i] = temp;
        }
    }

    printf("Matrix after transposition:\n");
    printMatrix(matrix, n);
}

