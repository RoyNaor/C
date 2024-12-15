#include <stdio.h>
#include "methods.h"

#define N 4

int main() {
    int choice;
    int grades[N];

    printf("Choose a function to run:\n");
    printf("1. Add Grades\n");
    printf("2. Check Palindrome\n");
    printf("3. Transpose Matrix\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            addGrades(grades, N);
            break;

        case 2:
            printf("Enter a string to check for palindrome: ");
            char str[100];
            scanf("%s", str);
            isPalindrome(str);
            break;

        case 3: {
            int n;
            printf("Enter the size of the matrix (n x n): ");
            scanf("%d", &n);

            if (n > 100) {
                printf("Matrix size is too large (max 100).\n");
                break;
            }

            int matrix[100][100];
            printf("Enter the elements of the matrix:\n");
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    scanf("%d", &matrix[i][j]);
                }
            }

            transposeMatrix(matrix, n);
            break;

        default:
            printf("Invalid choice. Please select 1 or 2.\n");
    }

}

    return 0;
}
