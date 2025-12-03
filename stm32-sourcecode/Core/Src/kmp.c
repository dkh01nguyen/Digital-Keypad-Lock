#include "kmp.h"


const char VALID_PASSWORD[PASSWORD_LENGTH] = "1234";

/**
Build LPS array for 4-character pattern
**/

void KMP_BuildLPS(const uint8_t *pattern, uint16_t *lps)
{
    uint16_t len = 0;
    lps[0] = 0;
    uint16_t i = 1;
    
    while (i < PASSWORD_LENGTH) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
}

bool KMP_FindPassword(const uint8_t *input, uint16_t length, const uint8_t *password)
{
    if (length < PASSWORD_LENGTH) {
        return false;
    }
    
    // Build LPS array
    uint16_t lps[PASSWORD_LENGTH];
    KMP_BuildLPS(password, lps);
    
    uint16_t i = 0;  // Index for input
    uint16_t j = 0;  // Index for password
    
    while (i < length) {
        if (password[j] == input[i]) {
            i++;
            j++;
        }
        
        if (j == PASSWORD_LENGTH) {
            // Password found!
            return true;
        } else if (i < length && password[j] != input[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    
    return false;  // Password not found
}
