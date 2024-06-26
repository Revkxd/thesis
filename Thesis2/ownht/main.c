#define OWNHT
#ifdef OWNHT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#include "../src/constants.h"
#include "../src/converters.c"
#include "../src/maxfunctions.c"
#include "selfht.c"

#define PRINTERS 1
#undef PRINTERS

int reversed = 0;

void modded_three_frame(char* dnaSequence, char* proteinSequence, int C[][strlen(proteinSequence) + 1]) {
    int N = strlen(dnaSequence), M = strlen(proteinSequence);
    int gep = 2, gop = 3, frameshift_penalty = 4;
    int I[N][M + 1], D[N][M + 1];
    int TI[N][M + 1], TD[N][M + 1], TC[N][M + 1];
    int i,j;

    // Anti-Garbage Values
    for(i = 0; i < N; i++) {
        for(j = 0; j < M + 1; j++) {
            I[i][j] = 0;
            D[i][j] = 0;
            C[i][j] = 0;
            TI[i][j] = 0;
            TD[i][j] = 0;
            TC[i][j] = 0;
        }
    }

    // NOTE: INFINITY is a double, just converted to int to save memory
    // Initialization
    for(j = 0; j < M + 1; j++) {
        I[0][j] = -999;
        D[0][j] = -999;
        D[2][j] = -999;
        D[3][j] = -999;
        D[1][j] = C[0][j] - gop - gep;
        TI[j][0] = -999;
        TD[0][j] = -999;
        TD[2][j] = -999;
        TD[3][j] = -999;
        TD[1][j] = 1;
    }

    C[0][0] = 0;
    TC[0][0] = 0;
    for(j = 1; j < M + 1; j++) {
        C[0][j] = 0;
        C[j][0] = 0;
        C[1][j] = max_of_three(I[1][j], D[1][j], C[0][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 1)));
        C[2][j] = max_of_two(I[2][j], C[0][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 2)) - frameshift_penalty);
        C[3][j] = max_of_two(I[3][j], C[1][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 3)) - frameshift_penalty);
        C[4][j] = max_of_four(I[4][j], D[4][j], C[1][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 4)), C[2][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 4)) - frameshift_penalty);

        TC[0][j] = 0;
        TC[j][0] = 0;
        TC[1][j] = C[1][j] == I[1][j] ? -2 : (C[1][j] == D[1][j] ? -1 : (C[1][j] == C[0][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 1)) ? 1 : 0));
        TC[2][j] = C[2][j] == I[2][j] ? -2 : (C[2][j] == C[0][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 2)) - frameshift_penalty ? 2 : 0);
        TC[3][j] = C[3][j] == I[3][j] ? -2 : (C[3][j] == C[1][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 3)) - frameshift_penalty ? 2 : 0);
        if(C[4][j] == I[4][j])
            TC[4][j] = -2;
        else if(C[4][j] == D[4][j])
            TC[4][j] = -1;
        else if(C[4][j] == C[1][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 4)))
            TC[4][j] = 3;
        else if(C[4][j] == C[2][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, 4)) - frameshift_penalty)
            TC[4][j] = 2;
        else
            TC[4][j] = 0;
    }

    // Change all negative values in C matrix to 0
    for(i = 0; i < N; i++) {
        for(j = 0; j < M + 1; j++) {
            if(C[i][j] < 0)
                C[i][j] = 0;
        }
    }

    char filename[50];
    char letter;
    // Matrix Filling
    for(i = 0; i < N; i++) {
        letter = 'A' + i;
        strcpy(filename, &letter);
        if (reversed)
            strcat(filename, &letter);
        strcat(filename, ".txt");
        FILE *fp = fopen(filename, "w");
        for(j = 1; j < M + 1; j++) {
            if(!ht_search(i, j, 1, &I[i][j])) {
                I[i][j] = max_of_two(I[i][j-1] - gep, C[i][j-1] - gop - gep);
                ht_insert(i, j, 1, I[i][j]);
            }
            if (i < 4)
                continue;
            if(!ht_search(i, j, 2, &D[i][j])) {
                D[i][j] = max_of_two(D[i-3][j] - gep, C[i-3][j] - gop - gep);
                ht_insert(i, j, 2, D[i][j]);
            }
            if(!ht_search(i, j, 3, &C[i][j])) {
                C[i][j] = max_of_five(
                    I[i][j],
                    D[i][j],
                    C[i-2][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)) - frameshift_penalty,
                    C[i-3][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)),
                    C[i-4][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)) - frameshift_penalty
                );
                ht_insert(i, j, 3, C[i][j]);
            }
        }
        print_table_to_file(fp);
        fclose(fp);
    }

    // Traceback Matrix Filling
    for(i = 0; i < N; i++) {
        letter = 'A' + i;
        strcpy(filename, &letter);
        if (reversed)
            strcat(filename, &letter);
        strcat(filename, "_traceback.txt");
        FILE *fp = fopen(filename, "w");
        for(j = 0; j < M; j++) {
            if(!ht_search(i, j, 4, &TI[i][j])) {
                TI[i][j] = I[i][j] == I[i][j-1] - gep ? 0 : (I[i][j] == C[i][j-1] - gop - gep ? 1 : -999);
                ht_insert(i, j, 4, TI[i][j]);
            }
            if(!ht_search(i, j, 5, &TD[i][j])) {
                TD[i][j] = D[i][j] == D[i-3][j] - gep ? 0 : (D[i][j] == C[i-3][j] - gop - gep ? 1 : -999);
                ht_insert(i, j, 5, TD[i][j]);
            }
            if(!ht_search(i, j, 6, &TC[i][j])) {
                if(C[i][j] == I[i][j])
                    TC[i][j] = -2;
                else if(C[i][j] == D[i][j])
                    TC[i][j] = -1;
                else if(C[i][j] == C[i-2][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)) - frameshift_penalty)
                    TC[i][j] = 2;
                else if(C[i][j] == C[i-3][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)))
                    TC[i][j] = 3;
                else if(C[i][j] == C[i-4][j-1] + get_score(proteinSequence[j - 1], get_translated_codon(dnaSequence, i)) - frameshift_penalty)
                    TC[i][j] = 4;
                else
                    TC[i][j] = 0;
                ht_insert(i, j, 6, TC[i][j]);
            }
        }
        print_table_to_file(fp);
        fclose(fp);
    }

    // Print the matrices for debugging
    #ifdef PRINTERS
    printf("I Matrix:\n");
    print_matrix(N, M + 1, I);
    printf("D Matrix:\n");
    print_matrix(N, M + 1, D);
    printf("C Matrix:\n");
    print_matrix(N, M + 1, C);
    printf("TI Matrix:\n");
    print_matrix(N, M + 1, TI);
    printf("TD Matrix:\n");
    print_matrix(N, M + 1, TD);
    printf("TC Matrix:\n");
    print_matrix(N, M + 1, TC);
    #endif
}

int six_frame(char* dnaSequence, char* proteinSequence) {
    int N = strlen(dnaSequence), M = strlen(proteinSequence);
    int C1[N][M + 1], C2[N][M + 1];
    #ifdef PRINTERS
    printf("First Run:\n");
    #endif
    reversed = 0;
    modded_three_frame(dnaSequence, proteinSequence, C1);

    #ifdef PRINTERS
    printf("Reverse Complement:\n");
    #endif
    reversed = 1;
    reverse_complement(dnaSequence);
    modded_three_frame(dnaSequence, proteinSequence, C2);

    int max1 = -999, max2 = -999;
    int val1, val2;
    int i, j;
    for(i = 0; i < strlen(dnaSequence); i++) {
        for(j = 0; j < strlen(proteinSequence) + 1; j++) {
            val1 = C1[i][j];
            val2 = C2[i][j];

            max1 = val1 > max1 ? val1 : max1;
            max2 = val2 > max2 ? val2 : max2;
        }
    }

    return max_of_two(max1, max2);
}

int main() {
    String dnaSequences[] = {"AGTGTAGTGCCCTCTAGTTCA","ATGCG", "ATGCGA", "ATGCGATACGCTTGA", "CTTGGTCCGAAT"};
    String proteinSequences[] = {"TVS-PGS", "MCA", "MR", "MRIR", "LGPL"};
    int i;
    double time_taken, start, end;
    for(i = 0; i < 5; i++) {
        init_hash_table();
        printf("DNA Sequence: %s\n", dnaSequences[i]);
        printf("Protein Sequence: %s\n", proteinSequences[i]);
        start = clock();
        printf("Score: %d\n\n", six_frame(dnaSequences[i], proteinSequences[i]));
        end = clock();
        time_taken = (double)(end - start)*1e3 / CLOCKS_PER_SEC;
        printf("Run %d time taken: %f ms\n\n", i, time_taken);
        // break;
    }
    return 0;
}
#endif