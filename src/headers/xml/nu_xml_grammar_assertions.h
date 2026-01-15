#pragma once

static int AssertRootGrammar(struct Vector* tokens)
{
    // ENFORCE RULE: FIRST TOKEN MUST BE OPEN TAG
    // ENFORCE RULE: SECOND TOKEN MUST BE TAG NAME
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE_TAG | PROPERTY
    int root_open = tokens->size > 2 && 
        *((enum NU_XML_TOKEN*) Vector_Get(tokens, 0)) == OPEN_TAG &&
        *((enum NU_XML_TOKEN*) Vector_Get(tokens, 1)) == WINDOW_TAG;

    if (tokens->size > 2 &&
        *((enum NU_XML_TOKEN*) Vector_Get(tokens, 0)) == OPEN_TAG &&
        *((enum NU_XML_TOKEN*) Vector_Get(tokens, 1)) == WINDOW_TAG) 
    {
        if (tokens->size > 5 && 
            *((enum NU_XML_TOKEN*) Vector_Get(tokens, tokens->size-3)) == OPEN_END_TAG &&
            *((enum NU_XML_TOKEN*) Vector_Get(tokens, tokens->size-2)) == WINDOW_TAG && 
            *((enum NU_XML_TOKEN*) Vector_Get(tokens, tokens->size-1)) == CLOSE_TAG)
        {
            return 0; // Success
        }
        else // Closing type is not window
        {
            printf("%s\n", "[Generate_Tree] Error! XML tree root not closed.");
            return -1;
        }
    }
    else // Root is not a window type
    {
        printf("%s\n", "[Generate_Tree] Error! XML tree has no root. XML documents must begin with a <window> tag.");
        return -1;
    }
}

static int AssertNewTagGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG NAME 
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE CLOSE_END OR PROPERTY
    if (i < tokens->size - 2 && NU_TokenToNodeType(*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1))) != NAT)
    {
        enum NU_XML_TOKEN third_token = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i+2));
        if (third_token == CLOSE_TAG || third_token == CLOSE_END_TAG || NU_Is_Token_Property(third_token)) return 1; // Success
    }
    enum NU_XML_TOKEN token = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i));
    enum NU_XML_TOKEN second_token = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1));
    enum NU_XML_TOKEN third_token = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i+2));
    printf("here %d %d %d\n", token, second_token, third_token);
    return 0; // Failure
}

static int AssertPropertyGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE PROPERTY ASSIGN
    // ENFORCE RULE: THIRD TOKEN SHOULD BE PROPERTY TEXT
    if (i < tokens->size - 2 && *((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1)) == PROPERTY_ASSIGNMENT)
    {
        if (*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+2)) == PROPERTY_VALUE) return 0; // Success
        printf("%s\n", "[Generate_Tree] Error! Expected property value after assignment.");
        return -1; // Failure
    }
    printf("%s\n", "[Generate_Tree] Error! Expected '=' after property.");
    return -1; // Failure
}

static int AssertTagCloseStartGrammar(struct Vector* tokens, int i, NodeType openType)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG AND MUST MATCH OPENING TAG
    // ENDORCE RULE: THIRD TOKEN MUST BE A TAG END
    if (i < tokens->size - 2 && 
        NU_TokenToNodeType(*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1))) == openType && 
        *((enum NU_XML_TOKEN*) Vector_Get(tokens, i+2)) == CLOSE_TAG) return 0; // Success
    else {
        printf("%s", "[Generate Tree] Error! close tag does not match opening tag. ");
        printf("%s %d %s %d", "close tag:", NU_TokenToNodeType(*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1))), "open tag:", openType);
    }
    return -1; // Failure
}
