#pragma once

static bool StringToFloat32(float32* result, const char* str)
{
    *result = 0.0f;
    float fraction_divider = 1.0f;
    int32 decimal_found = 0;
    int32 i=0;
    while (str[i] != '\0') {
        char c = str[i];
        if (c == '.') {
            if (decimal_found) {
                *result = 0.0f;
                return 0;
            }
            decimal_found = 1;
            i += 1;
            continue;
        }
        if (c < '0' || c > '9') {
            *result = 0.0f;
            return 0;
        }
        int32 digit = c - '0';
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

static bool CStringToInt32(int32* result, const char* str)
{
    if (!result || !str) return 0;
    *result = 0;
    int32 i = 0;
    int32 negative = 0;
    if (str[0] == '-') {
        negative = 1;
        i = 1;
    }
    if (str[i] == '\0') {
        *result = 0;
        return 0;
    }
    while (str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0;
        }
        int32 digit = c - '0';
        if (*result > (INT32_MAX - digit) / 10) {
            *result = INT32_MAX;
            return 1;
        }
        *result = (*result * 10) + digit;
        i += 1;
    }
    if (negative) *result = -*result;
    return 1;
}

static bool CStringToUint8(uint8* result, const char* str)
{
    if (!result || !str) return 0;
    *result = 0;
    int32 i=0;
    if (str[0] == '\0') return 0;
    while(str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0;
        }
        int32 digit = c - '0';
        if (*result > (UINT8_MAX - digit) / 10) {
            *result = UINT8_MAX;
        }
        else {
            *result = (uint8)(*result * 10 + digit);
        }
        i += 1;
    }
    return 1;
}

static bool CStringToUint16(uint16* result, const char* str)
{
    if (!result || !str) return 0;
    uint32 val = 0;
    int32 i = 0;
    while (str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0; // invalid character
        }
        uint32 digit = (uint32)(c - '0');
        if (val > (UINT16_MAX - digit) / 10) {
            val = UINT16_MAX;
            break;
        }
        val = val * 10 + digit;
        i++;
    }
    *result = (uint16)val;
    return 1;
}

static bool CStringToInt16(int16* result, const char* str)
{
    if (!result || !str) return 0;
    int32 val = 0;
    int32 i = 0;
    while (str[i] != '\0') {
        char c = str[i];
        if (c < '0' || c > '9') {
            *result = 0;
            return 0;
        }
        int32 digit = c - '0';
        if (val > (INT16_MAX - digit) / 10) {
            val = INT16_MAX; // saturate on overflow
            break;
        }
        val = val * 10 + digit;
        i++;
    }
    *result = (int16)val;
    return 1;
}
