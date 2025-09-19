#pragma once

struct Font_Resource
{
    char* name;
    uint8_t* data; 
    int size;  
};

struct Font_Registry {
    int* font_ids;  
    int count;      
};

