#pragma once

struct RGB {
    uint8_t r, g, b;
};

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

static int String_To_Int(int* result, char* str)
{
    *result = 0;
    int i = 0;
    int negative = 0;

    if (str[0] == '-') {
        negative = 1;
        i = 1;
    }

    if (str[i] == '\0') {
        *result = 0;
        return 0; // no digits after sign
    }

    while (str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0;
        }
        int digit = c - '0';
        *result = (*result * 10) + digit;
        i += 1;
    }

    if (negative)
        *result = -*result;

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

static int Hex_To_Int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1; 
}

bool Parse_Hexcode(const char* string, int char_count, struct RGB* rgb) {
    if ((char_count != 7 && char_count != 9) || string[0] != '#') return false;
    int r1 = Hex_To_Int(string[1]);
    int r2 = Hex_To_Int(string[2]);
    int g1 = Hex_To_Int(string[3]);
    int g2 = Hex_To_Int(string[4]);
    int b1 = Hex_To_Int(string[5]);
    int b2 = Hex_To_Int(string[6]);
    if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) return false;
    rgb->r = (uint8_t)((r1 << 4) | r2);
    rgb->g = (uint8_t)((g1 << 4) | g2);
    rgb->b = (uint8_t)((b1 << 4) | b2);
    return true;
}