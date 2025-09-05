#pragma once

static int String_To_Float(float* result, char* str)
{
    *result = 0.0f;
    float fraction_divider = 1.0f;
    int decimal_found = 0;
    int i=0;
    while (str[i] != '\0') {
        char c = str[i];
        if (c == '.') {
            if (decimal_found) {
                *result = 0.0f;
                return 0;
            }
            decimal_found = 1;
            continue;
        }
        if (c < '0' || c > '9') {
            *result = 0.0f;
            return 0;
        }
        int digit = c - '0';
        if (!decimal_found) {
            *result = (*result * 10.0f) + digit;
        }
        else {
            fraction_divider *= 10.0f;
            *result += digit / fraction_divider;
        }
        i += 1;
    }
    return 1;
}

static int String_To_uint8_t(uint8_t* result, char* str)
{
    *result = 0;
    int i=0;
    while(str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0;
        }
        int digit = c - '0';
        if (*result > (UINT8_MAX - digit) / 10) {
            *result = 255; // limit to max uint8_t value
        }
        *result = (uint8_t)(*result * 10 + digit);
        i += 1;
    }
    return 1;
}
