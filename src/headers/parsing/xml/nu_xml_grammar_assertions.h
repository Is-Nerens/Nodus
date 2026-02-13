#pragma once

static int AssertRootGrammar(TokenArray* tokens)
{
    // ENFORCE RULE: FIRST TOKEN MUST BE OPEN TAG
    // ENFORCE RULE: SECOND TOKEN MUST BE TAG NAME
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE_TAG | PROPERTY
    int root_open = tokens->size > 2 && 
        TokenArray_Get(tokens, 0) == OPEN_TAG &&
        TokenArray_Get(tokens, 1) == WINDOW_TAG;

    if (tokens->size > 2 &&
        TokenArray_Get(tokens, 0) == OPEN_TAG &&
        TokenArray_Get(tokens, 1) == WINDOW_TAG) 
    {
        if (tokens->size > 5 && 
            TokenArray_Get(tokens, tokens->size-3) == OPEN_END_TAG &&
            TokenArray_Get(tokens, tokens->size-2) == WINDOW_TAG && 
            TokenArray_Get(tokens, tokens->size-1) == CLOSE_TAG)
        {
            return 1; // Success
        }
        else // Closing type is not window
        {
            printf("%s\n", "[Generate_Tree] Error! XML tree root not closed.");
            return 0;
        }
    }
    else // Root is not a window type
    {
        printf("%s\n", "[Generate_Tree] Error! XML tree has no root. XML documents must begin with a <window> tag.");
        return 0;
    }
}

static int AssertNewTagGrammar(struct TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG NAME 
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE CLOSE_END OR PROPERTY
    if (i < tokens->size - 2 && NU_TokenToNodeType(TokenArray_Get(tokens, i+1)) != NU_NAT)
    {
        enum NU_XML_TOKEN third_token = TokenArray_Get(tokens, i+2);
        if (third_token == CLOSE_TAG || third_token == CLOSE_END_TAG || NU_Is_Token_Property(third_token)) return 1; // Success
    }
    return 0; // Failure
}

static int AssertPropertyGrammar(struct TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE PROPERTY ASSIGN
    // ENFORCE RULE: THIRD TOKEN SHOULD BE PROPERTY TEXT
    if (i < tokens->size - 2 && TokenArray_Get(tokens, i+1) == PROPERTY_ASSIGNMENT)
    {
        if (TokenArray_Get(tokens, i+2) == PROPERTY_VALUE) return 1; // Success
        printf("%s\n", "[Generate_Tree] Error! Expected property value after assignment.");
        return 0; // Failure
    }
    printf("%s\n", "[Generate_Tree] Error! Expected '=' after property.");
    return 0; // Failure
}

static int AssertTagCloseStartGrammar(struct TokenArray* tokens, int i, NodeType openType)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG AND MUST MATCH OPENING TAG
    // ENDORCE RULE: THIRD TOKEN MUST BE A TAG END
    if (i < tokens->size - 2 && 
        NU_TokenToNodeType(TokenArray_Get(tokens, i+1)) == openType && 
        TokenArray_Get(tokens, i+2) == CLOSE_TAG) return 1; // Success
    else {
        printf("%s", "[Generate Tree] Error! close tag does not match opening tag. ");
        printf("%s %d %s %d", "close tag:", NU_TokenToNodeType(TokenArray_Get(tokens, i+1)), "open tag:", openType);
    }
    return 0; // Failure
}
