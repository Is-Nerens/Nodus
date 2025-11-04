#pragma once

static int AssertRootGrammar(struct Vector* token_vector)
{
    // ENFORCE RULE: FIRST TOKEN MUST BE OPEN TAG
    // ENFORCE RULE: SECOND TOKEN MUST BE TAG NAME
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE_TAG | PROPERTY
    int root_open = token_vector->size > 2 && 
        *((enum NU_XML_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG;

    if (token_vector->size > 2 &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG) 
    {
        if (token_vector->size > 5 && 
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-3)) == OPEN_END_TAG &&
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-2)) == WINDOW_TAG && 
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-1)) == CLOSE_TAG)
        {
            return 0; // Success
        }
        else // Closing tag is not window
        {
            printf("%s\n", "[Generate_Tree] Error! XML tree root not closed.");
            return -1;
        }
    }
    else // Root is not a window tag
    {
        printf("%s\n", "[Generate_Tree] Error! XML tree has no root. XML documents must begin with a <window> tag.");
        return -1;
    }
}

static int AssertNewTagGrammar(struct Vector* token_vector, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG NAME 
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE CLOSE_END OR PROPERTY
    if (i < token_vector->size - 2 && NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))) != NAT)
    {
        enum NU_XML_Token third_token = *((enum NU_XML_Token*) Vector_Get(token_vector, i+2));
        if (third_token == CLOSE_TAG || third_token == CLOSE_END_TAG || NU_Is_Token_Property(third_token)) return 0; // Success
    }
    return -1; // Failure
}

static int AssertPropertyGrammar(struct Vector* token_vector, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE PROPERTY ASSIGN
    // ENFORCE RULE: THIRD TOKEN SHOULD BE PROPERTY TEXT
    if (i < token_vector->size - 2 && *((enum NU_XML_Token*) Vector_Get(token_vector, i+1)) == PROPERTY_ASSIGNMENT)
    {
        if (*((enum NU_XML_Token*) Vector_Get(token_vector, i+2)) == PROPERTY_VALUE) return 0; // Success
        printf("%s\n", "[Generate_Tree] Error! Expected property value after assignment.");
        return -1; // Failure
    }
    printf("%s\n", "[Generate_Tree] Error! Expected '=' after property.");
    return -1; // Failure
}

static int AssertTagCloseStartGrammar(struct Vector* token_vector, int i, enum Tag openTag)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG AND MUST MATCH OPENING TAG
    // ENDORCE RULE: THIRD TOKEN MUST BE A TAG END
    if (i < token_vector->size - 2 && 
        NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))) == openTag && 
        *((enum NU_XML_Token*) Vector_Get(token_vector, i+2)) == CLOSE_TAG) return 0; // Success
    else {
        printf("%s", "[Generate Tree] Error! Closing tag does not match.");
        printf("%s %d %s %d", "close tag:", NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))), "open tag:", openTag);
    }
    return -1; // Failure
}
